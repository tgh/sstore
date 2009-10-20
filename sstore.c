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

/*
 * Function decalarations
 */
static void sstore_cleanup_and_exit(void);


/*
 * Module Parameters
 */
static unsigned long max_blobs = 1;
static unsigned long max_size = 1024;

module_param(max_blobs, ulong, S_IRUGO);
module_param(max_size, ulong, S_IRUGO);

/*
 * Global variables
 */
int sstore_major = SSTORE_MAJOR;
int sstore_minor = SSTORE_MINOR;
struct sstore * sstore_dev_array;


//---------------------------------------------------------------------------

/*
 * INIT
 */
static int sstore_init(void)
{
    int result = 0;         //the return status of this function
    int i = 0;              //your standard for-loop variable
    dev_t device_num = 0;   //the device number (holds major and minor number)

    //DEBUG OUTPUT
    printk(KERN_DEBUG "In _init\n");

    /*
     * Get a range of minor numbers and register a region for devices.
     * 
     * When sstore_major is not 0 (when it is explicitly given a number by a
     * programmer), the sstore_major and sstore_minor numbers are given to the
     * MKDEV macro:
     * MKDEV(ma,mi)	((ma)<<8 | (mi)), which stores these numbers into a 32-bit
     * unsigned integer type (device_num) by dividing the bits up into a major
     * number section and a minor number section.  device_num is then used to
     * register the device with register_chrdev_region().  Otherwise,
     * the kernel is used to get a number dynamically by sending the address
     * of the device_num variable through the alloc_chrdev_region() function
     * in the else clause. 
     */
    if (sstore_major) {
        device_num = MKDEV(sstore_major, sstore_minor);
	    result = register_chrdev_region(device_num, SSTORE_DEVICE_COUNT, 
								                                "sstore");
    } else {
        result = alloc_chrdev_region(&device_num, sstore_major,
						SSTORE_DEVICE_COUNT, "sstore");
        sstore_major = MAJOR(device_num);
    }

    //check result of registration
    if (result < 0) {
		printk(KERN_ALERT "Major number %d not found: sstore", sstore_major);
		return result;
	}

    //allocate space for the devices (an array of sstore structs)
    sstore_dev_array = kmalloc(SSTORE_DEVICE_COUNT * sizeof(struct sstore),
                               GFP_KERNEL);
    //check that the allocation was successful, if not, exit gracefully
    if (!sstore_dev_array) {
        sstore_cleanup_and_exit();
        return -ENOMEM;
    }
    //clean the array to null values
    memset(sstore_dev_array, 0, SSTORE_DEVICE_COUNT * sizeof (struct sstore));

    //initialize each device in the array
    for (i = 0; i < SSTORE_DEVICE_COUNT; ++i) {

    }

    //successful return
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
static void sstore_cleanup_and_exit(void)
{
    int i = 0;
    dev_t device_num = MKDEV(sstore_major, sstore_minor);

    //DEBUG OUPUT
	printk(KERN_DEBUG "In _exit\n");

    //free the allocated devices
    if (sstore_dev_array) {
        for (i = 0; i < SSTORE_DEVICE_COUNT; ++i) {
            cdev_del(&sstore_dev_array[i].cdev);
        }
        kfree(sstore_dev_array);
    }

    /* 
     * Unregister devices (there is guaranteed to be registered devices here 
     * since init() takes care of registration failure)
     */
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
module_exit(sstore_cleanup_and_exit);
