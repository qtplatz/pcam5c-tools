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

#include "ov5640-gpio.h"
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
MODULE_PARM_DESC(devname, "ov5640 param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static int __debug_level__ = 0;
static struct platform_driver __platform_driver;
static dev_t ov5640_dev_t = 0;
static struct cdev * __ov5640_cdev;
static struct class * __ov5640_class;
static struct platform_device *__pdev;

struct ov5640_driver {
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
	bool streaming;
	bool enable_active_lanes;
	bool en_vcx;
};

typedef struct ov5640_cdev_private {
    u32 node;
    u32 size;
} ov5640_cdev_private;

static int
ov5640_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int
ov5640_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "debug level = %d\n", __debug_level__ );
    return 0;
}

static ssize_t
ov5640_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
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
ov5640_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, ov5640_proc_read, NULL );
}

static const struct proc_ops ov5640_proc_file_fops = {
    .proc_open    = ov5640_proc_open,
    .proc_read    = seq_read,
    .proc_write   = ov5640_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/////////////////////////////////
/////////////////////////////////

static int ov5640_cdev_open(struct inode *inode, struct file *file)
{
    struct ov5640_cdev_private *
        private_data = devm_kzalloc( &__pdev->dev
                                     , sizeof( struct ov5640_cdev_private )
                                     , GFP_KERNEL );
    if ( private_data ) {
        private_data->node =  MINOR( inode->i_rdev );
        private_data->size = 64 * 4;
        file->private_data = private_data;
    }

    return 0;

}

static int
ov5640_cdev_release( struct inode *inode, struct file *file )
{
    if ( file->private_data ) {
        devm_kfree( &__pdev->dev, file->private_data );
    }
    return 0;
}

static long ov5640_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t ov5640_cdev_read( struct file * file, char __user* data, size_t size, loff_t* f_pos )
{
    dev_info( &__pdev->dev, "ov5640_cdev_read: fpos=%llx, size=%ud\n", *f_pos, size );
    return size;
}

static ssize_t
ov5640_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    dev_info( &__pdev->dev, "ov5640_cdev_write: fpos=%llx, size=%ud\n", *f_pos, size );
    return size;
}

static ssize_t
ov5640_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
    int ret = (-1);
    return ret;
}

loff_t
ov5640_cdev_llseek( struct file * file, loff_t offset, int orig )
{
    // struct ov5640_cdev_reader * reader = file->private_data;
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

static struct file_operations ov5640_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = ov5640_cdev_llseek
    , .open    = ov5640_cdev_open
    , .release = ov5640_cdev_release
    , .unlocked_ioctl = ov5640_cdev_ioctl
    , .read    = ov5640_cdev_read
    , .write   = ov5640_cdev_write
    , .mmap    = ov5640_cdev_mmap
};

static int __init
ov5640_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&ov5640_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __ov5640_cdev = cdev_alloc();
    if ( !__ov5640_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __ov5640_cdev, &ov5640_cdev_fops );
    if ( cdev_add( __ov5640_cdev, ov5640_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return -ENOMEM;
    }

    __ov5640_class = class_create( THIS_MODULE, MODNAME );
    if ( !__ov5640_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return -ENOMEM;
    }
    __ov5640_class->dev_uevent = ov5640_dev_uevent;

    // make_nod /dev/adc_fifo
    if ( !device_create( __ov5640_class, NULL, ov5640_dev_t, NULL, MODNAME "%d", MINOR( ov5640_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return -ENOMEM;
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &ov5640_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
ov5640_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __ov5640_class, ov5640_dev_t ); // device_creeate

    class_destroy( __ov5640_class ); // class_create

    cdev_del( __ov5640_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( ov5640_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static int
ov5640_module_probe( struct platform_device * pdev )
{
    struct ov5640_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct ov5640_driver ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    __pdev = pdev;
    dev_info( &pdev->dev, "probed" );

	drv->rst_gpio = devm_gpiod_get_optional(&pdev->dev, "reset", GPIOD_OUT_HIGH);
	if ( IS_ERR( drv->rst_gpio ) ) {
		if ( PTR_ERR( drv->rst_gpio ) != -EPROBE_DEFER )
			dev_err(&pdev->dev, "Video Reset GPIO not setup in DT");
		return PTR_ERR( drv->rst_gpio );
	}

    dev_info(&pdev->dev, "Video Reset GPIO:%p", drv->rst_gpio );

    if ( drv->rst_gpio == 0 ) {
        drv->rst_gpio = devm_gpiod_get_optional(&pdev->dev, "pwdn", GPIOD_OUT_HIGH);
        if ( IS_ERR( drv->rst_gpio ) ) {
            if ( PTR_ERR( drv->rst_gpio ) != -EPROBE_DEFER )
                dev_err(&pdev->dev, "Video Reset GPIO not setup in DT");
            return PTR_ERR( drv->rst_gpio );
        }
    }
    dev_info(&pdev->dev, "Video Reset GPIO:%p", drv->rst_gpio );

    sema_init( &drv->sem, 1 );
    platform_set_drvdata( pdev, drv );

    return 0;
}

static int
ov5640_module_remove( struct platform_device * pdev )
{
    __pdev = 0;
    return 0;
}

static const struct of_device_id __ov5640_module_id [] = {
	{ .compatible = "ovti,ov5640-gpio" }
    , {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __ov5640_module_id )
        ,
    }
    , .probe = ov5640_module_probe
    , .remove = ov5640_module_remove
};

module_init( ov5640_module_init );
module_exit( ov5640_module_exit );
