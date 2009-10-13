/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.c - the most useful device driver ever! 
 */                                                    

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tyler Hayes");

static int sstore_init(void)
{
	printk(KERN_ALERT "Hello, world\n");
	return 0;
}

static void sstore_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(sstore_init);
module_exit(sstore_exit);

/*
 * Parameters
 */
static unsigned long max_blobs = 1;
static unsigned long max_size = 1024;
module_param(max_blobs, ulong, S_IRUGO);
module_param(max_size, ulong, S_IRUGO);
