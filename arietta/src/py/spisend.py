#!/usr/bin/python

import posix
import struct
from ctypes import addressof, create_string_buffer, sizeof, string_at
from fcntl import ioctl
from spi_ctypes import *
import time


class spibus():
	fd=None
	write_buffer=create_string_buffer(50)
	read_buffer=create_string_buffer(50)

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
spibus0.write_buffer[ 0]=chr(0x55)
spibus0.write_buffer[ 1]=chr(0x55)
spibus0.write_buffer[ 2]=chr(0x55)
spibus0.write_buffer[ 3]=chr(0x55)
spibus0.write_buffer[ 4]=chr(0x55)
spibus0.write_buffer[ 5]=chr(0x55)
spibus0.write_buffer[ 6]=chr(0x55)
spibus0.write_buffer[ 7]=chr(0x55)

spibus0.write_buffer[ 8]=chr(0x33)
spibus0.write_buffer[ 9]=chr(0x33)
spibus0.write_buffer[10]=chr(0x33)
spibus0.write_buffer[11]=chr(0x33)
spibus0.write_buffer[12]=chr(0x33)
spibus0.write_buffer[13]=chr(0x33)
spibus0.write_buffer[14]=chr(0x33)
spibus0.write_buffer[15]=chr(0x33)

spibus0.write_buffer[16]=chr(0xAA)
spibus0.write_buffer[17]=chr(0xAA)
spibus0.write_buffer[18]=chr(0xAA)
spibus0.write_buffer[19]=chr(0xAA)
spibus0.write_buffer[20]=chr(0xAA)
spibus0.write_buffer[21]=chr(0xAA)
spibus0.write_buffer[22]=chr(0xAA)
spibus0.write_buffer[23]=chr(0xAA)

spibus0.send(24)

#Shows the 2 byte received in full duplex in hex format
print hex(ord(spibus0.read_buffer[0]))
print hex(ord(spibus0.read_buffer[1]))



