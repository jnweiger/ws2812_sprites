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
		speed_hz=2550000,
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
    # uart sends the lsb first, and we invert all
    (255-0x11), 	# 1...1...	0 0
    (255-0x71), 	# 1...111.	0 1
    (255-0x17), 	# 111.1...	1 0
    (255-0x77), 	# 111.111.	1 1
  ]

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

set_led(0, 1  , 1  , 1)
set_led(1, 1  , 1  , 20)
set_led(2, 1  , 1  , 30)
set_led(3, 255, 255, 0  )
set_led(4, 255, 0  , 0  )
set_led(5, 0  , 255, 0  )
set_led(6, 255, 0  , 255)
set_led(7,   5,   5, 255)
set_led(8,   5,   5, 255)


gn = [ 0x77, 0x77, 0x77, 0x71,  0x77, 0x77, 0x77, 0x77,   0x77, 0x77, 0x77, 0x77 ]
rd = [ 0x77, 0x77, 0x77, 0x77,  0x77, 0x77, 0x77, 0x71,   0x77, 0x77, 0x77, 0x77 ]
bl = [ 0x77, 0x77, 0x77, 0x77,  0x77, 0x77, 0x77, 0x77,   0x77, 0x77, 0x77, 0x71 ]

for j in range(12): spibus0.write_buffer[12*0+j] = chr(gn[j])
# for j in range(12): spibus0.write_buffer[12*1+j] = chr(bl[j])
# for j in range(12): spibus0.write_buffer[12*2+j] = chr(rd[j])

spibus0.send(3*4*9)	# nleds)

#Shows the 2 byte received in full duplex in hex format
print hex(ord(spibus0.read_buffer[0]))
print hex(ord(spibus0.read_buffer[1]))



