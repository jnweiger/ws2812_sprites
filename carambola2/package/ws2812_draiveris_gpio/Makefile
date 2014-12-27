# Copyright (C) 2012 Žilvinas Valinskas, Saulius Lukšė
# Copyright (C) 2014 Jürgen Weigert <jw@owncloud.com>
# See LICENSE for more information.
# 
# Forked from https://github.com/Lukse/carambola2_ws2812_driver
#
# References: http://www.adafruit.com/datasheets/WS2812.pdf
#
## PC Build single package
# cd /home/test/carambola2/
# clear; make package/ws2812_draiveris/clean
# clear; make package/ws2812_draiveris/compile V=99

# PC: Carambola picocom
# sudo picocom -b 115200 /dev/ttyUSB0

# Carambola: get packet
## rmmod ws2812_draiveris; 
## rm /lib/modules/3.7.9/ws2812_draiveris.ko; 
## cd /tmp; rm kmod-ws2812_draiveris*; 
## wget http://192.168.0.63:8000/packages/kmod-ws2812_draiveris_3.7.9-5_ar71xx.ipk; 
## opkg install kmod-ws2812_draiveris_3.7.9-5_ar71xx.ipk --force-reinstall;
## insmod /lib/modules/3.7.9/ws2812_draiveris.ko gpio_number=20 inverted=1
## mknod /dev/ws2812 c 253 0
## echo -en '\x00\x00\x40' > /dev/ws2812

# test.py
##dev = os.open("/dev/ws2812", os.O_RDWR)
##data = [0x0, 0x60, 0x00] 
##os.write(dev, bytearray(data))

# upgrade firmware
## cd /tmp; wget http://192.168.0.63:8000/openwrt-ar71xx-generic-carambola2-squashfs-sysupgrade.bin; sysupgrade -v -n openwrt-ar71xx-generic-carambola2-squashfs-sysupgrade.bin

# carambola install python
## opkg update
## opkg install python

# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-ws2812_draiveris
PKG_RELEASE:=5

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ws2812_draiveris
	UBMENU:=Other modules
	TITLE:=ws2812 draiveris drivers
	FILES:= \
	$(PKG_BUILD_DIR)/ws2812_draiveris.ko

	AUTOLOAD:=$(call AutoLoad, 92, ws2812_draiveris-dev)
endef

define KernelPackage/ws2812_draiveris/description
	This package contains ws2812_draiveris drivers for
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	ARCH="$(LINUX_KARCH)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(BUILDFLAGS)" \
	modules
endef

define KernelPackage/ws2812_draiveris/install
	$(INSTALL_DIR) $(1)/lib/network/
endef

$(eval $(call KernelPackage,ws2812_draiveris))
