/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.h 
 */

#include <linux/cdev.h> /* for the cdev struct */

//----------------------------------------------------------------------------

/* device major number (0 means it will be dynamically allocated by kernel)
 * in the init function */
int SSTORE_MAJOR = 0;

//number of devices that can be associated with this driver
const int SSTORE_DEVICE_COUNT = 2;

//----------------------------------------------------------------------------

//that which we are storing with this device
struct blob {
    char * junk;
    int crap;
    int stuff;
};

//----------------------------------------------------------------------------

//the device structure
struct sstore {
    struct blob * blob_list;
    struct cdev * cdev;
};
