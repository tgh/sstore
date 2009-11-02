/*
 * sstore.c - the most useful device driver ever!
 *
 * COPYRIGHT (C) 2009 Tyler Hayes - tgh@pdx.edu
 *
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
#include <linux/sched.h>        /* for current process info */
#include <linux/uaccess.h>      /* for copy_to_user() and copy_from_user() */
#include "sstore.h"             /* SSTORE_MAJOR, SSTORE_DEVICE_COUNT
                                   struct sstore, struct blob */


/*
 * Function prototypes
 */
static int sstore_init(void);
int sstore_open(struct inode * i_node, struct file * file);
loff_t sstore_llseek(struct file * file, loff_t offset, int i);
ssize_t sstore_read(struct file * file, char __user * user, size_t size,
        loff_t * offset);
ssize_t sstore_write(struct file * file, const char __user * user,
        size_t size, loff_t * offset);
int sstore_ioctl(struct inode * i_node, struct file * file, unsigned int ui,
        unsigned long ul);
int sstore_release(struct inode * i_node, struct file * file);
static void sstore_cleanup_and_exit(void);

/*
 * Global variables
 */
unsigned int sstore_major = SSTORE_MAJOR;
unsigned int sstore_minor = SSTORE_MINOR;
struct sstore * sstore_dev_array;

/*
 * Module Parameters -- S_IRUGO is a permissions mask that means this parameter
 * can be read by the world, but cannot be changed.
 */
unsigned int max_blobs = 1;
unsigned int max_size = 1024;
module_param(max_blobs, uint, S_IRUGO);
module_param(max_size, uint, S_IRUGO);
module_param(sstore_major, uint, S_IRUGO);
module_param(sstore_minor, uint, S_IRUGO);

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

//---------------------------------------------------------------------------

/*
 * INIT
 */
static int __init sstore_init(void) {
    int result = 0; //the return status of this function
    int i = 0; //your standard for-loop variable
    int error = 0;  //to catch any errors returned from certain function calls
    dev_t device_num = 0; //the device number (holds major and minor number)

    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nIn sstore_init()");
    printk(KERN_DEBUG "\nmax_blobs = %d, max_size = %d", max_blobs, max_size);

    /*
     * Get a range of minor numbers and register a region for devices.
     * 
     * When sstore_major is not 0 (when it is explicitly given a number by a
     * programmer), the sstore_major and sstore_minor numbers are given to the
     * MKDEV macro:
     * MKDEV(ma,mi)	((ma)<<8 | (mi)), which stores these numbers into a
     * 32-bit unsigned integer type (device_num) by dividing the bits up into a
     * major number section and a minor number section.  device_num is then used
     * to register the device with register_chrdev_region().  Otherwise,
     * the kernel is used to get a number dynamically by sending the address
     * of the device_num variable through the alloc_chrdev_region() function
     * in the else clause. 
     */
    if (sstore_major) {
        device_num = MKDEV(sstore_major, sstore_minor);
        result = register_chrdev_region(device_num, SSTORE_DEVICE_COUNT,
                "sstore");
    } else {
        result = alloc_chrdev_region(&device_num, sstore_minor,
                SSTORE_DEVICE_COUNT, "sstore");
        sstore_major = MAJOR(device_num);
    }

    //check result of registration
    if (result < 0) {
        printk(KERN_ALERT "Major number %d not found: sstore", sstore_major);
        return result;
    }

    //allocate space for the devices (an array of sstore structs)
    sstore_dev_array = kmalloc(SSTORE_DEVICE_COUNT * sizeof (struct sstore),
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
        sstore_dev_array[i].fd_count = 0;
        sstore_dev_array[i].list_head = NULL;
        sstore_dev_array[i].current_blob = NULL;
        sstore_dev_array[i].blob_count = 0;
        cdev_init(&sstore_dev_array[i].cdev, &sstore_fops);
        sstore_dev_array[i].cdev.owner = THIS_MODULE;
        sstore_dev_array[i].cdev.ops = &sstore_fops;
        device_num = MKDEV(sstore_major, sstore_minor + i);
        error = cdev_add(&sstore_dev_array[i].cdev, device_num, 1);
	    if (error) {
            printk(KERN_ALERT "Error %d adding device sstore%d", error, i);
            sstore_cleanup_and_exit();
        }
    }

    //successful return
    return 0;
}

//---------------------------------------------------------------------------

/*
 * OPEN - this funciton is called when the device is opened in userspace (when
 * the "file" /dev/sstore0 or /dev/sstore1 is opened, for example)
 */

int sstore_open(struct inode * inode, struct file * filp) {
    struct sstore * device;

    //DEBUG OUPUT
    printk(KERN_DEBUG "\nIn sstore_open()");

    //check that current process has root priveleges
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    //identify which device is being opened
    device = container_of(inode->i_cdev, struct sstore, cdev);

    if (device) {
        ++device->fd_count;
        //DEBUG OUTPUT
        printk(KERN_DEBUG "\nopen count in open = %d\n", device->fd_count);
    }
    /*
     * store this sstore struct in the private_data field so that calls to read,
     * write, and ioctl--which will pass in the same file struct pointer--can
     * access the data being stored in the sstore struct's blob list. 
     */
    filp->private_data = device;

    return 0;
}

//---------------------------------------------------------------------------

/*
 * LLSEEK
 */
loff_t sstore_llseek(struct file * filp, loff_t offset, int i) {
    return offset;
}

//---------------------------------------------------------------------------

/*
 * READ
 */
ssize_t sstore_read(struct file * filp, char __user * buffer, size_t count,
                                                    loff_t * file_position) {
    struct sstore * device = filp->private_data;
    struct blob * blob;
    int current_index = 0;
    int error = 0;
    int bytes_read = 0;
    int i = 0;
    /*
     * DETAILED COMMENT NEEDED HERE FOR THE NEXT 3 LINES RE: USER STRUCT
     */
    char * data = buffer + 8;
    int requested_index = *buffer;
    count = *(buffer + 4);

    //acquire mutex lock
    // TO DO
    
    //return inavlid argument error if requested index goes beyond maximum blobs
    if (requested_index > max_blobs) {
        //release mutex lock (do we need it for max_blobs???)
        // TO DO
        return -EINVAL;
    }

    //check that there is a blob list allocated, and wait for one if there isn't
    if (!device->list_head)
        //block (wait for data)
        // TO DO

    //check that requested index is beyond the end of list, and wait if it is
    if (requested_index > device->blob_count - 1)
        //block (wait for data at requested index)
        // TO DO

    //traverse the list to the requested index
    current_index = device->current_blob->index;
    if (requested_index < current_index) {
        blob = device->list_head;
        current_index = 0;
    }
    else
        blob = device->current_blob;
    while (requested_index != current_index) {
        blob = blob->next;
        current_index = blob->index;
    }
    device->current_blob = blob; //redundant if requested index == current index
   
    /*
     * determine the amount of data to copy to the user. it will either be the
     * amount requested by the user if there is enough data in the junk array,
     * or it will be whatever is in the junk array if the requested amount is
     * too big.
     */
    for (i = 0; blob->junk[i] != '\0' && i < count; ++i) {
        ++bytes_read;
    }

    //copy the data to the user and check the return value for error
    error = copy_to_user(data, blob->junk, bytes_read);
    if (error) {
        //release lock
        // TO DO
        return -EFAULT;
    }

    //release mutex lock
    // TO DO

    //tell the user how many bytes were read
    return bytes_read;
}

//---------------------------------------------------------------------------

/*
 * WRITE
 */
ssize_t sstore_write(struct file * filp, const char __user * user,
        size_t size, loff_t * offset) {
    return size;
}

//---------------------------------------------------------------------------

/*
 * IOCTL
 */
int sstore_ioctl(struct inode * inode, struct file * filp, unsigned int ui,
        unsigned long ul) {
    return 0;
}

//---------------------------------------------------------------------------

/*
 * RELEASE
 */
int sstore_release(struct inode * inode, struct file * filp) {
    struct sstore * device;
    /*
     * these two blob pointers are used for traversing the blob list to free
     * them upon last close (when fd_count is zero)
     */
    struct blob * current_blob;
    struct blob * previous_blob;

    //DEBUG OUTPUT
    printk(KERN_DEBUG "In sstore_release\n");

    //identify which device is being closed
    device = container_of(inode->i_cdev, struct sstore, cdev);

    //aqcuire lock
    //TO DO

    if (device->fd_count) {
        //decrement the number of open file descriptors
        --device->fd_count;
        //DEBUG OUTPUT
        printk(KERN_DEBUG "\nopen count in release = %d\n", device->fd_count);
        //free the blob list when this is the last close
        if (device->fd_count == 0) {
            current_blob = device->list_head;
            previous_blob = current_blob;
            while (current_blob) {
                current_blob = current_blob->next;
                kfree(previous_blob);
                previous_blob = current_blob;
            }
        }
    }

    //release lock
    //TO DO    

    return 0;
}

//---------------------------------------------------------------------------

/*
 * EXIT
 */
static void sstore_cleanup_and_exit(void) {
    int i = 0;
    dev_t device_num = MKDEV(sstore_major, sstore_minor);

    //DEBUG OUPUT
    printk(KERN_DEBUG "In sstore_exit\n");

    //free the allocated devices
    if (sstore_dev_array) {
        for (i = 0; i < SSTORE_DEVICE_COUNT; ++i) {
            cdev_del(&sstore_dev_array[i].cdev);
        }
        kfree(sstore_dev_array);
    }

    /* 
     * Unregister devices (there is guaranteed to be registered devices here 
     * since init() takes care of registration failure)--in other words, free
     * the region of device numbers so the kernel can reuse them.
     */
    unregister_chrdev_region(device_num, SSTORE_DEVICE_COUNT);
}

//---------------------------------------------------------------------------


//tells kernel which functions run when driver is loaded/removed
module_init(sstore_init);
module_exit(sstore_cleanup_and_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tyler Hayes");
