#!/usr/bin/python

import posix
import struct
from ctypes import addressof, create_string_buffer, sizeof, string_at
from fcntl import ioctl
from spi_ctypes import *
import time

nleds=8

class spibus():
	fd=None
	write_buffer=create_string_buffer(nleds*24)
	read_buffer=create_string_buffer(nleds*24)

		# speed_hz=5000000,
		# speed_hz=2550000,  # 1.6us @ 4bit per bit
		# speed_hz=3500000,    # 1.2us @ 4bit per bit
	ioctl_arg = spi_ioc_transfer(
		tx_buf=addressof(write_buffer),
		rx_buf=addressof(read_buffer),
		len=1,
		delay_usecs=0,
		speed_hz=3500000,
		bits_per_word=8,
		cs_change = 0,
	)

	def __init__(self,device):
		self.fd = posix.open(device, posix.O_RDWR)
		ioctl(self.fd, SPI_IOC_RD_MODE, " ")
		ioctl(self.fd, SPI_IOC_WR_MODE, struct.pack('I',0))

	def send(self,len):
		self.ioctl_arg.len=len
		ioctl(self.fd, SPI_IOC_MESSAGE(1),addressof(self.ioctl_arg))

#Open the SPI bus 0
spibus0 = spibus("/dev/spidev32766.1")

def set_led(j, r, g, b):
  doublebit = [ 
    # spi sends the msb first, and we invert all
    0x77, 	# 111.111.	0 0
    0x71, 	# 1...1...	0 1
    0x17, 	# 1...111.	1 0
    0x11, 	# 1...1...	1 1
  ]

  # darkest colors
  # gn = [ 0x77, 0x77, 0x77, 0x71,  0x77, 0x77, 0x77, 0x77,   0x77, 0x77, 0x77, 0x77 ]
  # rd = [ 0x77, 0x77, 0x77, 0x77,  0x77, 0x77, 0x77, 0x71,   0x77, 0x77, 0x77, 0x77 ]
  # bl = [ 0x77, 0x77, 0x77, 0x77,  0x77, 0x77, 0x77, 0x77,   0x77, 0x77, 0x77, 0x71 ]

  i = j * 12

  spibus0.write_buffer[i+ 0] = chr(doublebit[(g>>6) & 3])
  spibus0.write_buffer[i+ 1] = chr(doublebit[(g>>4) & 3])
  spibus0.write_buffer[i+ 2] = chr(doublebit[(g>>2) & 3])
  spibus0.write_buffer[i+ 3] = chr(doublebit[(g>>0) & 3])

  spibus0.write_buffer[i+ 4] = chr(doublebit[(r>>6) & 3])
  spibus0.write_buffer[i+ 5] = chr(doublebit[(r>>4) & 3])
  spibus0.write_buffer[i+ 6] = chr(doublebit[(r>>2) & 3])
  spibus0.write_buffer[i+ 7] = chr(doublebit[(r>>0) & 3])

  spibus0.write_buffer[i+ 8] = chr(doublebit[(b>>6) & 3])
  spibus0.write_buffer[i+ 9] = chr(doublebit[(b>>4) & 3])
  spibus0.write_buffer[i+10] = chr(doublebit[(b>>2) & 3])
  spibus0.write_buffer[i+11] = chr(doublebit[(b>>0) & 3])

rgb = [
 [ 0 , 1 ,  0],
 [ 1 , 1 , 10],
 [ 1 , 1 , 30],
 [ 55, 55,  0],
 [ 55, 0 ,  0],
 [ 0 , 55,  0],
 [ 55,  0, 55],
 [ 55,  0,  0],
]

for x in range(len(rgb)):
  set_led(x, rgb[x][0], rgb[x][1], rgb[x][2])

dir=1
x=0
while (True):
  spibus0.send(3*4*8)
  set_led(x, rgb[x][0], rgb[x][1], rgb[x][2])
  if (x <= 0) : dir = 1
  if (x >= 7) : dir = -1
  x += dir
  set_led(x, 0,0,255)
  time.sleep(0.01)

  

#Shows the 2 byte received in full duplex in hex format
print hex(ord(spibus0.read_buffer[0]))
print hex(ord(spibus0.read_buffer[1]))



