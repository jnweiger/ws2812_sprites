			jw, Do 2. Okt 10:31:21 CEST 2014

compile a kernel
----------------
# FROM http://www.acmesystems.it/compile_linux_3_16
https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.16.1.tar.xz
tar xvfJ linux-3.16.1.tar.xz
cd linux-3.16.1
wget http://www.acmesystems.it/www/compile_linux_3_16/acme.patch
patch -p1 < acme.patch
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- acme-arietta_defconfig
make ARCH=arm menuconfig
 -> hrtimer on
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- acme-arietta.dtb
make -j8 ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- zImage
make modules -j8 ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
make modules_install INSTALL_MOD_PATH=./modules ARCH=arm
scp arch/arm/boot/dts/acme-arietta.dtb root@192.168.10.10:/boot
scp arch/arm/boot/zImage root@192.168.10.10:/boot
rsync -avc modules/lib/. root@192.168.10.10:/lib/.
reboot
depmod -a



			jw, Do 2. Okt 09:18:44 CEST 2014

baud rate
---------
http://www.atmel.com/Images/doc2640.pdf
-> Table 7.1 Baud rate selection
 MCKI (Mhz) 32000000 -> Max Baud rate: 2000000
 USART.Baud_Rate = MCKI/CD*16

.../drivers/tty/serial/atmel_serial.c

#define ATMEL_US_BRGR           0x20
#define UART_PUT_BRGR(port,v)   __raw_writel(v, (port)->membase + ATMEL_US_BRGR)


## check if a port uses DMA
#ifdef CONFIG_SERIAL_ATMEL_PDC
static bool atmel_use_dma_tx(struct uart_port *port)
  return atmel_port->use_dma_tx;
dma_map_single(...)

we need to add an extra ioctl to set the desired extended baud rate
Candidates: 
  TIOCGSERIAL TIOCSSERIAL
  TIOCGETX TIOCSETX
  TCGETX TCSETX

root@arietta:~# ./tcsetx /dev/ttyS2
/dev/ttyS2 -> fd=3
TCGETX=0 -> r=21554, errno=0
baud_base=9611 custom_divisor=868 xmit_fifo_size=1
type(use_dma_tx)=0 flags(use_pdc_tx)=0 line=133333333

root@arietta:~# ./tcsetx /dev/ttyS0
/dev/ttyS0 -> fd=3
TCGETX=0 -> r=21554, errno=0
baud_base=117370 custom_divisor=72 xmit_fifo_size=1
type(use_dma_tx)=0 flags(use_pdc_tx)=0 line=133333333

# We have no dma channels, can we switch them on?
of_get_property(np, "atmel,use-dma-tx", N


			jw, Mi 1. Okt 16:15:03 CEST 2014


74HTC02 in 8+9 out 10
8+9 from pin 28 
pin 10 to LEDs

ttyS2 TXD is pin 28 (aka PA5)

acme-arietta-dtb
acme-arietta-dts
settings generated with http://www.acmesystems.it/pinout_arietta
 ttyS2 enabled, i2c-0 enabled, SPI1 enabled
 PWM lines enabled,  I2S bus enabled

http://www.acmesystems.it/ledpanel
git clone https://github.com/tanzilli/ledpanel.git
