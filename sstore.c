/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.c - the most useful device driver ever! 
 */                                                    

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>  /* for module_param() */
#include <linux/fs.h>           /* for struct file, struct file_operations,
                                 * register_chrdev_region(),
                                 * alloc_chrdev_region() */
#include <linux/types.h>        /* for dev_t type (represents device numbers)*/
#include <linux/kdev_t.h>       /* for MKDEV(), MAJOR(), and MINOR() macros */
#include <linux/cdev.h>         /* for struct cdev */


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tyler Hayes");

int sstore_major = 0;
int sstore_device_count = 2;

//---------------------------------------------------------------------------

/*
 * INIT
 */
static int sstore_init(void)
{
    int result = 0;
    dev_t device = MKDEV(sstore_major, 0);

    printk(KERN_ALERT "In _init\n");

    if (sstore_major) {
	    result = register_chrdev_region(device, sstore_device_count, 
								"sstore");
    } else {
        result = alloc_chrdev_region(&device, sstore_major,
						sstore_device_count, "sstore");
        sstore_major = MAJOR(device);
    } 
    if (result < 0) {
		printk(KERN_ALERT "Major number %d not found: sstore", 
								sstore_major);
		return result;
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
    dev_t device = MKDEV(sstore_major, 0);

	printk(KERN_ALERT "In _exit\n");

    unregister_chrdev_region(device, sstore_device_count);
}

//---------------------------------------------------------------------------

struct file_operations sstore_fops = {
        .owner = THIS_MODULE,
        .llseek = sstore_llseek,
        .read = sstore_read,
        .write = sstore_write,
        .ioctl = sstore_ioctl,
        .open = sstore_open,
        .release = sstore_release
};

module_init(sstore_init);
module_exit(sstore_exit);

/*
 * Parameters
 */
static unsigned long max_blobs = 1;
static unsigned long max_size = 1024;
module_param(max_blobs, ulong, S_IRUGO);
module_param(max_size, ulong, S_IRUGO);
