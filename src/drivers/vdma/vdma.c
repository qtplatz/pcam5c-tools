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
#include <linux/miscdevice.h>
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

enum VDMA_OFFSET {
    OFFSET_VDMA_MM2S_CONTROL_REGISTER       = 0x00
    , OFFSET_VDMA_MM2S_STATUS_REGISTER        = 0x04
    , OFFSET_VDMA_MM2S_REG_INDEX              = 0x14
    , OFFSET_PARK_PTR_REG                     = 0x28
    , OFFSET_VERSION                          = 0x2c
    , OFFSET_VDMA_MM2S_VSIZE                  = 0x50
    , OFFSET_VDMA_MM2S_HSIZE                  = 0x54
    , OFFSET_VDMA_MM2S_FRMDLY_STRIDE          = 0x58
    , OFFSET_VDMA_MM2S_FRAMEBUFFER1           = 0x5c
    , OFFSET_VDMA_MM2S_FRAMEBUFFER2           = 0x60
    , OFFSET_VDMA_MM2S_FRAMEBUFFER3           = 0x64
    , OFFSET_VDMA_MM2S_FRAMEBUFFER4           = 0x68

    , OFFSET_VDMA_S2MM_CONTROL_REGISTER       = 0x30
    , OFFSET_VDMA_S2MM_STATUS_REGISTER        = 0x34
    , OFFSET_VDMA_S2MM_IRQ_MASK               = 0x3c
    , OFFSET_VDMA_S2MM_REG_INDEX              = 0x44
    , OFFSET_VDMA_S2MM_VSIZE                  = 0xa0
    , OFFSET_VDMA_S2MM_HSIZE                  = 0xa4
    , OFFSET_VDMA_S2MM_FRMDLY_STRIDE          = 0xa8
    , OFFSET_VDMA_S2MM_FRAMEBUFFER1           = 0xac
    , OFFSET_VDMA_S2MM_FRAMEBUFFER2           = 0xb0
    , OFFSET_VDMA_S2MM_FRAMEBUFFER3           = 0xb4
    , OFFSET_VDMA_S2MM_FRAMEBUFFER4           = 0xb8
};

static struct platform_driver __vdma_platform_driver;
struct platform_device * __pdev;

enum dma_ingress_egress {
    dma_size      = 0x00100000*5
};

struct vdma_driver {
    uint32_t irq[ 2 ];
    uint64_t irqCount;
    void __iomem * iomem;
    wait_queue_head_t queue;
    int queue_condition;
    struct semaphore sem;
    uint32_t wp;     // next effective dma phys addr
    uint32_t readp;  // fpga read phys addr
    dma_addr_t dma_handle[ 32 ];
    uint32_t * dma_vaddr[ 32 ];
};

struct vdma_cdev_reader {
    struct vdma_driver * drv;
    u32 minor;
    u32 size;
};

enum { dg_data_irq_mask = 2 };

static int __debug_level__ = 0;

struct register_bitfield {
    u32 lsb; u32 mask; const char * name;
};

struct register_bitfield irq_mask [] = {
    { 3, 0x01, "eol-late-err" }
    , { 2, 0x01, "sof-late-err" }
    , { 1, 0x01, "eol-early-err" }
    , { 0, 0x01, "sof-early-err" }
    , { 0, 0, 0 }
};

static void vdma_status_print( struct seq_file * m, u32 status )
{
    if (status & 0x0010)
        seq_printf(m, "vdma-internal-error;");
    if (status & 0x0020)
        seq_printf(m, "vdma-slave-error;");
    if (status & 0x0040)
        seq_printf(m, "vdma-decode-error;");
    if (status & 0x0080)
        seq_printf(m, "start-of-frame-early-error;");
    if (status & 0x0100)
        seq_printf(m, "end-of-line-early-error;");
    if (status & 0x0800)
        seq_printf(m, "start-of-frame-late-error;");
    if (status & 0x1000)
        seq_printf(m, "frame-count-interrupt;");
    if (status & 0x2000)
        seq_printf(m, "delay-count-interrupt;");
    if (status & 0x4000)
        seq_printf(m, "error-interrupt;");
    if (status & 0x8000)
        seq_printf(m, "end-of-line-late-error;");
    seq_printf(m, "frame-count:%d;", (status & 0x00ff0000) >> 16);
    seq_printf(m, "delay-count:%d;", (status & 0xff000000) >> 24);
    seq_printf(m, "%s", (status & 0x01) ? "halted" : "running" );
}

static void vdma_cr_print( struct seq_file * m, u32 value )
{
    seq_printf(m, "irq-delay-count:%d;", value >> 24 );
    seq_printf(m, "irq-frame-count:%d;", ( value & 0x00ff0000 ) >> 16 );
    if ( value & 0x00008000 )
        seq_printf(m, "repeat;");
    if ( value & 0x00004000 )
        seq_printf(m, "err-inq;");
    if ( value & 0x00002000 )
        seq_printf(m, "dlycnt-irq;");
    if ( value & 0x00001000 )
        seq_printf(m, "frmcnt-irqn;");
    seq_printf(m, "wrt-pntr-num:%d;", ( value >> 8 ) & 0x0f );
    if ( value & 0x00000080 )
        seq_printf(m, "gen-lock-src;" );
    if ( value & 0x00000010 )
        seq_printf(m, "frame-cnt;" );
    if ( value & 0x00000008 )
        seq_printf(m, "gen-lock;" );
    if ( value & 0x00000004 )
        seq_printf(m, "reset;" );
    if ( value & 0x00000002 )
        seq_printf(m, "circular-park;" );
    seq_printf(m, "%s", (value & 0x00) ? "stop" : "run" );
}

struct core_register {
    u32 offset;
    const char * name;
    u32 replicates;
    struct register_bitfield * fields;
    void (*outf)( struct seq_file * m, u32 status );
};

const struct core_register __core_register [] = {
    { OFFSET_VDMA_MM2S_CONTROL_REGISTER, "MM2S VDMA CR", 1, 0, vdma_cr_print }
    , { OFFSET_VDMA_MM2S_STATUS_REGISTER, "MM2S VDMA SR", 1, 0, vdma_status_print }
    // , { 0x14, "MM2S Register Index", 1, 0 }
    , { OFFSET_PARK_PTR_REG, "PARK_PTR_REG", 1, 0 }
    // , { 0x2c, "Video DMA Version register", 1 }
    , { OFFSET_VDMA_S2MM_CONTROL_REGISTER, "S2MM VDMA CR", 1, 0, vdma_cr_print }
    , { OFFSET_VDMA_S2MM_STATUS_REGISTER, "S2MM VDMA SR", 1, 0, vdma_status_print }
    , { OFFSET_VDMA_S2MM_IRQ_MASK, "S2MM_VDMA_IRQ_MASK", 1, irq_mask, 0 }
    // , { 0x44, "S2MM Register Index", 1, 0 }
    , { OFFSET_VDMA_MM2S_VSIZE, "MM2S_VSIZE MM2S", 1, 0 }
    , { OFFSET_VDMA_MM2S_HSIZE, "MM2S_HSIZE MM2S", 1, 0 }
    , { OFFSET_VDMA_MM2S_FRMDLY_STRIDE, "MM2S_FRMDLY_STRIDE", 1, 0 }
    // , { 0x5c, "MM2S Start Address", 1 }
    , { OFFSET_VDMA_S2MM_VSIZE, "VDMA_S2MM_VSIZE", 1, 0 }
    , { OFFSET_VDMA_S2MM_HSIZE, "VDMA_S2MM_HSIZE", 1, 0 }
    , { OFFSET_VDMA_S2MM_FRMDLY_STRIDE, "VDMA_S2MM_FRMDLY_STRIDE", 1, 0 }
    // , { 0xac, "S2MM Start Address", 16 }
};

static inline u32 vdma_read32(struct vdma_driver * drv, u32 addr)
{
	return ioread32(drv->iomem + addr);
}

static inline void vdma_write32(struct vdma_driver * drv, u32 addr, u32 value)
{
	iowrite32(value, drv->iomem + addr);
}

static void vdma_reset( struct vdma_driver * drv )
{
    dev_info( &__pdev->dev, "%s\n", __func__ );
    ((u32*)drv->iomem)[ 0 ] = 0x04;
    ((u32*)drv->iomem)[ 0x30 / sizeof(u32) ] = 0x04;
}

static bool vdma_reset_busy( struct vdma_driver * drv )
{
    return ( ( ((u32*)drv->iomem)[ 0x00 / sizeof(u32) ] & 0x04 ) == 0x04 ) |
        ( ( ((u32*)drv->iomem)[ 0x30 / sizeof(u32) ] & 0x04 ) == 0x04 );
}

static void vdma_start_triple_buffering( struct vdma_driver * drv )
{
    dev_info( &__pdev->dev, "%s\n", __func__ );
    vdma_reset( drv );
    while ( vdma_reset_busy( drv ) )
        ;
    int interrupt_frame_count = 3;
    u32* mm2scr = (u32*)drv->iomem + (0x00/sizeof(u32));
    u32* mm2ssr = (u32*)drv->iomem + (0x04/sizeof(u32));
    u32* s2mmcr = (u32*)drv->iomem + (0x30/sizeof(u32));
    u32* s2mmsr = (u32*)drv->iomem + (0x34/sizeof(u32));

    *s2mmsr = 0; // clear SR
    *mm2ssr = 0; // clear SR
    ((u32*)drv->iomem)[ 0x3c / sizeof(u32) ] = 0x0f; // do not mask interrupts
#if 0 // move down
    *s2mmcr =
        (interrupt_frame_count << 16) |
        0x0080 | // VDMA_CONTROL_REGISTER_INTERNAL_GENLOCK |
        0x0008 | // VDMA_CONTROL_REGISTER_GENLOCK_ENABLE |
        0x0002 | // VDMA_CONTROL_REGISTER_CIRCULAR_PARK);
        0x0001 ; // Run/Stop  (1 = RUN)

    *mm2scr =
        (interrupt_frame_count << 16) |
        0x0080 | // VDMA_CONTROL_REGISTER_INTERNAL_GENLOCK |
        0x0008 | // VDMA_CONTROL_REGISTER_GENLOCK_ENABLE |
        0x0002 | // VDMA_CONTROL_REGISTER_CIRCULAR_PARK);
        0x0001 ; // Run/Stop  (1 = RUN)
#endif
    vdma_write32( drv, 0x14, 0 ); // MM2S Index -> 0
    vdma_write32( drv, 0x44, 0 ); // S2MM Index -> 0

    for ( int i = 0; i < 4; ++ i ) {
        ((u32*)drv->iomem)[ i + 0x5c / sizeof(u32) ] = drv->dma_handle[ i ];
        ((u32*)drv->iomem)[ i + 0xac / sizeof(u32) ] = drv->dma_handle[ i ];
    }
    ((u32*)drv->iomem)[ OFFSET_PARK_PTR_REG / sizeof(u32) ] = 0; // parc ptr_reg

    // int
    // vdma_setup(vdma_handle *handle, unsigned int baseAddr,
    // int width, int height, int pixelLength, unsigned int fb1Addr, unsigned int fb2Addr, unsigned int fb3Addr) {
    vdma_write32(drv, OFFSET_VDMA_S2MM_FRMDLY_STRIDE, 1920 * 4 );
    vdma_write32(drv, OFFSET_VDMA_MM2S_FRMDLY_STRIDE, 1920 * 4 );

    // Write horizontal size (bytes)
    vdma_write32(drv, OFFSET_VDMA_S2MM_HSIZE, 1920 * 4 );
    vdma_write32(drv, OFFSET_VDMA_MM2S_HSIZE, 1920 * 4 );

    // Write vertical size (lines), this actually starts the transfer
    vdma_write32(drv, OFFSET_VDMA_S2MM_VSIZE, 1080 );
    vdma_write32(drv, OFFSET_VDMA_MM2S_VSIZE, 1080 );

    *s2mmcr =
        (interrupt_frame_count << 16) |
        0x0080 | // VDMA_CONTROL_REGISTER_INTERNAL_GENLOCK |
        0x0008 | // VDMA_CONTROL_REGISTER_GENLOCK_ENABLE |
        0x0002 | // VDMA_CONTROL_REGISTER_CIRCULAR_PARK);
        0x0001 ; // Run/Stop  (1 = RUN)

    *mm2scr =
        (interrupt_frame_count << 16) |
        0x0080 | // VDMA_CONTROL_REGISTER_INTERNAL_GENLOCK |
        0x0008 | // VDMA_CONTROL_REGISTER_GENLOCK_ENABLE |
        0x0002 | // VDMA_CONTROL_REGISTER_CIRCULAR_PARK);
        0x0001 ; // Run/Stop  (1 = RUN)

    int counter = 1000;
    while ( counter-- && ( ((*s2mmcr) & 01) == 0 || ((*s2mmsr) & 01) == 1 ) ) {
        dev_info( &__pdev->dev, "%s -- waiting for VDMA to start running CR=%x,SR=%x; %d.\n", __func__, *s2mmcr, *s2mmsr, counter );
    }
    dev_info( &__pdev->dev, "%s -- done VDMA to start running CR=%x,SR=%x; %d.\n", __func__, *s2mmcr, *s2mmsr, counter );
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    dev_info( &__pdev->dev, "handle_interrupt\n" );
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
        seq_printf( m, "vdma mem resource: %x -- %x, map to %p\n"
                    , __pdev->resource->start, __pdev->resource->end, drv->iomem );
        u32 version = vdma_read32( drv, 0x2c );
        seq_printf( m, "VDMA Version: %d.%02x.%d-%d\n"
                    , version >> 28
                    , (version >> 20) & 0xff
                    , (version >> 16) & 0x0f
                    , (version & 0xffff) );

        for ( size_t i = 0; i < countof( __core_register ); ++i ) {
            u32 offset = __core_register[ i ].offset;
            u32 r = vdma_read32( drv, offset );
            seq_printf( m, "0x%04x\t%04x'%04x\t%s\t", offset, r >> 16, r & 0xffff, __core_register[ i ].name );
            if ( __core_register[ i ].outf ) {
                (*__core_register[ i ].outf)(m, r);
            } else if ( __core_register[ i ].fields ) {
                struct register_bitfield * fields = __core_register[ i ].fields;
                while ( fields->mask != 0 ) {
                    seq_printf( m, "%s=%x,", fields->name, (r >> fields->lsb)&fields->mask);
                    ++fields;
                }
            }
            seq_printf(m, "\n" );
        }
        vdma_write32( drv, OFFSET_VDMA_S2MM_REG_INDEX, 0 ); // S2MM Index -> 0
        seq_printf( m, "S2MM: " );
        for ( size_t k = 0; k < 8; ++k ) {
            seq_printf( m, "[%02x] 0x%08x\t", k, vdma_read32( drv, OFFSET_VDMA_S2MM_FRAMEBUFFER1 + (k * sizeof( u32 )) ) );
        }
        seq_printf( m, "\nMM2S: " );
        for ( size_t k = 0; k < 8; ++k ) {
            seq_printf( m, "[%02x] 0x%08x\t", k, vdma_read32( drv, OFFSET_VDMA_MM2S_FRAMEBUFFER1 + (k * sizeof( u32 )) ) );
        }
        seq_printf( m, "\nDMA pages\n" );
        for ( size_t i = 0; i < countof( drv->dma_vaddr ) && drv->dma_vaddr[ i ]; ++i ) {
            seq_printf( m, "[%02d] 0x%08x\t", i, drv->dma_handle[ i ] );
            if ( ( i + 1 ) % 8 == 0 )
                seq_printf( m, "\n" );
        }
    }
    return 0;
}

static ssize_t
vdma_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    static char readbuf[256];
    size = ( size >= sizeof( readbuf )) ? (sizeof( readbuf ) - 1) : size;
    struct vdma_driver * drv = platform_get_drvdata( __pdev );
    if ( copy_from_user( readbuf, user, size ) || drv == 0 )
        return -EFAULT;

    readbuf[ size ] = '\0';
    dev_info( &__pdev->dev, "proc_write = %s\n", readbuf );

    if ( strcmp( readbuf, "halt" ) == 0 ) {
        vdma_reset( drv );
    } else if ( strcmp( readbuf, "go" ) == 0 ) {
        vdma_start_triple_buffering( drv );
    } else if ( strncmp( readbuf, "vsize", 5 ) == 0 ) {
        unsigned long vsize;
        if ( kstrtoul( &readbuf[5], 0, &vsize ) == 0 ) {
            dev_info( &__pdev->dev, "vsize=%lu\n", vsize );
            // todo: set value to hardware
        }
    } else if ( strncmp( readbuf, "hsize", 5 ) == 0 ) {
        unsigned long hsize;
        if ( kstrtoul( &readbuf[5], 0, &hsize ) == 0 ) {
            dev_info( &__pdev->dev, "hsize=%lu\n", hsize );
            // todo: set value to hardware
        }
    }
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
    unsigned int minor = MINOR( inode->i_rdev );
    struct vdma_cdev_reader * reader = devm_kzalloc( &__pdev->dev, sizeof( struct vdma_cdev_reader ), GFP_KERNEL );
    reader->drv = platform_get_drvdata( __pdev );
    reader->minor = minor;
    file->private_data = reader;
    if ( minor == 0 ) {
        reader->size = __pdev->resource->end - __pdev->resource->start + 1;
    }

    dev_info( &__pdev->dev, "vdma_cdev_open minor=%d\n", minor );

    return 0;
}

static int
vdma_cdev_release( struct inode *inode, struct file *file )
{
    unsigned int minor = MINOR( inode->i_rdev );
    dev_info( &__pdev->dev, "vdma_cdev_release node=%d\n", minor );

    if ( file->private_data )
        devm_kfree( &__pdev->dev, file->private_data );

    return 0;
}

static long vdma_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t vdma_cdev_read( struct file * file, char __user *data, size_t size, loff_t *f_pos )
{
    struct vdma_cdev_reader * reader = 0;
    if (( reader = file->private_data )) {
        dev_info( &__pdev->dev, "vdma_cdev_read: fpos=%llx, size=%ud\n", *f_pos, size );
        if ( reader->minor == 0 ) {
            size_t dsize = reader->size - *f_pos;
            if ( dsize > size )
                dsize = size;
            if ( copy_to_user( data, (const char *)reader->drv->iomem + (*f_pos), dsize ) ) {
                return -EFAULT;
            }
            *f_pos += dsize;
            return dsize;
        }
    }
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

static struct miscdevice vdma_miscdevice [] = {
    {
        .minor = MISC_DYNAMIC_MINOR
        , .name = "vdma0"
        , .fops = &vdma_cdev_fops
        , .mode = 0666
    }
    , {
        .minor = MISC_DYNAMIC_MINOR
        , .name = "vdma1"
        , .fops = &vdma_cdev_fops
        , .mode = 0666
    }
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

    for ( u32 i = 0; i < countof( drv->dma_vaddr ); ++i ) {
        if (( drv->dma_vaddr[ i ] = dma_alloc_coherent( &pdev->dev, dma_size, &drv->dma_handle[ i ], GFP_KERNEL ) )) {
            // dev_info( &pdev->dev, "vdma dma alloc_coherent %p\t%x\n", drv->dma_vaddr[ i ], drv->dma_handle[ i ] );
        } else {
            dev_err( &pdev->dev, "failed vdma dma alloc_coherent %p\n", drv->dma_vaddr );
            break;
        }
    }

    for ( int i = 0; i < countof( vdma_miscdevice ); ++i ) {
        misc_register( &vdma_miscdevice[i] );
        dev_info( &pdev->dev, "minor=%d\n", vdma_miscdevice[i].minor );
    }

    platform_set_drvdata( pdev, drv );

    int i = 0;
    while (( ( irq = platform_get_irq( pdev, i++ ) ) > 0 ) && ( i < countof( drv->irq ) )) {
        dev_info( &pdev->dev, "platform_get_irq: %d", irq );
        if ( devm_request_irq( &pdev->dev, irq, handle_interrupt, 0, MODNAME, pdev ) == 0 ) {
            drv->irq[ i - 1 ] = irq;
        } else {
            dev_err( &pdev->dev, "Failed to register IRQ.\n" );
            return -ENODEV;
        }
    }
    vdma_start_triple_buffering( drv );
    return 0;
}

static int
vdma_module_remove( struct platform_device * pdev )
{
    struct vdma_driver * drv = platform_get_drvdata( pdev );

    for ( int i = 0; (i < countof( drv->irq )) && (drv->irq[ i ] > 0); ++i ) {
        dev_info( &pdev->dev, "IRQ %d about to be freed\n", drv->irq[ i ] );
        free_irq( drv->irq[ i ], &pdev->dev );
    }
    for ( u32 i = 0; i < countof( drv->dma_vaddr ) && drv->dma_vaddr[ i ]; ++i ) {
        dma_free_coherent( &pdev->dev, dma_size, drv->dma_vaddr[ i ], drv->dma_handle[ i ] );
        // dev_info( &pdev->dev, "dma addr %x about to be freed\n", drv->dma_handle[ i ] );
    }

    for ( int i = 0; i < countof( vdma_miscdevice ); ++i )
        misc_deregister( &vdma_miscdevice[i] );

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
