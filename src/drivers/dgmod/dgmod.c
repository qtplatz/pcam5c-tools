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

#include "dgmod.h"
#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
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
#include <linux/ktime.h>

static char *devname = MODNAME;

MODULE_AUTHOR( "Toshinobu Hondo" );
MODULE_DESCRIPTION( "Driver for the Delay Pulse Generator" );
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(devname, "dgmod param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static int __debug_level__ = 0;
static struct platform_driver __platform_driver;
static dev_t dgmod_dev_t = 0;
static struct cdev * __dgmod_cdev;
static struct class * __dgmod_class;

struct dgmod_driver {
    uint32_t irq;
    const char * label;
    uint64_t irqCount_;
    void __iomem * reg_csr;
    void __iomem * reg_descriptor;
    void __iomem * reg_response;   // this is once time access only allowed (destructive read)
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

static int
dgmod_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "debug level = %d\n", __debug_level__ );
    return 0;
}

static ssize_t
dgmod_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
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
dgmod_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, dgmod_proc_read, NULL );
}

static const struct proc_ops dgmod_proc_file_fops = {
    .proc_open    = dgmod_proc_open,
    .proc_read    = seq_read,
    .proc_write   = dgmod_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/////////////////////////////////
/////////////////////////////////

static int dgmod_cdev_open(struct inode *inode, struct file *file)
{
    return 0;

}

static int
dgmod_cdev_release( struct inode *inode, struct file *file )
{
    return 0;
}

static long dgmod_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t dgmod_cdev_read( struct file * file, char __user *data, size_t size, loff_t *f_pos )
{
    return size;
}

static ssize_t
dgmod_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    return size;
}

static ssize_t
dgmod_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
    // const unsigned long length = vma->vm_end - vma->vm_start;
    return 0; // Success
}

loff_t
dgmod_cdev_llseek( struct file * file, loff_t offset, int orig )
{
	// struct dgmod_cdev_reader * reader = file->private_data;
    return file->f_pos;
}

static struct file_operations dgmod_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = dgmod_cdev_llseek
    , .open    = dgmod_cdev_open
    , .release = dgmod_cdev_release
    , .unlocked_ioctl = dgmod_cdev_ioctl
    , .read    = dgmod_cdev_read
    , .write   = dgmod_cdev_write
    , .mmap    = dgmod_cdev_mmap
};

static int __init
dgmod_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&dgmod_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __dgmod_cdev = cdev_alloc();
    if ( !__dgmod_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __dgmod_cdev, &dgmod_cdev_fops );
    if ( cdev_add( __dgmod_cdev, dgmod_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return -ENOMEM;
    }

    __dgmod_class = class_create( THIS_MODULE, MODNAME );
    if ( !__dgmod_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return -ENOMEM;
    }

    // make_nod /dev/adc_fifo
    if ( !device_create( __dgmod_class, NULL, dgmod_dev_t, NULL, MODNAME "%d", MINOR( dgmod_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return -ENOMEM;
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &dgmod_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
dgmod_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __dgmod_class, dgmod_dev_t ); // device_creeate

    class_destroy( __dgmod_class ); // class_create

    cdev_del( __dgmod_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( dgmod_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    //struct dgmod_driver * drv = dev_id ? platform_get_drvdata( dev_id ) : 0;
    return IRQ_HANDLED;
}

static int
dgmod_module_probe( struct platform_device * pdev )
{
    int irq = 0;
    struct dgmod_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct dgmod_driver ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    dev_info( &pdev->dev, "dgmod_module proved [%s]", pdev->name );

    for ( int i = 0; i < pdev->num_resources; ++i ) {
        struct resource * res = platform_get_resource( pdev, IORESOURCE_MEM, i );
        if ( res ) {
            void __iomem * regs = devm_ioremap_resource( &pdev->dev, res );
            /* if ( regs && i == 0 ) // check reg_names[i] */
            /*     drv->reg_csr = regs; */
            /* else if ( regs && i == 1 ) */
            /*     drv->reg_descriptor = regs; */
            /* else if ( regs && i == 2 ) */
            /*     drv->reg_response = regs; */
            dev_info( &pdev->dev, "dgmod probe resource[%d]: %x -- %x, map to %p\n", i, res->start, res->end, regs );
        }
    }
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
dgmod_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be freed\n", pdev->name, irqNumber, pdev->resource->start );
        free_irq( irqNumber, 0 );
    }
    return 0;
}

static const struct of_device_id __dgmod_module_id [] = {
    { .compatible = "xlnx,delaypulse-generator-1.0" },
    {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __dgmod_module_id )
        ,
    }
    , .probe = dgmod_module_probe
    , .remove = dgmod_module_remove
};

module_init( dgmod_module_init );
module_exit( dgmod_module_exit );
