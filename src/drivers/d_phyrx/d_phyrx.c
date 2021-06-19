/**************************************************************************
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

#include "d_phyrx.h"
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h> // create_proc_entry
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

static char *devname = MODNAME;

MODULE_AUTHOR( "Toshinobu Hondo" );
MODULE_DESCRIPTION( "Driver for the Delay Pulse Generator" );
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(devname, "mipi-d_phy_rx param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static int __debug_level__ = 0;
static struct platform_driver __platform_driver;
static dev_t d_phy_rx_dev_t = 0;
static struct cdev * __d_phy_rx_cdev;
static struct class * __d_phy_rx_class;
static struct platform_device *__pdev;

struct mipi_d_phy_rx_driver {
    uint32_t irq;
    uint64_t irqCount_;
    struct semaphore sem;
    wait_queue_head_t queue;
	struct gpio_desc *rst_gpio;
	struct clk_bulk_data *clks;
	void __iomem *iomem;
	u32 max_num_lanes;
	u32 datatype;
	struct mutex lock;
	// struct media_pad pads[XCSI_MEDIA_PADS];
	bool streaming;
	bool enable_active_lanes;
	bool en_vcx;
};

typedef struct d_phy_rx_cdev_private {
    u32 node;
    u32 size;
} d_phy_rx_cdev_private;

struct core_register {
    u32 offset;
    const char * name;
};


static int
d_phy_rx_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int
d_phy_rx_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "debug level = %d\n", __debug_level__ );
    struct mipi_d_phy_rx_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        seq_printf( m
                    , "d_phy_rx mem resource: %x -- %x, map to %pK\n"
                    , __pdev->resource->start, __pdev->resource->end, drv->iomem );
        u32 addr = __pdev->resource->start;
        for ( u32 i = 0; i < 16; ++i ) {
            const u32 * p = (u32*)( ((u8 *)drv->iomem) + (i * 16));
            seq_printf( m, "%08x: %08x %08x\t%08x %08x\n", addr, p[0], p[1], p[2], p[3] );
            addr += 4 * sizeof(*p);
        }
    }
    return 0;
}

static ssize_t
d_phy_rx_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    static char readbuf[256];

    if ( size >= sizeof( readbuf ) )
        size = sizeof( readbuf ) - 1;

    if ( copy_from_user( readbuf, user, size ) )
        return -EFAULT;

    readbuf[ size ] = '\0';

    return size;
}

static int
d_phy_rx_proc_open( struct inode * inode, struct file * file )
{
    printk( KERN_INFO "" MODNAME " proc_open private_data=%x\n", (u32)file->private_data );
    return single_open( file, d_phy_rx_proc_read, NULL );
}

static const struct proc_ops d_phy_rx_proc_file_fops = {
    .proc_open    = d_phy_rx_proc_open,
    .proc_read    = seq_read,
    .proc_write   = d_phy_rx_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/////////////////////////////////
/////////////////////////////////

static int d_phy_rx_cdev_open(struct inode *inode, struct file *file)
{
    struct d_phy_rx_cdev_private *
        private_data = devm_kzalloc( &__pdev->dev
                                     , sizeof( struct d_phy_rx_cdev_private )
                                     , GFP_KERNEL );
    if ( private_data ) {
        private_data->node =  MINOR( inode->i_rdev );
        private_data->size = 64 * 4;
        file->private_data = private_data;
    }

    return 0;

}

static int
d_phy_rx_cdev_release( struct inode *inode, struct file *file )
{
    if ( file->private_data ) {
        devm_kfree( &__pdev->dev, file->private_data );
    }
    return 0;
}

static long d_phy_rx_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t d_phy_rx_cdev_read( struct file * file, char __user* data, size_t size, loff_t* f_pos )
{
    dev_info( &__pdev->dev, "d_phy_rx_cdev_read: fpos=%llx, size=%ud\n", *f_pos, size );

    struct mipi_d_phy_rx_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        d_phy_rx_cdev_private * private_data = file->private_data;
        size_t dsize = private_data->size - *f_pos;
        if ( dsize > size )
            dsize = size;
        if ( copy_to_user( data, (const char *)drv->iomem + (*f_pos), dsize ) ) {
            return -EFAULT;
        }
        *f_pos += dsize;
        return dsize;
    }
    return 0;
}

static ssize_t
d_phy_rx_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    dev_info( &__pdev->dev, "d_phy_rx_cdev_write: fpos=%llx, size=%ud\n", *f_pos, size );
    return size;
}

static ssize_t
d_phy_rx_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
    int ret = (-1);
    return ret;
}

loff_t
d_phy_rx_cdev_llseek( struct file * file, loff_t offset, int orig )
{
    // struct d_phy_rx_cdev_reader * reader = file->private_data;
    loff_t pos = 0;
    switch( orig ) {
    case 0: // SEEK_SET
        pos = offset;
        break;
    case 1: // SEEK_CUR
        pos = file->f_pos + offset;
        break;
    case 2: // SEEK_END
        pos = file->f_pos + offset;
        break;
    default:
        return -EINVAL;
    }
    if ( pos < 0 )
        return -EINVAL;
    file->f_pos = pos;
    return pos;
}

static struct file_operations d_phy_rx_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = d_phy_rx_cdev_llseek
    , .open    = d_phy_rx_cdev_open
    , .release = d_phy_rx_cdev_release
    , .unlocked_ioctl = d_phy_rx_cdev_ioctl
    , .read    = d_phy_rx_cdev_read
    , .write   = d_phy_rx_cdev_write
    , .mmap    = d_phy_rx_cdev_mmap
};

static int __init
d_phy_rx_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&d_phy_rx_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __d_phy_rx_cdev = cdev_alloc();
    if ( !__d_phy_rx_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __d_phy_rx_cdev, &d_phy_rx_cdev_fops );
    if ( cdev_add( __d_phy_rx_cdev, d_phy_rx_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return -ENOMEM;
    }

    __d_phy_rx_class = class_create( THIS_MODULE, MODNAME );
    if ( !__d_phy_rx_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return -ENOMEM;
    }
    __d_phy_rx_class->dev_uevent = d_phy_rx_dev_uevent;

    // make_nod /dev/adc_fifo
    if ( !device_create( __d_phy_rx_class, NULL, d_phy_rx_dev_t, NULL, MODNAME "%d", MINOR( d_phy_rx_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return -ENOMEM;
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &d_phy_rx_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
d_phy_rx_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __d_phy_rx_class, d_phy_rx_dev_t ); // device_creeate

    class_destroy( __d_phy_rx_class ); // class_create

    cdev_del( __d_phy_rx_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( d_phy_rx_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    struct mipi_d_phy_rx_driver * drv = dev_id ? platform_get_drvdata( dev_id ) : 0;
    (void)drv;
    dev_info( &__pdev->dev, "d_phy_rx handle_interrupt\n" );
    return IRQ_HANDLED;
}

static int
d_phy_rx_module_probe( struct platform_device * pdev )
{
    int irq = 0;
    struct mipi_d_phy_rx_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct mipi_d_phy_rx_driver ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    __pdev = pdev;
    dev_info( &pdev->dev, "d_phy_rx_module probed" );

	drv->iomem = devm_platform_ioremap_resource(pdev, 0);
	if ( IS_ERR( drv->iomem ) ) {
        dev_err(&pdev->dev, "iomem get failed");
		return PTR_ERR(drv->iomem);
    }
    dev_info( &pdev->dev, "d_phy_rx probe resource: %x -- %x, map to %p\n"
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
d_phy_rx_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be freed\n", pdev->name, irqNumber, pdev->resource->start );
        free_irq( irqNumber, &pdev->dev );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __d_phy_rx_module_id [] = {
    { .compatible = "xlnx,MIPI-D-PHY-RX-1.3" }
    , {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __d_phy_rx_module_id )
        ,
    }
    , .probe = d_phy_rx_module_probe
    , .remove = d_phy_rx_module_remove
};

module_init( d_phy_rx_module_init );
module_exit( d_phy_rx_module_exit );
