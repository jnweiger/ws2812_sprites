#! /usr/bin/python

import os, time

os.system("insmod ws2812-draiveris gpio_number=20 inverted=1")
time.sleep(0.3)	# until the device appears

dev = os.open("/dev/ws2812", os.O_RDWR)
y = "\3\3\0"	# very low intensity yellow
b = "\0\0\3"	# very low intensity blue

while True:
  os.write(dev, bytearray((y+b+b+b+b)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+y+b+b+b)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+b+y+b+b)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+b+b+y+b)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+b+b+b+y)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+b+b+y+b)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+b+y+b+b)*12))
  time.sleep(0.1)
  os.write(dev, bytearray((b+y+b+b+b)*12))
  time.sleep(0.1)
