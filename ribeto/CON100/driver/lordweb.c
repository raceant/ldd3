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
#include "lordweb.h"



#define DEVICE_NAME "Lord-Control"

#define LORD_SEEK_SET       (1)  //reset write pointer(for background buff) to top left
#define LORD_END_WRITE      (2)  //background buff is prepared, swap with foreground buff
#define LORD_WRITE          (3)  //write buff(background)
#define LORD_IO_WRITE       (4)
#define LORD_RS485_READ     (5)

#define LORD_UP_ON      (0x00000001)
#define LORD_UP_OFF     (0x00000002)
#define LORD_DOWN_ON    (0x00000004)
#define LORD_DOWN_OFF   (0x00000008)
#define LORD_RS485_TX   (0x00000010)
#define LORD_RS485_RX   (0x00000020)

#define SCREEN_X    64
#define SCREEN_Y    16
#define SCREEN_BUFF_SIZE    (SCREEN_X * SCREEN_Y / 8)

//Two buffs,one for display(foreground),one for write(background), aditionally four bytes for overflow protection.
static char g_ScrBuff[2 * SCREEN_BUFF_SIZE + 4];
static int  g_iCurrentBuffNum;  //The current(foreground) buff number(0~1)
static int  g_iWrOffset; //Write offset in background buff  

typedef struct 
{
	unsigned long 	pin;
	unsigned int	func;
}Pins_T;

#define PIN_LED_A       0
#define PIN_LED_B       1
#define PIN_LED_C       2
#define PIN_LED_D       3
#define PIN_LED_ENABLE  4
#define PIN_LED_LATCH   5
#define PIN_LED_CLK     6
#define PIN_LED_DATA    7
#define PIN_VD108_STAT  8
#define PIN_UP          9
#define PIN_DOWN        10
#define PIN_RUN         11
#define PIN_485_DIR     12


static Pins_T g_pins[] = {
	/* 0~3:PIN A~D */ 
	{S3C2410_GPF4,S3C2410_GPF4_OUTP}, 
	{S3C2410_GPF5,S3C2410_GPF5_OUTP}, 
	{S3C2410_GPF6,S3C2410_GPF6_OUTP}, 
	{S3C2410_GPG0,S3C2410_GPG0_OUTP}, 
	
	/*4:output enable */
	{S3C2410_GPG3,S3C2410_GPG3_OUTP}, 
	/*5:latch (storage pulse)*/
	{S3C2410_GPG5,S3C2410_GPG5_OUTP}, 
	/*6:clock*/
	{S3C2410_GPG11,S3C2410_GPG11_OUTP}, 
	/*7:serial data*/
	{S3C2410_GPG6,S3C2410_GPG6_OUTP}, 

    /*8:VD108*/
	{S3C2410_GPF1,S3C2410_GPF1_INP},
    /*9:UP*/
	{S3C2410_GPB5,S3C2410_GPB5_OUTP}, 
    /*10:DOWN*/
	{S3C2410_GPB6,S3C2410_GPB6_OUTP}, 
    /*11:RUN*/
	{S3C2410_GPB8,S3C2410_GPB8_OUTP}, 
    /*12:DIR*/
	{S3C2410_GPG10,S3C2410_GPG10_OUTP} 
};

static void InitTimer1(void)
{
	unsigned long tcfg0;
	unsigned long tcfg1;
	unsigned long tcntb;
	unsigned long tcmpb;
	unsigned long tcon;

	struct clk *clk_p;
	unsigned long pclk;
	tcon = __raw_readl(S3C2410_TCON);
	tcfg1 = __raw_readl(S3C2410_TCFG1);
	tcfg0 = __raw_readl(S3C2410_TCFG0);

	//prescaler = 10
	tcfg0 &= ~S3C2410_TCFG_PRESCALER0_MASK;
	tcfg0 |= (10 - 1); 

	//mux = 1/16
	tcfg1 &= ~S3C2410_TCFG1_MUX1_MASK;
	tcfg1 |= S3C2410_TCFG1_MUX1_DIV16;

	__raw_writel(tcfg1, S3C2410_TCFG1);
	__raw_writel(tcfg0, S3C2410_TCFG0);

	clk_p = clk_get(NULL, "pclk");
	pclk  = clk_get_rate(clk_p);
	tcntb  = (pclk/128);
	tcmpb = tcntb>>1;

	__raw_writel(100, S3C2410_TCNTB(1));
	__raw_writel(50, S3C2410_TCMPB(1));
		
	tcon &= ~0xf00;
	tcon |= 0xb00;		//start timer
	__raw_writel(tcon, S3C2410_TCON);
	tcon &= ~0x200;
	__raw_writel(tcon, S3C2410_TCON);

	//tcnto = __raw_readl(S3C2410_TCNTO(1));
}

static int lord_ioctl(
	struct inode *inode, 
	struct file *file, 
	unsigned int cmd, 
	unsigned long arg)
{
    char *pBuff;
 
    switch(cmd)
    {
    case LORD_SEEK_SET:
        g_iWrOffset = 0;
        memset(g_ScrBuff, 0x00, sizeof(g_ScrBuff));
        break;
    case LORD_END_WRITE:
        g_iWrOffset = 0;
        g_iCurrentBuffNum = (g_iCurrentBuffNum) ? 0 : 1;
        break;
    case LORD_WRITE:
        pBuff = (g_iCurrentBuffNum) ? g_ScrBuff : g_ScrBuff + SCREEN_BUFF_SIZE;
        if (g_iWrOffset >= SCREEN_BUFF_SIZE)
            g_iWrOffset = 0;
        pBuff += g_iWrOffset;
        *(long*)pBuff = arg;
	     g_iWrOffset += 4;
        break;
    case LORD_IO_WRITE:
        if (LORD_UP_ON & arg)
		    s3c2410_gpio_setpin(g_pins[PIN_UP].pin, 1);
        if (LORD_UP_OFF & arg)
		    s3c2410_gpio_setpin(g_pins[PIN_UP].pin, 0);
        if (LORD_DOWN_ON & arg)
		    s3c2410_gpio_setpin(g_pins[PIN_DOWN].pin, 1);
        if (LORD_DOWN_OFF & arg)
		    s3c2410_gpio_setpin(g_pins[PIN_DOWN].pin, 0);
        if (LORD_RS485_TX & arg)
		    s3c2410_gpio_setpin(g_pins[PIN_485_DIR].pin, 1);
        if (LORD_RS485_RX & arg)
		    s3c2410_gpio_setpin(g_pins[PIN_485_DIR].pin, 0);
        break;
    default:
        break;
    }
	return 0;
}

static irqreturn_t irq_interrupt(int irq, void *dev_id)
{
	static int iLines = 0; //number of the current scan lines
	int	    iColumn;
    char    *pData;
    char    mask;

	//clear time1 interrupt
	__raw_writel(0x800, S3C2410_SRCPND);
	__raw_writel(0x800, S3C2410_INTPND);

    pData = g_ScrBuff;
    if (g_iCurrentBuffNum)
        pData += SCREEN_BUFF_SIZE;
    pData += (SCREEN_X/8)* (iLines & 0x0F);

    mask = 0x80;
	for (iColumn = 0; iColumn < SCREEN_X; iColumn++)
	{
        
		s3c2410_gpio_setpin(g_pins[PIN_LED_DATA].pin, (*pData & mask) ? 0 : 1); //data
		s3c2410_gpio_setpin(g_pins[PIN_LED_CLK].pin, 1); //clock
		s3c2410_gpio_setpin(g_pins[PIN_LED_CLK].pin, 0); //clock
		mask >>= 1;
		if (0 == mask) {
		    mask = 0x80;
		    pData++;
		}
	}
	s3c2410_gpio_setpin(g_pins[PIN_LED_ENABLE].pin, 1); //turn off
	s3c2410_gpio_setpin(g_pins[PIN_LED_LATCH].pin, 1); //serial to parrel
	s3c2410_gpio_setpin(g_pins[PIN_LED_LATCH].pin, 0); 
	//scan lines
	s3c2410_gpio_setpin(g_pins[PIN_LED_A].pin, (iLines & 0x01) ? 1 : 0);
	s3c2410_gpio_setpin(g_pins[PIN_LED_B].pin, (iLines & 0x02) ? 1 : 0);
	s3c2410_gpio_setpin(g_pins[PIN_LED_C].pin, (iLines & 0x04) ? 1 : 0);
	s3c2410_gpio_setpin(g_pins[PIN_LED_D].pin, (iLines & 0x08) ? 1 : 0);
	s3c2410_gpio_setpin(g_pins[PIN_LED_ENABLE].pin, 0); //turn on

    //CPU runing indicate
	s3c2410_gpio_setpin(g_pins[PIN_RUN].pin, (iLines & 0x800) ? 1 : 0);

	iLines++;
	//iLines &= 0x0F;

	return IRQ_RETVAL(IRQ_HANDLED);
}

static unsigned int lord_irq_poll( struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	printk (DEVICE_NAME" polling\n");
	return mask;
}


static int lord_irq_open(struct inode *inode, struct file *file)
{

//	if (0 == request_irq(IRQ_TIMER1, irq_interrupt, IRQ_TYPE_EDGE_BOTH, 
//		name, NULL))
//		printk (DEVICE_NAME" open\n");

	return 0;
}


static int lord_irq_close(struct inode *inode, struct file *file)
{

//	printk (DEVICE_NAME" close\n");

	return 0;
}

static struct file_operations dev_fops = {
	.owner	=	THIS_MODULE,
	.ioctl	=	lord_ioctl,
	.open	=   lord_irq_open,
	.release=   lord_irq_close, 
	.poll	=   lord_irq_poll,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init lordweb_init(void)
{
	int	    i;
	int 	ret;
	char*   name = "lord_timer1";

    g_iCurrentBuffNum = 0;
    g_iWrOffset = 0;
    memset(g_ScrBuff, 0x05, sizeof(g_ScrBuff));

	for (i = 0; i < sizeof(g_pins)/sizeof(g_pins[0]); i++)
	{
		s3c2410_gpio_cfgpin(g_pins[i].pin, g_pins[i].func);
		s3c2410_gpio_setpin(g_pins[i].pin, 1);
	}
    s3c2410_gpio_setpin(g_pins[PIN_485_DIR].pin, 0);
	ret = misc_register(&misc);
    if (ret)
        return ret;

    ret = request_irq(IRQ_TIMER1, irq_interrupt, IRQ_TYPE_EDGE_BOTH, 
		name, NULL);
    InitTimer1();

	printk (DEVICE_NAME" initialized\n");
	return ret;
}

static void __exit lordweb_exit(void)
{
	free_irq(IRQ_TIMER1, NULL);
    misc_deregister(&misc);
	printk (DEVICE_NAME" removed\n");
}                                    

module_init(lordweb_init);
module_exit(lordweb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("www.ribeto.com");
MODULE_DESCRIPTION("Lord Web's First module test");
