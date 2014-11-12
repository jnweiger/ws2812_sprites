			jw, Mo 1. Jan 20:46:44 CET 2007

choosing usart1, usart0, spi
----------------------------
# With /dev/ttyS2 (usart1) and no dma or pdc enabled, we can make the ws2812 leds flicker wild.
# This confirms correct wiring, and basic setup. But there is no reliable timing yet. This is to be
# expected due to one interrupt per byte.
#
# study http://www.acmesystems.it/rs232, section about DMA settings...
# study https://github.com/tanzilli/playground spi/usart examples in C and python.
# We use the dts designer at http://www.acmesystems.it/pinout_arietta 
# for guidance, what is most easy.
# The usart's have no dma when enabled. Their buffer is 1 byte.
# (Code is there, but whe the simple device tree entry shown below, usart1
# is non-operational. Without dma/pdc, we get fast 1 byte writes, with enabled pdc, it is incedibly slow.
#
# The SPI always has its dma enabled, when chosen  http://www.acmesystems.it/pinout_arietta
#
# Alternative 1)
# We have more specific instructions for the device tree, in linux-3.16.1/Documentation/devicetree/bindings/serial/atmel-usart.txt
# demonstrating usart0 with dma. Try that.
#
# best success so far using acme-arietta.dts.usart0:
#  atmel_usart f801c000.serial: using dma0chan1 for rx DMA transfers
#  atmel_usart f801c000.serial: using dma0chan2 for tx DMA transfers
#  atmel_serial: TCSETX custom_divisor=4
# but fifo_size=1 and no light. hmmm.
# 
#
# Alternative 2)
# FROM https://groups.google.com/forum/#!msg/acmesystems/otm4_54Y5mU/DJXmHqqAbIkJ
In case someone else will need it...
This made my Aria ttyS3 use DMA and work fine at speed 460800.
(Kernel 3.14.23)

     /* /dev/ttyS3 */
      usart2: serial@f8024000 {
        interrupts = <7 IRQ_TYPE_LEVEL_HIGH 5>;
        pinctrl-names = "default";
        pinctrl-0 = <&pinctrl_usart2>;
        atmel,use-dma-rx;
        atmel,use-dma-tx;
        dmas = <&dma1 1 AT91_DMA_CFG_PER_ID(12)>,
               <&dma1 1 (AT91_DMA_CFG_PER_ID(13) | AT91_DMA_CFG_FIFOCFG_ASAP)>;

        dma-names = "tx", "rx";
        status = "okay";
      };

###############################################
# Alternative 3)
# Try SPI. 

			jw, Fr 3. Okt 18:18:15 CEST 2014

compile a device tree
---------------------
# in order to enable dma for usart1 (aka serial2 aka /dev/ttyS2)
# 
# study https://www.sparkfun.com/datasheets/DevTools/SAM7/at91sam7%20serial%20communications.pdf
# study http://www.acmesystems.it/rs232, section about DMA settings...

firefox http://devicetree.org/Device_Tree_Usage
less linux-3.16.1/Documentation/devicetree/bindings/serial/atmel-usart.txt
less linux-3.16.1/arch/arm/boot/dts/at91sam9g25.dtsi

vi arch/arm/boot/dts/acme-arietta.dts
...
                        /* /dev/ttyS2 with PDC (not dma) */
                        usart1: serial@f8020000 {
                                atmel,use-dma-rx;
                                atmel,use-dma-tx;
                                dma-names = "tx", "rx";
                                status ="okay";
                                pinctrl-0 = <&pinctrl_usart1 
                                        &pinctrl_usart1_rts 
                                        &pinctrl_usart1_cts>;
                        };
ZZ
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- acme-arietta.dtb VERBOSE=1
scp arch/arm/boot/dts/acme-arietta.dtb root@192.168.10.10:/boot

		

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
  TCGETX TCSETX

root@arietta:~# ./tcsetx /dev/ttyS2
/dev/ttyS2 -> fd=3
TCGETX=0 -> r=21554, errno=0
baud_base=9611 custom_divisor=868 xmit_fifo_size=1
type(use_dma_tx)=0 flags(use_pdc_tx)=0 line=133333333

root@arietta:~# ./tcsetx /dev/ttyS2 2000000
/dev/ttyS2 -> fd=3
TCGETX=21554 -> r=0, errno=0
TCSETX=21555 -> r=0, errno=0
TCGETX=21554 -> r=0, errno=0
baud_base=2083333 custom_divisor=5 xmit_fifo_size=1
type(use_dma_tx)=0 flags(use_pdc_tx)=4096 line=133333333

## Cool, setting the baud rate works!

# We have no dma channels, can we switch them on?
of_get_property(np, "atmel,use-dma-tx", N


			jw, Mi 1. Okt 16:15:03 CEST 2014


74HTC02 in 8+9 out 10
8+9 from pin 28 
pin 10 to LEDs

ttyS2 (usart1) TXD is pin 28 (aka PA5)
ttyS1 (usart0) TXD is pin 23 (aka PA0)
SPI MOSI is pin 8 (aka PA22)

acme-arietta-dtb
acme-arietta-dts
settings generated with http://www.acmesystems.it/pinout_arietta
 ttyS2 enabled, i2c-0 enabled, SPI1 enabled
 PWM lines enabled,  I2S bus enabled

http://www.acmesystems.it/ledpanel
git clone https://github.com/tanzilli/ledpanel.git

