/*************************************

NAME:lordweb.c
COPYRIGHT:www.ribeto.com

*************************************/

#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <mach/regs-gpio.h>
#include <mach/regs-irq.h>
#include <mach/hardware.h>
#include <plat/regs-timer.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>



#define DEVICE_NAME "VD108B"

//PIN connect to vd108
#define VD108_PIN   S3C2410_GPF1
#define VD108_FUNC  S3C2410_GPF1_INP
#define UP_PIN      S3C2410_GPB5
#define UP_FINC     S3C2410_GPB5_OUTP
#define DOWN_PIN    S3C2410_GPB6
#define DOWN_FINC   S3C2410_GPB6_OUTP
#define RUN_PIN     S3C2410_GPB8
#define RUN_FINC    S3C2410_GPB8_OUTP
#define RUN_PIN     S3C2410_GPG10
#define RUN_FINC    S3C2410_GPG10_OUTP

static int vd108_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	unsigned long err;
	int down;

	down = !s3c2410_gpio_getpin(VD108_PIN);

	err = copy_to_user(buff, (const void *)&down, min(sizeof(down), count));

	return err ? -EFAULT : min(sizeof(down), count);
}

static unsigned int vd108_poll( struct file *file, struct poll_table_struct *wait)
{
	//printk (DEVICE_NAME" polling\n");
	return 0;
}


static int vd108_open(struct inode *inode, struct file *file)
{
	//printk (DEVICE_NAME" open\n");
	return 0;
}


static int vd108_close(struct inode *inode, struct file *file)
{
    //printk (DEVICE_NAME" close\n");
	return 0;
}

static struct file_operations dev_fops = {
	.owner	=	THIS_MODULE,
	.read	=	vd108_read,
	.open	=   vd108_open,
	.release=   vd108_close, 
	.poll	=   vd108_poll,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init lordweb_init(void)
{
	int 	ret;

    s3c2410_gpio_cfgpin(S3C2410_GPF2,S3C2410_GPF2_INP);
	ret = misc_register(&misc);
	//printk (DEVICE_NAME" initialized\n");
	return ret;
}

static void __exit lordweb_exit(void)
{
    misc_deregister(&misc);
	//printk (DEVICE_NAME" removed\n");
}                                    

module_init(lordweb_init);
module_exit(lordweb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("www.ribeto.com");
MODULE_DESCRIPTION("rebito vd108b vehicle detector driver");
