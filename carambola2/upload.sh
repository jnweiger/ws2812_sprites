#!/bin/sh
#
# Preconditions:
# Power up the carambola2. Wait 100 seconds, green led starts blinking slow.
#
# Search for wlan SSID jw_blinkenlights, take dhcp
#
# Test ssh root@192.168.2.1, it should work without password prompt.
# if not, then do
# ssh-copy-id root@192.168.2.1,
# ssh root@192.168.2.1 cp .ssh/authorized_keys /etc/dropbear/authorized_keys
#
# Set the syscomp scope to 5uS timebase, otherwise it will not trigger.
#
# Also try this:
# ssh -v root@192.168.2.1 'while true; do python uart.py blink; done'
#
set -x
# IPADDR=192.168.178.30
IPADDR=192.168.2.1
cd $HOME/src/github/carambola2/
make package/ws2812_sprites/compile V=s || exit
ssh root@$IPADDR killall ws2812_sprites
ls -la staging_dir/target-mips_r2_uClibc-0.9.33.2/root-ar71xx/bin/ws2812_sprites 
scp    staging_dir/target-mips_r2_uClibc-0.9.33.2/root-ar71xx/bin/ws2812_sprites root@$IPADDR:/bin
ssh root@$IPADDR time /bin/ws2812_sprites
