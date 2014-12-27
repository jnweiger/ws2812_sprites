/*  
 *  ws2812_draiveris.c - A bitbang kernel module for ws2812
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


#define sysRegRead(phys) \
 (*(volatile u32 *)KSEG1ADDR(phys)) 

#define sysRegWrite(phys, val) \
 ((*(volatile u32 *)KSEG1ADDR(phys)) = (val)) 


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Saulius Lukse saulius.lukse@gmail.com; Juergen Weigert juewei@fabfolk.com");
MODULE_DESCRIPTION("Bitbang GPIO driver for ws2812");


//static char *mystring = "blah";
//module_param(mystring, charp, 0000);
//MODULE_PARM_DESC(mystring, "A character string");

static int gpio_number = 20; // default is nr 20
module_param(gpio_number, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(gpio_number, "GPIO number");

static int inverted = 1; // default is 1 == inverted, good for 74HCT02 line drivers.
module_param(inverted, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(inverted, "drive inverted outputs");

#define SET_GPIOS_H(gpio_bits)	do { sysRegWrite(0x1804000CL, gpio_bits); } while (0)
#define SET_GPIOS_L(gpio_bits)	do { sysRegWrite(0x18040010L, gpio_bits); } while (0)

void led_bit(int bit)
{
  if (bit==0)
  {
    // high (3.3us)
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);

    // low (7us)
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);

    //sysRegWrite(0x1804000CL, 1<<gpio_number);
  }
  else
  {
    // high (3.3us)
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);
    sysRegWrite(0x1804000CL, 1<<gpio_number);

    // low (7us)
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);
    sysRegWrite(0x18040010L, 1<<gpio_number);

    //sysRegWrite(0x1804000CL, 1<<gpio_number);
  }
}



void update_leds(char *buff, size_t len)
{

  unsigned long flags;
  long int i = 0;
  int b = 0;

  sysRegWrite(0x1806000CL, 1<<31); // Just in case set watchdog to timeout some time later 
  sysRegWrite(0x18040000L, 1<<gpio_number); // output enable to PIN20

  //static DEFINE_SPINLOCK(critical);

  sysRegWrite(0x1804000CL, 1<<gpio_number);	// HIGH
  // wait 65uS
  for(i=0;i<1000;i++)
  {
    volatile int j = i;
    sysRegWrite(0x18040010L, 1<<gpio_number);	// LOW
  }


  static DEFINE_SPINLOCK(critical);
  spin_lock_irqsave(&critical, flags);
  for (i = 0; i<len; i++)
  {
    for(b = 7; b>=0; b--)
      if (buff[i] & (1 << b))
        led_bit(1);
    else
        led_bit(0);
  }
  sysRegWrite(0x18040010L, 1<<gpio_number);	// LOW
  //sysRegWrite(0x1804000CL, 1<<gpio_number);	// HIGH
  spin_unlock_irqrestore(&critical, flags);

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

	printk(KERN_INFO "Major = %d\n", Major);
  printk(KERN_INFO "GPIO number: %d\n", gpio_number);


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
