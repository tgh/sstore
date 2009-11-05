/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.h 
 */

#include <linux/cdev.h>         /* for the cdev struct */
#include <linux/semaphore.h>    /* for a mutual exclusion semaphore */
#include <linux/ioctl.h>        /* for ioctl macros */
#include <linux/wait.h>         /* for a wait queue */

//----------------------------------------------------------------------------

/*
 * MAJOR/MINOR NUMBERS
 *
 * device major number (0 means it will be dynamically allocated by kernel
 * in the init function).  A zero for the minor number is common practice,
 * but not required.  It is the starting number for devices associated with
 * the driver when allowing multiple devices to use the driver concurrently,
 * i.e. sstore0, sstore1, sstore2, etc.
 */
#define SSTORE_MAJOR 0
#define SSTORE_MINOR 0

//----------------------------------------------------------------------------

/*
 * IOCTL DEFINITIONS
 *
 * ioctl() system call in user space is used for things other than read and
 * write.  This driver only uses one ioctl command, which is deleting a blob at
 * a given index.
 * 0xFF is chosen as the driver's "magic number" simply because it's not listed
 * as being used in the Documentaion/ioctl/ioctl-number.txt file.  (See
 * "Linux Device Drivers" 3rd Ed. pgs. 137-140 for more detail,
 * asm-generic/ioctl.h for macro definitions, and Documentation/ioctl/ioctl-
 * number.txt for magic numbers)
 */
#define SSTORE_IOCTL_MAGIC 0xFF
#define SSTORE_IOCTL_DELETE _IO(SSTORE_IOCTL_MAGIC, 0)
/*
 * this max value is used in driver's ioctl() to test that user's command number
 * passed in is valid.  The number corresponds to the largest command number.
 * Each command is given a sequential number (using the _IO, IOR, _IOW, or _IOWR
 * macros) starting with 0.  Since there is only one here (0, corresponding to
 * SSTORE_IOCTL_DELETE), 0 is used.  If there were 14 different commands, 14
 * would be used.
 */
#define SSTORE_IOCTL_MAX 0

//----------------------------------------------------------------------------

/*
 * MISC. DEFINITIONS
 */

//the number of devices that can be associated with this driver
const int SSTORE_DEVICE_COUNT = 2;

//----------------------------------------------------------------------------

/*
 * STRUCT DEFINITIONS
 */

//the device will be storing a linked list of this struct
struct blob {
    int index;              //index number of the blob (where it is in the list)
    char * junk;            //the data that the blob holds
    struct blob * next;     //pointer to the next blob in the list
};


//the device structure
struct sstore {
    /*
     * fd_count keeps track of how many open file descriptors in user space are
     * associated with the device represented by an instance of this struct.
     * This is done so that the release function in the driver can shut down
     * the device on the last close. (See "Linux Device Drivers" 3rd Ed. pg. 59)
     */
    unsigned int fd_count;
    unsigned int blob_count;    //number of blobs in the list at any given time
    /*
     * this is the head of the wait queue. wait_queue_head_t is a typedef for
     * struct __wait_queue_head (see linux/wait.h).  This queue is used in the
     * read() function of sstore.c.
     */
    wait_queue_head_t wait_queue;
    struct blob * list_head;    //head of the blob linked list
    struct blob * seek_blob;    //a pointer to the last used blob
    struct semaphore mutex;     //semaphore for mutal exclusion
    struct cdev cdev;
};


//this structure mirrors the readWriteBuffer struct in the test program
struct user_buffer {
    int index;      //index into the blob list
    int size;       //size of the data transfer
    char * data;    //where the data being transfered resides
};
