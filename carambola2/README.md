python implementation
=====================
	
    scp uart.py root@192.168.2.1:
    ssh -v root@192.168.2.1 'pkill python'
    ssh -v root@192.168.2.1 'while true; do python uart.py blink; done'

C implementation
================

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

    cd package
    ln -s ~/src/github/ws2812_sprites/carambola2/package/ws2812_sprites .
    echo >> .config CONFIG_PACKAGE_ws2812_sprites=y
    # make oldconfig # not needed?
    make package/ws2812_sprites/compile V=99
    make package/ws2812_sprites/install V=99
    ls -l bin/*/packages/ws2812_sprites*
     -rw-r--r-- 1 testy users 1913 Jan  3 01:48 bin/ar71xx/packages/ws2812_sprites_1_ar71xx.ipk

upload instructions
-------------------

To prepare for upload, you should have set up ssh connection to the device with ssh-keys so that it does 
not prompt for passwords all the time.

    # do this once, so that you don't get asked for passwords any more.
    ssh-copy-id root@192.168.2.1

Upload and install the built binary package

    scp bin/*/packages/ws2812_sprites* root@192.168.2.1:
    ssh root@192.168.2.1:
    opkg install ws2812_sprites_1_ar71xx.ipk
     Installing ws2812_sprites (1) to root...
     Configuring ws2812_sprites.

Alternatively use the upload.sh script, which takes some shortcuts ...

    vi package/ws2812_sprites/src/ws2812_sprites.c
    ./upload.sh
