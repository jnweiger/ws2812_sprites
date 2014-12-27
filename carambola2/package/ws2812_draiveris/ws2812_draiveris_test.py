#! /usr/bin/python
# FROM https://github.com/Lukse/carambola2_ws2812_driver/blob/master/Makefile
# 
# insmod ws2812_draiveris gpio_number=22
# mknod /dev/ws2812 c 253 0

import os

os.system("insmod ws2812_draiveris gpio_number=20 inverted=1")
os.system("mknod /dev/ws2812 c 253 0")
dev = os.open("/dev/ws2812", os.O_RDWR)
data = [0x00, 0x00, 0x03] * 66
os.write(dev, bytearray(data))

