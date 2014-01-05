Build instructions
------------------


 git clone https://github.com/8devices/carambola2.git
 
 cd carambola2
 
 # good build instructions are here:
 
 less README

 ./scripts/feeds update
 
 ./scripts/feeds install
 
 make menuconfig
 
 make V=s

 cd package
 
 ln -s ~/src/github/ws2812_sprites/carambola2/package/ws2812_sprites .
 
 echo >> .config CONFIG_PACKAGE_ws2812_sprites=y
 
 make oldconfig
 
 make package/ws2812_sprites/compile V=99
 
 make package/ws2812_sprites/install V=99
 
 ls -l bin/*/packages/ws2812_sprites*
 
  -rw-r--r-- 1 testy users 1913 Jan  3 01:48 bin/ar71xx/packages/ws2812_sprites_1_ar71xx.ipk

 # do this once, so that you don't get asked for passwords any more.

 # ssh-copy-id root@192.168.2.1

 scp bin/*/packages/helloworld* root@192.168.2.1:

 ssh root@192.168.2.1:

 opkg install ws2812_sprites_1_ar71xx.ipk

  Installing ws2812_sprites (1) to root...

  Configuring ws2812_sprites.

 # or use the ./upload script, which takes some shortcuts...

