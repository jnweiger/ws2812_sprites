/*  
 *  ws2812-draiveris.c - A bitbang kernel module for ws2812
 *
 *  Copyright (C) 2006-2012 OpenWrt.org
 *  Copyright (C) 2012 Žilvinas Valinskas, Saulius Lukšė
 *  Copyright (C) 2014 Jürgen Weigert <jw@owncloud.com>
 *
 *  This is free software, licensed under the GNU General Public License v2.
 *  See /LICENSE for more information.
 * 
 * parameters: 
 *	gpio_number=20		# set base number. 
 *	inverted=1		# new: allow use of inverters as line drivers.
 */
#include <linux/init.h>
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include <linux/types.h>
#include <linux/string.h>

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>  /* for put_user */
#include <linux/fs.h>


#define sysRegRead(phys) 	 (*(volatile u32 *)KSEG1ADDR(phys))
#define sysRegWrite(phys, val)	((*(volatile u32 *)KSEG1ADDR(phys)) = (val))

// FROM http://www.eeboard.com/wp-content/uploads/downloads/2013/08/AR9331.pdf p65, p77
#define SYS_REG_GPIO_OE		0x18040000L
#define SYS_REG_GPIO_IN		0x18040004L
#define SYS_REG_GPIO_OUT	0x18040008L
#define SYS_REG_GPIO_SET	0x1804000CL
#define SYS_REG_GPIO_CLEAR	0x18040010L
#define SYS_REG_GPIO_INT	0x18040014L
#define SYS_REG_GPIO_INT_TYPE	0x18040018L
#define SYS_REG_GPIO_INT_POLARITY	0x1804001CL
#define SYS_REG_GPIO_INT_PENDING	0x18040020L
#define SYS_REG_GPIO_INT_MASK		0x18040024L
#define SYS_REG_GPIO_INT_FUNCTION_1	0x18040028L
#define SYS_REG_GPIO_IN_ETH_SWITCH_KED	0x1804002CL
#define SYS_REG_GPIO_INT_FUNCTION_2	0x18040030L

#define SYS_REG_RST_WATCHDOG_TIMER 0x1806000CL

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Saulius Lukse saulius.lukse@gmail.com; Juergen Weigert juewei@fabfolk.com");
MODULE_DESCRIPTION("Bitbang GPIO driver for multiple WS2812 led chains");


static int gpio_number = 20; // default is nr 20
module_param(gpio_number, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(gpio_number, "GPIO number of first chain");

static int inverted = 1; // default is 1 == inverted, good for 74HCT02 line drivers.
module_param(inverted, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(inverted, "drive inverted outputs");

static int led_chains = 3; // default is 3 == the board, I currently have.
module_param(led_chains, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(led_chains, "use additional GPIOs if > 1.");

static int leds_per_chain = 90; // default is 3 == the board, I currently have.
module_param(leds_per_chain, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(leds_per_chain, "start of next led chain");

// define SET_GPIOS_H(gpio_bits)	do { sysRegWrite(SYS_REG_GPIO_SET, gpio_bits); } while (0)
// define SET_GPIOS_L(gpio_bits)	do { sysRegWrite(SYS_REG_GPIO_CLEAR, gpio_bits); } while (0)
#define SET_GPIOS_H(gpio_bits)	sysRegWrite(SYS_REG_GPIO_SET, gpio_bits)
#define SET_GPIOS_L(gpio_bits)	sysRegWrite(SYS_REG_GPIO_CLEAR, gpio_bits)

static int gpio_bit_mask;

void led_bit_1_i(void)
{
    // good: 12L3H .. 12L8H 11L4H .. 16L4H
    // bad:  12L2H 10L4H
    // best: 12L4H (shortest, with one safety each)
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
}

void led_bit_0_i(void)
{
    // good: 3L12H .. 5L12H 4L11H
    // bad: 2L12H 8L12H 6L12H 4L8H 4L10H 3L11H
    // almost: 4L12H has sporadic green flashes on the first led.
    // best: 3L12H (shortest, with one safety each)
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
}


void led_bit_1(void)
{
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
}

void led_bit_0(void)
{
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
}


void update_leds(const char *buff, size_t len)
{

  unsigned long flags;
  long int i = 0;
  int b = 0;

  sysRegWrite(SYS_REG_RST_WATCHDOG_TIMER, 1<<31); // Just in case set watchdog to timeout some time later
  // http://www.eeboard.com/wp-content/uploads/downloads/2013/08/AR9331.pdf
  // p65: SYS_REG_GPIO_OE 0: Enables the driver to be used as input mechanism; 1: Enables the output driver
  i = sysRegRead(SYS_REG_GPIO_OE);
  sysRegWrite(SYS_REG_GPIO_OE, i|gpio_bit_mask); 	// output enable to PIN20


  if (inverted)
    {
      // wait 65uS
      for(i=0;i<1000;i++)
      {
	volatile int j = i;
	SET_GPIOS_H(gpio_bit_mask);
      }


      static DEFINE_SPINLOCK(critical);
      spin_lock_irqsave(&critical, flags);
      for (i = 0; i<len; i++)
      {
        unsigned char c = buff[i];

        if (c & 0x80) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x40) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x20) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x10) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x08) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x04) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x02) led_bit_1_i(); else led_bit_0_i();
        if (c & 0x01) led_bit_1_i(); else led_bit_0_i();
      }
      spin_unlock_irqrestore(&critical, flags);
    }
  else
    {
      // wait 65uS
      for(i=0;i<1000;i++)
      {
	volatile int j = i;
	SET_GPIOS_L(gpio_bit_mask);
      }

      static DEFINE_SPINLOCK(critical);
      spin_lock_irqsave(&critical, flags);
      for (i = 0; i<len; i++)
      {
        unsigned char c = buff[i];

        if (c & 0x80) led_bit_1(); else led_bit_0();
        if (c & 0x40) led_bit_1(); else led_bit_0();
        if (c & 0x20) led_bit_1(); else led_bit_0();
        if (c & 0x10) led_bit_1(); else led_bit_0();
        if (c & 0x08) led_bit_1(); else led_bit_0();
        if (c & 0x04) led_bit_1(); else led_bit_0();
        if (c & 0x02) led_bit_1(); else led_bit_0();
        if (c & 0x01) led_bit_1(); else led_bit_0();
      }
      spin_unlock_irqrestore(&critical, flags);
    }
}


/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "ws2812" /* Dev name as it appears in /proc/ws2812   */
#define BUF_LEN 900    /* Max length of the message from the device = 300 LED'S*/

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;   /* Major number assigned to our device driver */
static int Device_Open = 0; /* Is device open?  
         * Used to prevent multiple access to device */
static char msg[BUF_LEN]; /* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};


int init_module(void)
{ 
  Major = register_chrdev(0, DEVICE_NAME, &fops);

  if (Major < 0) {
    printk(KERN_ALERT "Registering char device failed with %d\n", Major);
    return Major;
  }

  printk(KERN_INFO "Build: %s %s\n", __DATE__, __TIME__);
  printk(KERN_INFO "Major = %d\n", Major);
  printk(KERN_INFO "GPIO number: %d\n", gpio_number);
  printk(KERN_INFO "Inverted: %d\n", inverted);
  printk(KERN_INFO "Number of led chains: %d\n", led_chains);
  printk(KERN_INFO "Leds per chain: %d\n", leds_per_chain);

  // 3 chains: 7<<gpio_number
  gpio_bit_mask = ((1<<led_chains)-1)<<gpio_number;
  return SUCCESS;
}


/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{  
  unregister_chrdev(Major, DEVICE_NAME);
  printk(KERN_ALERT "Bye, that's all.\n");
}






/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
  static int counter = 0;

  if (Device_Open)
    return -EBUSY;

  Device_Open++;
  sprintf(msg, "I already told you %d times Hello world!\n", counter++);
  msg_Ptr = msg;
  try_module_get(THIS_MODULE);

  return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
  Device_Open--;    /* We're now ready for our next caller */

  /* 
   * Decrement the usage count, or else once you opened the file, you'll
   * never get get rid of the module. 
   */
  module_put(THIS_MODULE);

  return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
         char *buffer,  /* buffer to fill with data */
         size_t length, /* length of the buffer     */
         loff_t * offset)
{
  /*
   * Number of bytes actually written to the buffer 
   */
  int bytes_read = 0;

  /*
   * If we're at the end of the message, 
   * return 0 signifying end of file 
   */
  if (*msg_Ptr == 0)
    return 0;

  /* 
   * Actually put the data into the buffer 
   */
  while (length && *msg_Ptr) {

    /* 
     * The buffer is in the user data segment, not the kernel 
     * segment so "*" assignment won't work.  We have to use 
     * put_user which copies data from the kernel data segment to
     * the user data segment. 
     */
    put_user(*(msg_Ptr++), buffer++);

    length--;
    bytes_read++;
  }

  /* 
   * Most read functions return the number of bytes put into the buffer
   */
  return bytes_read;
}


static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
  update_leds(buff, len);
  //printk(KERN_INFO "GOT: %s, %d\n", buff, len);
  return len;
}
