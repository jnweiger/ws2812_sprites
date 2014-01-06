#!/bin/sh
set -x
# IPADDR=192.168.178.30
IPADDR=192.168.2.1
cd $HOME/src/github/carambola2/
make package/ws2812_sprites/compile V=s || exit
ssh root@$IPADDR killall ws2812_sprites
ls -la staging_dir/target-mips_r2_uClibc-0.9.33.2/root-ar71xx/bin/ws2812_sprites 
scp    staging_dir/target-mips_r2_uClibc-0.9.33.2/root-ar71xx/bin/ws2812_sprites root@$IPADDR:/bin
ssh root@$IPADDR time /bin/ws2812_sprites
