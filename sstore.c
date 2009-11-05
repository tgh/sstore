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
#include <linux/sched.h>        /* for current process info */
#include <linux/uaccess.h>      /* for copy_to_user() and copy_from_user() */
#include <linux/proc_fs.h>      /* for use of the /proc file system */
#include "sstore.h"             /* SSTORE_MAJOR, SSTORE_DEVICE_COUNT
                                   struct sstore, struct blob */


/*
 * Function prototypes
 */
static int sstore_init(void);
int sstore_open(struct inode * i_node, struct file * file);
int sstore_proc_read_data(char * page, char ** start, off_t offset, int count,
        int * eof, void * data);
int sstore_proc_read_stats(char * page, char ** start, off_t offset, int count,
        int * eof, void * data);
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
//major and minor numbers
unsigned int sstore_major = SSTORE_MAJOR;
unsigned int sstore_minor = SSTORE_MINOR;
//for an array of sstore devices
struct sstore * sstore_dev_array;
//used for creating a /proc directory (used in init() and cleanup_and_exit())
struct proc_dir_entry * sstore;

/*
 * Module Parameters -- S_IRUGO is a permissions mask that means this parameter
 * can be read by the world, but cannot be changed.
 */
unsigned int max_blobs = 10;
unsigned int max_size = 1048;
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

    //initialize each sstore device in the array
    for (i = 0; i < SSTORE_DEVICE_COUNT; ++i) {
        //set open file count to 0
        sstore_dev_array[i].fd_count = 0;
        //set blob count to 0
        sstore_dev_array[i].blob_count = 0;
        //set pointers to NULL
        sstore_dev_array[i].list_head = NULL;
        sstore_dev_array[i].seek_blob = NULL;
        //initialize mutex lock for mutual exclusion of sstore struct variables
        sema_init(&sstore_dev_array[i].mutex, 1);
        //initialize wait queue for blocking i/o in read
        init_waitqueue_head(&sstore_dev_array[i].wait_queue);
        //initialize char device structure
        cdev_init(&sstore_dev_array[i].cdev, &sstore_fops);
        sstore_dev_array[i].cdev.owner = THIS_MODULE;
        sstore_dev_array[i].cdev.ops = &sstore_fops;
        device_num = MKDEV(sstore_major, sstore_minor + i);
        //notify the kernel of this device--upon success, device is now "live"
        error = cdev_add(&sstore_dev_array[i].cdev, device_num, 1);
	    if (error) {
            printk(KERN_ALERT "Error %d adding device sstore%d", error, i);
            sstore_cleanup_and_exit();
        }
    }

    /*
     * create /proc files.  The data file keeps a record of the data stored in
     * the device's blob list.  The stats file gives statistics of the device,
     * such as a count of open device file descriptors.
     */
    sstore = proc_mkdir("sstore", NULL);
    create_proc_read_entry("data", 0, sstore, sstore_proc_read_data, NULL);
    create_proc_read_entry("stats", 0, sstore, sstore_proc_read_stats, NULL);

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

    //acquire mutex lock
    if (down_interruptible(&device->mutex))
        return -ERESTARTSYS;

    if (device) {
        ++device->fd_count;
        //DEBUG OUTPUT
        printk(KERN_DEBUG "\nopen count in open = %d", device->fd_count);
    }
    /*
     * store this sstore struct in the private_data field so that calls to read,
     * write, and ioctl--which will pass in the same file struct pointer--can
     * access the data being stored in the sstore struct's blob list. 
     */
    filp->private_data = device;

    //release mutex lock
    up(&device->mutex);

    return 0;
}

//---------------------------------------------------------------------------

/*
 * PROC: sstore/data file.
 *
 * This function outputs the contents of the device's blob list.  The char **
 * start and void * data arguments are ignored.
 */
int sstore_proc_read_data(char * page, char ** start, off_t offset, int count,
        int * eof, void * data) {
    struct sstore * device; //used to traverse the device array
    struct blob * blob;     //used to traverse the blob list
    int seek = 0;           //keeps track of where to write in page
    int limit = count - 100;//add a pillow of 100 bytes just in case
    int i = 0;

    //print the contents of each device
    for (i = 0; i < SSTORE_DEVICE_COUNT && seek < limit; ++i) {
        device = &sstore_dev_array[i];
        //acquire mutex lock on device
        if (down_interruptible(&device->mutex))
            return -ERESTARTSYS;
        //set temporary blob pointer to head of blob list in the device
        blob = device->list_head;
        //output "no data" message if device doesn't have a list allocated
        if (!blob)
            seek += sprintf(page + seek, "\nSstore Device %i has no data.", i);
        //output data throughout the list
        while (blob) {
            seek += sprintf(page + seek, "\nSstore Device No. = %i", i);
            seek += sprintf(page + seek, " - Blob No. = %i", blob->index);
            seek += sprintf(page + seek, " - Data = ");
            if (blob->junk)
                seek += sprintf(page + seek, "\"%s\"", blob->junk);
            else
                seek += sprintf(page + seek, "NO DATA");
            blob = blob->next;
        }

        //output a newline for readablilty
        seek += sprintf(page + seek, "\n");
        //release mutex lock
        up(&device->mutex);
    }

    //set end-of-file flag
    *eof = 1;

    return seek;
}

//---------------------------------------------------------------------------

/*
 * PROC: sstore/stats file.
 *
 * This function outputs statistics of the device, such as a count of open
 * device file descriptors.
 */
int sstore_proc_read_stats(char * page, char ** start, off_t offset, int count,
        int * eof, void * data) {
    struct sstore * device; //used to traverse the device array
    int seek = 0;           //keeps track of where to write in page
    int limit = count - 100;//add a pillow of 100 bytes just in case
    int i = 0;

    //print the stats of each device
    for (i = 0; i < SSTORE_DEVICE_COUNT && seek < limit; ++i) {
        device = &sstore_dev_array[i];
        //acquire mutex lock on device
        if (down_interruptible(&device->mutex))
            return -ERESTARTSYS;
        //output number of open file descriptors for device
        seek += sprintf(page + seek, "\nSstore Device %i: ", i);
        seek += sprintf(page + seek, "%i open store(s) - ", device->fd_count);
        //output number of blobs in the device's blob list
        seek += sprintf(page + seek, "%i blobs - ", device->blob_count);
        //output the index of the blob the seek_blob pointer is pointing to
        if (device->seek_blob)
            seek += sprintf(page + seek, "seek pointer is at index %i",
                                                    device->seek_blob->index);
        else
            seek += sprintf(page + seek, "seek pointer is NULL");

        //output a newline for readablilty
        seek += sprintf(page + seek, "\n");
        //release mutex lock
        up(&device->mutex);
    }

    //set end-of-file flag
    *eof = 1;

    return seek;
}

//---------------------------------------------------------------------------

/*
 * READ.  The loff_t * file_position and size_t count arguments are ignored.
 *
 * This function will return data to the user even if there wasn't enough to
 * satisfy the amount requested.  In that case, all of the data in the blob
 * will be copied back to user.  If there is no data at all to be read, or if
 * there is no blob at the desired index, then it will wait (sleep) until data
 * is available there.
 */
ssize_t sstore_read(struct file * filp, char __user * buffer, size_t count,
                                                    loff_t * file_position) {
    struct sstore * device = filp->private_data;
    struct blob * blob;         //used for traversing the blob list
    struct user_buffer * u_buf; //char __user * buffer gets copied into here
    int current_index = 0;      //the value of the current_blob pointer index
    int error = 0;              //used for detecting error return values
    int bytes_read = 0;         //the amount actually read (sent back to user)
    int i = 0;                  //for for loops


    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nIn sstore read()");

    //allocate space for the user's buffered content to be placed
    u_buf = kmalloc(sizeof (struct user_buffer), GFP_KERNEL);
    //copy contents of user buffer (a struct of the same form) into u_buf struct
    error = copy_from_user(u_buf, buffer, sizeof (struct user_buffer));
    if (error) {
        //DEBUG OUTPUT
        printk(KERN_DEBUG "\nError in copying buffer from user in read()\n");
        return -EFAULT;
    }
    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nrequested index in read = %d", u_buf->index);
    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nrequested size of data in read = %d", u_buf->size);
    
    //return inavlid argument error if requested index goes beyond maximum blobs
    if (u_buf->index > max_blobs || u_buf->index <= 0)
        return -EINVAL;

    //acquire mutex lock
    if (down_interruptible(&device->mutex))
        return -ERESTARTSYS;

    /*
     * check that requested index is beyond the end of list, and wait if it is.
     * This also takes care if the case where the blob list is empty (device->
     * list_head is NULL).
     */
    while (u_buf->index > device->blob_count) {
        //release mutex lock
        up(&device->mutex);

        //DEBUG OUTPUT
        printk(KERN_DEBUG "\n\"%s\" in read() is sleeping...", current->comm);
        //block (wait for blob at requested index)
        if (wait_event_interruptible(device->wait_queue, 
                                     (u_buf->index <= device->blob_count)));
            return -ERESTARTSYS;
        //acquire mutex lock
        if (down_interruptible(&device->mutex))
            return -ERESTARTSYS;
    }

    //traverse the list to the requested index
    current_index = device->seek_blob->index;
    if (u_buf->index < current_index) {
        blob = device->list_head;
        current_index = 1;
    }
    else
        blob = device->seek_blob;
    while (u_buf->index != current_index) {
        blob = blob->next;
        current_index = blob->index;
    }
    device->seek_blob = blob; //redundant if requested index == current index

    /*
     * determine the amount of data to copy to the user. it will either be the
     * amount requested by the user if there is enough data in the junk array,
     * or it will be whatever is in the junk array if the requested amount is
     * too big.  If there is nothing there (junk pointer is NULL), wait until
     * something is there.
     */
    while (!blob->junk) {
        //release mutex lock
        up(&device->mutex);

        //DEBUG OUTPUT
        printk(KERN_DEBUG "\n\"%s\" in read() is sleeping...", current->comm);
        //block (wait for data on the requested blob)
        if (wait_event_interruptible(device->wait_queue, (blob->junk)));
            return -ERESTARTSYS;
        //acquire mutex lock
        if (down_interruptible(&device->mutex))
            return -ERESTARTSYS;
    }
    for (i = 0; blob->junk[i] != '\0' && i < u_buf->size; ++i) {
        ++bytes_read;
    }

    //copy the junk data to the buffer sent in by the user and check for error
    error = copy_to_user(u_buf->data, blob->junk, bytes_read);
    if (error) {
        //release mutex lock
        up (&device->mutex);
        return -EFAULT;
    }

    //release mutex lock
    up (&device->mutex);

    //tell the user how many bytes were read
    return bytes_read;
}

//---------------------------------------------------------------------------

/*
 * WRITE.  The loff_t * file_position and size_t count arguments are ignored.
 *
 * This function will allocate blobs if it needs to in order to get to the
 * given index (as long as the index is not beyond max_blobs of course).  If
 * there is data already in the blob at the given index, that blob is replaced
 * by a new blob with the data to be written, and the previous blob is freed.
 * NOTE: it is assumed that the data being written is delimited by '\0'. 
 */
ssize_t sstore_write(struct file * filp, const char __user * buffer,
                                        size_t count, loff_t * file_position) {
    struct sstore * device = filp->private_data;
    struct blob * blob;         //used for traversing the blob list
    struct blob * previous_blob;// "
    struct user_buffer * u_buf; //char __user * buffer get copied into here
    int current_index = 0;      //the value of the current_blob pointer index
    int error = 0;              //used for detecting error return values
    int bytes_written = 0;      //the amount actually written


    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nIn sstore write()");

    //allocate space for the user's buffered content to be placed
    u_buf = kmalloc(sizeof (struct user_buffer), GFP_KERNEL);
    //copy contents of user buffer (a struct of the same form) into u_buf struct
    error = copy_from_user(u_buf, buffer, sizeof (struct user_buffer));
    if (error) {
        //DEBUG OUTPUT
        printk(KERN_DEBUG "\nError in copying buffer from user in write()\n");
        return -EFAULT;
    }
    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nrequested index in write = %d", u_buf->index);
    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nrequested size of data in write = %d", u_buf->size);

    //acquire mutex lock
    if (down_interruptible(&device->mutex))
        return -ERESTARTSYS;

    //return inavlid argument error if given index is beyond maximum blobs
    if (u_buf->index > max_blobs || u_buf->index <= 0  || u_buf->size <= 0) {
        //release mutex lock
        up(&device->mutex);
        return -EINVAL;
    }

    /* 
     * traverse the list to the given index and allocate any blobs along the way
     * if need be
     */
    if (!device->list_head) {
        current_index = 0;
        device->list_head = kmalloc(sizeof (struct blob), GFP_KERNEL);
        blob = device->list_head;
        ++current_index;
        ++device->blob_count;
        while (u_buf->index != current_index) {
            blob->index = current_index;
            blob->junk = NULL;
            blob->next = kmalloc(sizeof (struct blob), GFP_KERNEL);
            blob = blob->next;
            ++current_index;
            ++device->blob_count;
        }
        blob->index = current_index;
        blob->next = NULL;
        blob->junk = NULL;
    } else {
        current_index = device->seek_blob->index;
        if (u_buf->index < current_index) {
            blob = device->list_head;
            current_index = 1;
        } else
            blob = device->seek_blob;
        while (current_index != u_buf->index) {
            blob = blob->next;
            ++current_index;
            if (!blob) {
                blob = kmalloc(sizeof (struct blob), GFP_KERNEL);
                blob->index = current_index;
                blob->next = NULL;
                blob->junk = NULL;
                ++device->blob_count;
            }
        }
    }

    /*
     * we are now at the blob at the given index. Set the device's current
     * blob pointer to the blob being written to.
     */
    device->seek_blob = blob;

    /*
     * allocate space for the data in the blob.  If the given amount to
     * write is greater than the maximum size specified by the module
     * parameter max_size, then max_size of data is written.
     */
    if (u_buf->size > max_size)
        u_buf->size = max_size;
    bytes_written = u_buf->size;
    if (blob->junk) {
        blob = kmalloc (sizeof (struct blob), GFP_KERNEL);
        blob->index = u_buf->index;
        blob->next = device->seek_blob->next;
        blob->junk = NULL;
        if (u_buf->index != 1) {
            previous_blob = device->list_head;
            while (previous_blob->next != device->seek_blob) {
                previous_blob = previous_blob->next;
            }
            previous_blob->next = blob;
        } else
            device->list_head = blob;
        device->seek_blob->next = NULL;
        kfree(device->seek_blob->junk);
        kfree(device->seek_blob);
        device->seek_blob = blob;
    }
    blob->junk = kmalloc(u_buf->size + 1, GFP_KERNEL);

    //copy the data from user to blob
    error = copy_from_user(blob->junk, u_buf->data, bytes_written);
    if (error) {
        //release mutex lock
        up(&device->mutex);
        return -EFAULT;
    }

    //notify sleeping readers that something has been written to the blob list
    wake_up_interruptible(&device->wait_queue);

    //release mutex lock
    up(&device->mutex);

    //return the number of bytes written to the user
    return bytes_written;
}

//---------------------------------------------------------------------------

/*
 * IOCTL.
 *
 * The only command in ioctl is to delete a blob at an index specified by arg.
 * When a blob does not exist at the valid index passed in by the user, -EINVAL
 * is returned.  It would be nice to have a -ENOBLOB error defined, but oh well.
 */
int sstore_ioctl(struct inode * inode, struct file * filp, unsigned int command,
                                                        unsigned long arg) {
    struct sstore * device = filp->private_data;
    struct blob * current_blob;     //used for traversing blob list
    struct blob * previous_blob;    // "


    //DEBUG OUTPUT
    printk(KERN_DEBUG "\nIn sstore ioctl()");

    /*
     * check validity of command sent in by user.
     * -ENOTTY means "Inappropriate I/O control operation." (see "Linux Device
     * Drivers" 3rd Ed. p.140)
     */
	if (_IOC_TYPE(command) != SSTORE_IOCTL_MAGIC) return -ENOTTY;
	if (_IOC_NR(command) > SSTORE_IOCTL_MAX) return -ENOTTY;

    switch (command) {
        case SSTORE_IOCTL_DELETE:
            if (arg > max_blobs || arg <= 0)
                return -EINVAL;

            //acquire mutex lock
            if (down_interruptible(&device->mutex))
                return -ERESTARTSYS;

            //return no blob error if there is no list
            if (!device->list_head) {
                //release mutex lock
                up(&device->mutex);
                return -EINVAL;
            }

            //special case: blob to delete is the first one
            if (arg == 1) {
                //set current_blob to the blob being deleted
                current_blob = device->list_head;
                //set head pointer to second blob in list
                device->list_head = current_blob->next;
                /*
                 * set previous pointer to the start of the rest of the list in
                 * order to update the index values of the remaining blobs.
                 */
                previous_blob = device->list_head;
            } else {
                if (arg < device->seek_blob->index)
                    current_blob = device->list_head;
                else
                    current_blob = device->seek_blob;
                //traverse the list until the blob at given index (arg) is found
                while (current_blob->index != arg) {
                    current_blob = current_blob->next;
                    //return no blob error if end of list has been reached
                    if (!current_blob) {
                        //release mutex lock
                        up(&device->mutex);
                        return -EINVAL;
                    }
                }
                //traverse previous_blob pointer to the blob in front of current
                previous_blob = device->list_head;
                while (previous_blob->next != current_blob)
                    previous_blob = previous_blob->next;
                //set previous blob's next pointer to blob in front of current
                previous_blob->next = current_blob->next;
                /*
                 * set previous pointer to the start of the rest of the list in
                 * order to update the index values of the remaining blobs.
                 */
                previous_blob = previous_blob->next;
            }

            //clear first blob's next pointer (could already be NULL)
            current_blob->next = NULL;
            //delete the blob (free kmalloced memory)
            if (current_blob->junk)
                kfree(current_blob->junk);
            kfree(current_blob);

            //update the index numbers of the remaining blobs in the list
            while (previous_blob) {
                --previous_blob->index;
                previous_blob = previous_blob->next;
            }

            //update the blob count
            --device->blob_count;

            //release mutex lock
            up(&device->mutex);

            break;
            
        /*
         * the only way this could be entered is if a command was removed from
         * sstore.h and the subsequent commands were not updated, thus a gap
         * in the command numbers.
         */
        default:
            return -EINVAL;
    }

    //return success
    return 0;
}

//---------------------------------------------------------------------------

/*
 * RELEASE.
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
    printk(KERN_DEBUG "\nIn sstore_release");

    //identify which device is being closed
    device = container_of(inode->i_cdev, struct sstore, cdev);

    //acquire mutex lock
    if (down_interruptible(&device->mutex))
        return -ERESTARTSYS;

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
            //cleanup pointers
            current_blob = NULL;
            previous_blob = NULL;
            device->list_head = NULL;
            device->seek_blob = NULL;
        }
    }

    //release mutex lock
    up(&device->mutex);

    return 0;
}

//---------------------------------------------------------------------------

/*
 * EXIT.
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

    //remove /proc files
    remove_proc_entry("data", sstore);
    remove_proc_entry("stats", sstore);
    remove_proc_entry("sstore", NULL);

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
