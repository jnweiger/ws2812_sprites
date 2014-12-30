Hardware
========

A Carambola2 has the same Atheros AR9331 Chipset as the TL-WR703N or WR3070 Routers.
Note that while the Carambola2 breakout board has very nice connectors to access *many* GPIO pins, 
most of the breakout board you bought remains unused: 2x ETH, 2x USB. The line driver 74HTC


Other Options
-------------

 * A naked Carambola2 without breakout board. This is much smaller, less expensive, but lacks a 5V -> 3.3V 
   regulator and needs an antenna and again the 74HTC02 needs to be mounted somewhere.

 * A TL-WR703N Nano Router. Has both the voltage regulator and an onboard antenna. It has 1x ETH, 
   1x USB that remain unused. If ordered from China, this should be around 20 EUR, beating the naked Carambola2.
   Flash instructions for these routers are here: http://wiki.openwrt.org/toh/tp-link/tl-wr703n
   Disadvantage: Not many GPIO pins have been located by now. Soldering to them is a challenge. 
   Difficult to find (everybody sells wr702, which does not work). Check for TL-MR3020.
   Advantages: Comes in a tiny housing that has room for a 74HTC02, and the Micro-USB connecter that 
   serves as power supply has 3 unused lines. These could be my 3 line drivers, yeah!

 * HLK RM04 is a similar form factor to the naked Carambola2, contains the 5V -> 3.3V regulator, but
   RALINK-RT5350F (360 Mhz), 16MB RAM only.
   http://www.hlktech.net/product_detail.php?ProId=39 
   http://wiki.openwrt.org/toh/hilink/hlk-rm04
   Seen on Ebay for 11 EUR, seen on aliexpress for 17 EUR.


python implementation
=====================
	
Heavy glitches with 20 or more LEDs.
Slight glitches always, as we are at the mercy of the scheduler.

    scp uart.py root@192.168.2.1:
    ssh -v root@192.168.2.1 'pkill python'
    ssh -v root@192.168.2.1 'while true; do python uart.py blink; done'

C implementation
=================

Bit banging with GPIO pins, kernel drivers with blocked interrupts and all.

 Tested glitch-free with 270 leds.
 We can drive multiple led strips, connected to separate GPIO pins
 all simultaneously. The driver accepts write() calls, with G,R,B sequences,
 as if all leds were in one single strip.


Build instructions
------------------

    git clone https://github.com/8devices/carambola2.git
    cd carambola2
 
good build instructions are here:

    less README

General instructions to build the entire SDK are like this:

    ./scripts/feeds update
    ./scripts/feeds install
    make menuconfig 
    make V=s

While sitting inside the SDK, we add and build our package
The ws2812_draiveris package is the kernel driver. It is production ready.
The ws2812_sprites package is user land code, unfinished.

    cd package
    ln -s ~/src/github/ws2812_sprites/carambola2/package/ws2812_draiveris .
    ln -s ~/src/github/ws2812_sprites/carambola2/package/ws2812_sprites .
    echo >> .config CONFIG_PACKAGE_ws2812-sprites=y
    echo >> .config CONFIG_PACKAGE_kmod-ws2812-draiveris=y
    # make oldconfig # not needed?
    make package/ws2812_sprites/compile V=99
    make package/ws2812-draiveris/compile V=99
    ls -l bin/*/packages/ws2812*
     -rw-r--r-- 1 testy users 1913 Jan  3 01:48 bin/ar71xx/packages/ws2812-sprites_1_ar71xx.ipk
     -rw-r--r-- 1 testy users 3839 Dez 28 00:08 bin/ar71xx/packages/kmod-ws2812-draiveris_3.10.49_8_ar71xx.ipk

upload instructions
-------------------

To prepare for upload, you should have set up ssh connection to the device with ssh-keys so that it does 
not prompt for passwords all the time.

    # do this once, so that you don't get asked for passwords any more.
    ssh-copy-id root@192.168.3.1	# this needs to go into /etc/dropbear/authorized_keys

Upload and install the built binary package

    scp bin/*/packages/ws2812* root@192.168.3.1:/tmp
    ssh root@192.168.3.1:
    opkg remove ws2812-sprites kmod-ws2812-draaiveris
    opkg install /tmp/ws2812-sprites_1_ar71xx.ipk
     Installing ws2812-sprites (1) to root...
     Configuring ws2812-sprites.
    opkg install /tmp/kmod-ws2812-draiveris_3.10.49_8_ar71xx.ipk
    rmmod kmod-ws2812-draiveris
    insmod kmod-ws2812-draiveris gpios=20,21,22

This should create /dev/ws2812, to be used by the example python script
ws2812_draiveris_test.py

Alternatively use the upload.sh script, which takes some shortcuts ...

    vi package/ws2812_sprites/src/ws2812_sprites.c
    ./upload.sh
