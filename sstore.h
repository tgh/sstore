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
    char * junk;
    int crap;
    int stuff;
    struct blob * next;
};

//----------------------------------------------------------------------------

//the device structure
struct sstore {
    struct blob * list_head;
    struct blob * list_tail;
    struct cdev cdev;
};
