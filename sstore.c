/*                                                     
 * (C) COPYRIGHT 2009 Tyler Hayes - tgh@pdx.edu
 *
 * sstore.c - the most useful device driver ever! 
 */                                                    

#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

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
