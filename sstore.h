/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.h 
 */

#include <linux/cdev.h> /* for the cdev struct */

//----------------------------------------------------------------------------

/* device major number (0 means it will be dynamically allocated by kernel
 * in the init function).  A zero for the minor number is common practice,
 * but not required.  It is the starting number for devices associated with
 * the driver when allowing multiple devices to use the driver concurrently,
 * i.e. sstore0, sstore1, sstore2, etc.
 */
#define SSTORE_MAJOR 0
#define SSTORE_MINOR 0

//the number of devices that can be associated with this driver
const int SSTORE_DEVICE_COUNT = 2;

//----------------------------------------------------------------------------

//that which we are storing with this device
struct blob {
    int index;
    char * junk;
    struct blob * next;
};

//----------------------------------------------------------------------------

//the device structure
struct sstore {
    /*
     * fd_count keeps track of how many open file descriptors in user space are
     * associated with the device represented by an instance of this struct.
     * This is done so that the release function in the driver can shut down
     * the device on the last close. (See "Linux Device Drivers" 3rd Ed. pg. 59)
     */
    unsigned int fd_count;
    unsigned int blob_count;
    struct blob * list_head;
    struct blob * current_blob;
    struct cdev cdev;
};

//----------------------------------------------------------------------------

//this structure mirrors the readWriteBuffer struct in the test program
struct user_buffer {
    int index;
    int size;
    char * data;
};
