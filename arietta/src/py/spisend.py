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

#Send 24 characters

for i in range(nleds):
  j=i*24
  spibus0.write_buffer[ 0+j]=chr(0x55)
  spibus0.write_buffer[ 1+j]=chr(0x55)
  spibus0.write_buffer[ 2+j]=chr(0x55)
  spibus0.write_buffer[ 3+j]=chr(0x55)
  spibus0.write_buffer[ 4+j]=chr(0x55)
  spibus0.write_buffer[ 5+j]=chr(0x55)
  spibus0.write_buffer[ 6+j]=chr(0x55)
  spibus0.write_buffer[ 7+j]=chr(0x55)
  
  spibus0.write_buffer[ 8+j]=chr(0x33)
  spibus0.write_buffer[ 9+j]=chr(0x33)
  spibus0.write_buffer[10+j]=chr(0x33)
  spibus0.write_buffer[11+j]=chr(0x33)
  spibus0.write_buffer[12+j]=chr(0x33)
  spibus0.write_buffer[13+j]=chr(0x33)
  spibus0.write_buffer[14+j]=chr(0x33)
  spibus0.write_buffer[15+j]=chr(0x33)
  
  spibus0.write_buffer[16+j]=chr(0xAA)
  spibus0.write_buffer[17+j]=chr(0xAA)
  spibus0.write_buffer[18+j]=chr(0xAA)
  spibus0.write_buffer[19+j]=chr(0xAA)
  spibus0.write_buffer[20+j]=chr(0xAA)
  spibus0.write_buffer[21+j]=chr(0xAA)
  spibus0.write_buffer[22+j]=chr(0xAA)
  spibus0.write_buffer[23+j]=chr(0xAA)

spibus0.send(24*nleds)

#Shows the 2 byte received in full duplex in hex format
print hex(ord(spibus0.read_buffer[0]))
print hex(ord(spibus0.read_buffer[1]))



