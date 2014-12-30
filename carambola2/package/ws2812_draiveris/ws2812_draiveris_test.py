#! /usr/bin/python
# FROM https://github.com/Lukse/carambola2_ws2812_driver/blob/master/Makefile
# 
# insmod ws2812_draiveris gpio_number=22
# mknod /dev/ws2812 c 152 0

import os, time

os.system("insmod ws2812-draiveris gpios=20,21,22 inverted=1")
# os.system("mknod /dev/ws2812 c 152 0")
dev = os.open("/dev/ws2812", os.O_RDWR)
#       green  red  blue

val=5
while True:
  for i in range(30):
    data  = [1,1,0]*(i) + [0,0,val] + [1,1,0]*(29-i)
    data += [1,1,0]*(i) + [0,val,0] + [1,1,0]*(29-i)
    data += [1,1,0]*(i) + [val,0,0] + [1,1,0]*(29-i)

    data += [0,0,3]*(i) + [val,val,0] + [0,0,3]*(29-i)
    data += [0,0,3]*(29-i)+[val,0,val] + [0,0,3]*(i)
    data += [0,0,3]*(i) + [0,val,val] + [0,0,3]*(29-i)

    data += [3,0,0]*(i) + [val,val,val] + [3,0,0]*(29-i)
    data += [3,0,0]*(i) + [val>>2,val>>2,val>>2] + [3,0,0]*(29-i)
    data += [3,0,0]*(i) + [30-i,30-i,i] + [3,0,0]*(29-i)
    os.write(dev, bytearray(data))
    time.sleep(0.02)

  for i in range(30):
    data  = [1,1,0]*(29-i) + [0,0,val] + [1,1,0]*(i)
    data += [1,1,0]*(29-i) + [0,val,0] + [1,1,0]*(i)
    data += [1,1,0]*(29-i) + [val,0,0] + [1,1,0]*(i)

    data += [0,1,0]*(29-i) + [val,val,0] + [0,1,0]*(i)
    data += [0,1,0]*(i)    + [val,0,val] + [0,1,0]*(29-i)
    data += [0,1,0]*(29-i) + [0,val,val] + [0,1,0]*(i)

    data += [3,0,0]*(29-i) + [val,val,val] + [3,0,0]*(i)
    data += [3,0,0]*(29-i) + [val>>2,val>>2,val>>2] + [3,0,0]*(i)
    data += [3,0,0]*(29-i) + [i,i,30-i] +    [3,0,0]*(i)
    os.write(dev, bytearray(data))
    time.sleep(0.02)

