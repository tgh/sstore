/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.c - the most useful device driver ever! 
 */                                                    

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>  /* for module_param(), so that the module can
                                 * take arguments from the command line when
                                 * using insmod */
#include <linux/fs.h>           /* for struct file, struct file_operations,
                                 * register_chrdev_region(),
                                 * alloc_chrdev_region() */
#include <linux/types.h>        /* for dev_t (represents device numbers),
                                 * ssize_t, size_t, loff_t types */
#include <linux/kdev_t.h>       /* for MKDEV(), MAJOR(), and MINOR() macros */
#include <linux/cdev.h>         /* for struct cdev */
#include "sstore.h"             /* SSTORE_MAJOR, SSTORE_DEVICE_COUNT
                                   struct sstore, struct blob */


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tyler Hayes");

struct sstore * sstore_dev_array;

//---------------------------------------------------------------------------

/*
 * INIT
 */
static int sstore_init(void)
{
    int result = 0;
    int i = 0;      //your standard for loop variable
    dev_t device_num = MKDEV(SSTORE_MAJOR, 0);

    printk(KERN_ALERT "In _init\n");

    if (SSTORE_MAJOR) {
	    result = register_chrdev_region(device_num, SSTORE_DEVICE_COUNT, 
								                                "sstore");
    } else {
        result = alloc_chrdev_region(&device_num, SSTORE_MAJOR,
						SSTORE_DEVICE_COUNT, "sstore");
        SSTORE_MAJOR = MAJOR(device_num);
    } 
    if (result < 0) {
		printk(KERN_ALERT "Major number %d not found: sstore", 
								SSTORE_MAJOR);
		return result;
	}

    sstore_dev_array = kmalloc(SSTORE_DEVICE_COUNT * sizeof(struct sstore),
                               GFP_KERNEL);
    if (!sstore_dev_array) {
        unregister_chrdev_region(device_num, SSTORE_DEVICE_COUNT);
        return -ENOMEM;
    }
    memset(sstore_dev_array, 0, SSTORE_DEVICE_COUNT * sizeof (struct sstore));

    for (i = 0; i < SSTORE_DEVICE_COUNT; ++i) {
        
    }

	return 0;
}

//---------------------------------------------------------------------------

/*
 * OPEN
 */
int sstore_open(struct inode * i_node, struct file * file)
{
	return 0;
}

//---------------------------------------------------------------------------

/*
 * LLSEEK
 */
loff_t sstore_llseek(struct file * file, loff_t offset, int i)
{
	return offset;
}

//---------------------------------------------------------------------------

/*
 * READ
 */
ssize_t sstore_read(struct file * file, char __user * user, size_t size,
							loff_t * offset)
{
	return size;
}

//---------------------------------------------------------------------------

/*
 * WRITE
 */
ssize_t sstore_write(struct file * file, const char __user * user,
					size_t size, loff_t * offset)
{
	return size;
}

//---------------------------------------------------------------------------

/*
 * IOCTL
 */
int sstore_ioctl(struct inode * i_node, struct file * file, unsigned int ui,
							unsigned long ul)
{
	return 0;
}

//---------------------------------------------------------------------------

/*
 * RELEASE
 */
int sstore_release(struct inode * i_node, struct file * file)
{
	return 0;
}

//---------------------------------------------------------------------------

/*
 * EXIT
 */
static void sstore_exit(void)
{
    dev_t device_num = MKDEV(SSTORE_MAJOR, 0);

	printk(KERN_ALERT "In _exit\n");

    unregister_chrdev_region(device_num, SSTORE_DEVICE_COUNT);
}

//---------------------------------------------------------------------------

/*
 * FOPS (file operations). This struct is a collection of function pointers
 * that point to a char driver's methods.
 */
struct file_operations sstore_fops = {
        .owner = THIS_MODULE,
        .llseek = sstore_llseek,
        .read = sstore_read,
        .write = sstore_write,
        .ioctl = sstore_ioctl,
        .open = sstore_open,
        .release = sstore_release
};

//tells kernel which functions run when driver is loaded/removed
module_init(sstore_init);
module_exit(sstore_exit);

/*
 * Module Parameters
 */
static unsigned long max_blobs = 1;
static unsigned long max_size = 1024;
module_param(max_blobs, ulong, S_IRUGO);
module_param(max_size, ulong, S_IRUGO);
