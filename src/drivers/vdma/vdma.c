/**************************************************************************
** Copyright (C) 2021 Toshinobu Hondo, Ph.D.
** Copyright (C) 2021 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.TXT included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/

#include "vdma.h"
// #include "dma_ingress.h"
#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h> // create_proc_entry
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/time.h>

static char *devname = MODNAME;

MODULE_AUTHOR( "Toshinobu Hondo" );
MODULE_DESCRIPTION( "Driver for vdma" );
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(devname, MODNAME " param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static struct platform_driver __vdma_platform_driver;
struct platform_device * __pdev;

enum dma { dmalen = 16*sizeof(u32) }; // 512bits

enum direction { dma_r = 0 /*dma_w = 1 */};

enum dma_ingress_egress {
    ingress_address     = 0x30000000
    , ingress_size      = 0x08000000
    , ingress_stream    = 0x00000000
    , egress_stream     = 0x00000000
};

struct vdma_driver {
    uint32_t irq;
    const char * label;
    uint64_t irqCount;
    void __iomem * iomem;
    uint32_t * dma_ptr;
    uint32_t phys_source_address;
    uint32_t phys_destination_address;
    wait_queue_head_t queue;
    int queue_condition;
    struct semaphore sem;
    uint32_t wp;     // next effective dma phys addr
    uint32_t readp;  // fpga read phys addr
    struct resource * mem_resource;
};

struct vdma_cdev_reader {
    struct vdma_driver * driver;
    u32 rp;
};

enum { dg_data_irq_mask = 2 };

static int __debug_level__ = 0;

struct core_register {
    u32 offset;
    const char * name;
    u32 replicates;
};
const struct core_register __core_register [] = {
    { 0x00, "MM2S VDMA Control Register", 1 }
    , { 0x04, "MM2S VDMA Status Register", 1 }
    , { 0x14, "MM2S Register Index", 1 }
    , { 0x28, "MM2S and S1MM Park Pointer Register", 1 }
    , { 0x2c, "Video DMA Version register", 1 }
    , { 0x30, "S2MM VDMA Control Register", 1 }
    , { 0x34, "S2MM VDMA Status Register", 1 }
    , { 0x3c, "S2MM_VDMA_IRQ_MASK", 1 }
    , { 0x44, "S2MM Register Index", 1 }
    , { 0x50, "MM2S_VSIZE MM2S Vertical Size Register", 1 }
    , { 0x54, "MM2S_HSIZE MM2S Horizontal Size Register", 1 }
    , { 0x58, "MM2S_FRMDLY_STRIDE MM2S Frame Delay and Stride Register", 1 }
    , { 0x5c, "MM2S Start Address", 16 }
    , { 0xa0, "Vertical Size Register", 1 }
    , { 0xa4, "Horizontal Size Register", 1 }
    , { 0xa8, "Frame Delay and Stride Register", 1 }
    , { 0xac, "S2MM Start Address", 16 }
};

static inline u32 vdma_reg_read(struct vdma_driver * drv, u32 addr)
{
	return ioread32(drv->iomem + addr);
}



static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    struct vdma_driver * driver = dev_id ? platform_get_drvdata( dev_id ) : 0;
    (void)(driver);
    return IRQ_HANDLED;
}

static int
vdma_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "vdma debug level = %d\n", __debug_level__ );
    struct vdma_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        seq_printf( m
                    , "csi2rx mem resource: %x -- %x, map to %p\n"
                    , __pdev->resource->start, __pdev->resource->end, drv->iomem );
        for ( size_t i = 0; i < countof( __core_register ); ++i ) {
            for ( size_t k = 0; k < __core_register[ i ].replicates; ++k ) {
                u32 offset = __core_register[ i ].offset + ( k * sizeof( u32 ) );
                if ( __core_register[ i ].replicates == 1 ) {
                    seq_printf( m, "0x%04x\t%08x\t%s\n"
                                , offset, vdma_reg_read( drv, offset ), __core_register[ i ].name );
                } else {
                    seq_printf( m, "0x%04x\t%08x\t%s [%d]\n"
                                , offset, vdma_reg_read( drv, offset ), __core_register[ i ].name, k );
                }
            }
        }
    }
    return 0;
}

static ssize_t
vdma_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    return size;
}

static int
vdma_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, vdma_proc_read, NULL );
}

static const struct proc_ops proc_file_fops = {
    .proc_open    = vdma_proc_open,
    .proc_read    = seq_read,
    .proc_write   = vdma_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int vdma_cdev_open(struct inode *inode, struct file *file)
{
    unsigned int node = MINOR( inode->i_rdev );
    (void)node;
    return 0;
}

static int
vdma_cdev_release( struct inode *inode, struct file *file )
{
    unsigned int node = MINOR( inode->i_rdev );
    (void)node;
    return 0;
}

static long vdma_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t vdma_cdev_read( struct file * file, char __user *data, size_t size, loff_t *f_pos )
{
    return size;
}

static ssize_t
vdma_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    ssize_t processed = 0;
    return processed;
}

static ssize_t
vdma_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
    return 0; // Success
}

loff_t
vdma_cdev_llseek( struct file * file, loff_t offset, int orig )
{
    file->f_pos += offset;
    return file->f_pos;
}

static struct file_operations vdma_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = vdma_cdev_llseek
    , .open    = vdma_cdev_open
    , .release = vdma_cdev_release
    , .unlocked_ioctl = vdma_cdev_ioctl
    , .read    = vdma_cdev_read
    , .write   = vdma_cdev_write
    , .mmap    = vdma_cdev_mmap
};

static dev_t vdma_dev_t = 0;
static struct cdev * __vdma_cdev;
static struct class * __vdma_class;

static int vdma_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int __init
vdma_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&vdma_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __vdma_cdev = cdev_alloc();
    if ( !__vdma_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __vdma_cdev, &vdma_cdev_fops );
    if ( cdev_add( __vdma_cdev, vdma_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return -ENOMEM;
    }

    __vdma_class = class_create( THIS_MODULE, MODNAME );
    if ( !__vdma_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return -ENOMEM;
    }
    __vdma_class->dev_uevent = vdma_dev_uevent;

    // make_nod /dev/vdma
    if ( !device_create( __vdma_class, NULL, vdma_dev_t, NULL, MODNAME "%d", MINOR( vdma_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return -ENOMEM;
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &proc_file_fops );

    platform_driver_register( &__vdma_platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
vdma_module_exit( void )
{
    platform_driver_unregister( &__vdma_platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __vdma_class, vdma_dev_t ); // device_creeate

    class_destroy( __vdma_class ); // class_create

    cdev_del( __vdma_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( vdma_dev_t, 1 ); // alloc_chrdev_region
    //

    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static int
vdma_module_probe( struct platform_device * pdev )
{
    int irq = 0;

    struct vdma_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct vdma_driver ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    __pdev = pdev;
    dev_info( &pdev->dev, "vdma_module probed" );

	drv->iomem = devm_platform_ioremap_resource(pdev, 0);
	if ( IS_ERR( drv->iomem ) ) {
        dev_err(&pdev->dev, "iomem get failed");
		return PTR_ERR(drv->iomem);
    }
    dev_info( &pdev->dev, "vdma probe resource: %x -- %x, map to %p\n"
              , pdev->resource->start, pdev->resource->end, drv->iomem );

    sema_init( &drv->sem, 1 );

    platform_set_drvdata( pdev, drv );

    if ( ( irq = platform_get_irq( pdev, 0 ) ) > 0 ) {
        dev_info( &pdev->dev, "platform_get_irq: %d", irq );
        if ( devm_request_irq( &pdev->dev, irq, handle_interrupt, 0, MODNAME, pdev ) == 0 ) {
            drv->irq = irq;
        } else {
            dev_err( &pdev->dev, "Failed to register IRQ.\n" );
            return -ENODEV;
        }
    }
    return 0;
}

static int
vdma_module_remove( struct platform_device * pdev )
{
    struct vdma_driver * drv = platform_get_drvdata( pdev );

    if ( drv && drv->irq > 0 ) {
        dev_info( &pdev->dev, "IRQ %d about to be freed\n", drv->irq );
    }
    dev_info( &pdev->dev, "Unregistered.\n" );

    return 0;
}

static const struct of_device_id __vdma_module_id [] = {
    { .compatible = "xlnx,axi-vdma-6.3" }
    , { .compatible = "xlnx,axi-vdma-1.00.a" }
    , {}
};

static struct platform_driver __vdma_platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __vdma_module_id )
        ,
    }
    , .probe = vdma_module_probe
    , .remove = vdma_module_remove
};

module_init( vdma_module_init );
module_exit( vdma_module_exit );
