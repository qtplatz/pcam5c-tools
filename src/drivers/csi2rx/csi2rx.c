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

#include "csi2rx.h"
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
MODULE_PARM_DESC(devname, "csi2rx param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

/*
 * Sink pad connected to sensor source pad.
 * Source pad connected to next module like demosaic.
 */
#define XCSI_MEDIA_PADS		2
#define XCSI_DEFAULT_WIDTH	1920
#define XCSI_DEFAULT_HEIGHT	1080

/* MIPI CSI-2 Data Types from spec */
#define XCSI_DT_YUV4208B	0x18
#define XCSI_DT_YUV4228B	0x1e
#define XCSI_DT_YUV42210B	0x1f
#define XCSI_DT_RGB444		0x20
#define XCSI_DT_RGB555		0x21
#define XCSI_DT_RGB565		0x22
#define XCSI_DT_RGB666		0x23
#define XCSI_DT_RGB888		0x24
#define XCSI_DT_RAW6		0x28
#define XCSI_DT_RAW7		0x29
#define XCSI_DT_RAW8		0x2a
#define XCSI_DT_RAW10		0x2b
#define XCSI_DT_RAW12		0x2c
#define XCSI_DT_RAW14		0x2d
#define XCSI_DT_RAW16		0x2e
#define XCSI_DT_RAW20		0x2f

#define XCSI_VCX_START		4
#define XCSI_MAX_VC		4
#define XCSI_MAX_VCX		16

#define XCSI_NEXTREG_OFFSET	4

/* There are 2 events frame sync and frame level error per VC */
#define XCSI_VCX_NUM_EVENTS	((XCSI_MAX_VCX - XCSI_MAX_VC) * 2)

static int __debug_level__ = 0;
static struct platform_driver __platform_driver;
static dev_t csi2rx_dev_t = 0;
static struct cdev * __csi2rx_cdev;
static struct class * __csi2rx_class;
static struct platform_device *__pdev;

struct xcsi2rxss_state {
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

static const struct clk_bulk_data xcsi2rxss_clks[] = {
	{ .id = "lite_aclk" },
	{ .id = "video_aclk" },
};

typedef struct csi2rx_cdev_private {
    u32 node;
    u32 size;
} csi2rx_cdev_private;

struct core_register {
    u32 offset;
    const char * name;
};
const struct core_register __core_register [] = {
    { 0x00, "Core Configuration Register" }
    , { 0x04, "Protocol Configuration Register" }
    , { 0x10, "Core Status Register" }
    , { 0x20, "Global Interrupt Enable Register" }
    , { 0x24, "Interrupt Status Register" }
    , { 0x28, "Interrupt Enable Register" }
    , { 0x30, "Generic Short Packet Register" }
    , { 0x34, "VCX Frame Error Register" }
    , { 0x3c, "Clock Lane Information Register" }
    , { 0x40, "Lane0 Information" }
    , { 0x44, "Lane1 Information" }
    , { 0x48, "Lane2 Information" }
    , { 0x4c, "Lane3 Information" }
    // 0x60 -- 0xdc
};

/*
 * Register related operations
 */
static inline u32 xcsi2rxss_read(struct xcsi2rxss_state *xcsi2rxss, u32 addr)
{
	return ioread32(xcsi2rxss->iomem + addr);
}

static inline void xcsi2rxss_write(struct xcsi2rxss_state *xcsi2rxss, u32 addr,
				   u32 value)
{
	iowrite32(value, xcsi2rxss->iomem + addr);
}

static inline void xcsi2rxss_clr(struct xcsi2rxss_state *xcsi2rxss, u32 addr,
				 u32 clr)
{
	xcsi2rxss_write(xcsi2rxss, addr,
			xcsi2rxss_read(xcsi2rxss, addr) & ~clr);
}

static inline void xcsi2rxss_set(struct xcsi2rxss_state *xcsi2rxss, u32 addr, u32 set)
{
	xcsi2rxss_write(xcsi2rxss, addr, xcsi2rxss_read(xcsi2rxss, addr) | set);
}

static int
csi2rx_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int
csi2rx_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "debug level = %d\n", __debug_level__ );
    struct xcsi2rxss_state * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        seq_printf( m
                    , "csi2rx mem resource: %x -- %x, map to %p\n"
                    , __pdev->resource->start, __pdev->resource->end, drv->iomem );
        for ( size_t i = 0; i < countof( __core_register ); ++i ) {
            seq_printf( m, "0x04x:\t%08x\t%s\n"
                        , xcsi2rxss_read( drv, __core_register[ i ].offset ), __core_register[ i ].name );
        }

    }
    return 0;
}

static ssize_t
csi2rx_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
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
csi2rx_proc_open( struct inode * inode, struct file * file )
{
    printk( KERN_INFO "" MODNAME " proc_open private_data=%x\n", (u32)file->private_data );
    return single_open( file, csi2rx_proc_read, NULL );
}

static const struct proc_ops csi2rx_proc_file_fops = {
    .proc_open    = csi2rx_proc_open,
    .proc_read    = seq_read,
    .proc_write   = csi2rx_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/////////////////////////////////
/////////////////////////////////

static int csi2rx_cdev_open(struct inode *inode, struct file *file)
{
    struct csi2rx_cdev_private *
        private_data = devm_kzalloc( &__pdev->dev
                                     , sizeof( struct csi2rx_cdev_private )
                                     , GFP_KERNEL );
    if ( private_data ) {
        private_data->node =  MINOR( inode->i_rdev );
        private_data->size = 64 * 4;
        file->private_data = private_data;
    }

    return 0;

}

static int
csi2rx_cdev_release( struct inode *inode, struct file *file )
{
    if ( file->private_data ) {
        devm_kfree( &__pdev->dev, file->private_data );
    }
    return 0;
}

static long csi2rx_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t csi2rx_cdev_read( struct file * file, char __user* data, size_t size, loff_t* f_pos )
{
    dev_info( &__pdev->dev, "csi2rx_cdev_read: fpos=%llx, size=%ud\n", *f_pos, size );

    struct xcsi2rxss_state * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        csi2rx_cdev_private * private_data = file->private_data;
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
csi2rx_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    dev_info( &__pdev->dev, "csi2rx_cdev_write: fpos=%llx, size=%ud\n", *f_pos, size );
    return size;
}

static ssize_t
csi2rx_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
    int ret = (-1);
    return ret;
}

loff_t
csi2rx_cdev_llseek( struct file * file, loff_t offset, int orig )
{
    // struct csi2rx_cdev_reader * reader = file->private_data;
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

static struct file_operations csi2rx_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = csi2rx_cdev_llseek
    , .open    = csi2rx_cdev_open
    , .release = csi2rx_cdev_release
    , .unlocked_ioctl = csi2rx_cdev_ioctl
    , .read    = csi2rx_cdev_read
    , .write   = csi2rx_cdev_write
    , .mmap    = csi2rx_cdev_mmap
};

static int __init
csi2rx_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&csi2rx_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __csi2rx_cdev = cdev_alloc();
    if ( !__csi2rx_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __csi2rx_cdev, &csi2rx_cdev_fops );
    if ( cdev_add( __csi2rx_cdev, csi2rx_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return -ENOMEM;
    }

    __csi2rx_class = class_create( THIS_MODULE, MODNAME );
    if ( !__csi2rx_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return -ENOMEM;
    }
    __csi2rx_class->dev_uevent = csi2rx_dev_uevent;

    // make_nod /dev/adc_fifo
    if ( !device_create( __csi2rx_class, NULL, csi2rx_dev_t, NULL, MODNAME "%d", MINOR( csi2rx_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return -ENOMEM;
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &csi2rx_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
csi2rx_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __csi2rx_class, csi2rx_dev_t ); // device_creeate

    class_destroy( __csi2rx_class ); // class_create

    cdev_del( __csi2rx_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( csi2rx_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    struct xcsi2rxss_state * drv = dev_id ? platform_get_drvdata( dev_id ) : 0;
    (void)drv;
    dev_info( &__pdev->dev, "csi2rx handle_interrupt\n" );
    return IRQ_HANDLED;
}

static int
csi2rx_module_probe( struct platform_device * pdev )
{
    int irq = 0;
    struct xcsi2rxss_state * drv = devm_kzalloc( &pdev->dev, sizeof( struct xcsi2rxss_state ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    __pdev = pdev;
    dev_info( &pdev->dev, "csi2rx_module proved" );

	drv->rst_gpio = devm_gpiod_get_optional(&pdev->dev, "video-reset", GPIOD_OUT_HIGH);
	if ( IS_ERR( drv->rst_gpio ) ) {
		if ( PTR_ERR( drv->rst_gpio ) != -EPROBE_DEFER )
			dev_err(&pdev->dev, "Video Reset GPIO not setup in DT");
		return PTR_ERR( drv->rst_gpio );
	}
	/* ret = xcsi2rxss_parse_of(xcsi2rxss); */
	/* if (ret < 0) */
	/* 	return ret; */
	drv->iomem = devm_platform_ioremap_resource(pdev, 0);
	if ( IS_ERR( drv->iomem ) ) {
        dev_err(&pdev->dev, "iomem get failed");
		return PTR_ERR(drv->iomem);
    }
    dev_info( &pdev->dev, "csi2rx probe resource: %x -- %x, map to %p\n"
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
csi2rx_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be freed\n", pdev->name, irqNumber, pdev->resource->start );
        free_irq( irqNumber, 0 );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __csi2rx_module_id [] = {
    { .compatible = "xlnx,mipi-csi2-rx-subsystem-5.1" },
    {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __csi2rx_module_id )
        ,
    }
    , .probe = csi2rx_module_probe
    , .remove = csi2rx_module_remove
};

module_init( csi2rx_module_init );
module_exit( csi2rx_module_exit );
