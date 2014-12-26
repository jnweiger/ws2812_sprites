#!/usr/bin/python
#
# (c) 2014 jw@owncloud.com
#
# use the uart at higher baud rates and compile the bitstream for the ws2812
#
# We use a 74HTC02 as an inverter and 3.3V to 5V level shifter.
#
# This is not reliable:
# We add an open collector driver to the tx pin, which has two helpful effects
# - open collector can drive long lines, with the pullup on the receiving end.
# - the H-L transition enforced by the uart stopbit-startbit combination becomes
#   a useful L-H edge.
# J13, pin3 (tx)--- 10kohm --- base BFS17
# J13, pin5 (gnd) ------ emitter BFS17
# data line to led strip ---- collector BFS17
# At the led strip, bridge 1kohm between +5V VCC and data line.

import os, time,sys

# #define AR933X_UART_MAX_STEP    0x3333
# ... as suggested in http://permalink.gmane.org/gmane.comp.embedded.openwrt.devel/23013
# This value should range between 1310-13107 to maintain a better than +5% accuracy.
#

# serialClockFreq = 400Mhz
# (gdb) p 1310*400000/(131.072*800000.)
# $1 = 4.99725341796875
# (gdb) p 131.072*800000/(400000/(5.+1))
# $2 = 1572.8639999999998
#
# for 800khz we should get:
#  UART_CLOCK_SCALE=5
#  UART_CLOCK_STEP=1573
#  mmio -x 0x18020008 0x00050625
# for 3Mhz we should get:
#  UART_CLOCK_SCALE=1
#  UART_CLOCK_STEP=1966
#  mmio -x 0x18020008 0x000107ae
# but the values do not work. Frequency is far too low.
#
# Reverse enineering:
#
# These all create a 3.2Mhz bit clock, 0.3125us
# mmio -x 0x18020008 0x00002900
# mmio -x 0x18020008 0x00015200
# mmio -x 0x18020008 0x00027b80
#
# This creates a 2.4Mhz bit clock, 0.417us
# mmio -x 0x18020008 0x00001ec0
#
# This creates a 4Mhz bit clock, 0.25us
# mmio -x 0x18020008 0x00003380
#
# This creates a 4.255Mhz bit clock, 0.235us
# mmio -x 0x18020008 0x00003600

class CarambolaUartWS2812():
  def __init__(self, serial_port='/dev/ttyATH0'):
    self.ser = open(serial_port, 'w')
    os.system("mmio -x 0x18020008 0x00001ec0") # set 2.55Mhz bit clock, 0.417us
    ## os.system("mmio -x 0x18020008 0x00003600") # set 4.255Mhz bit clock, 0.235us
  
  class Pattern():

    doublebit = (
        # the uart sends the bits lsb first. and we invert all.
        chr(255-0x10),          # ^....1..._    0 0
        chr(255-0x70),          # ^....111._    0 1
        chr(255-0x13),          # ^11..1..._    1 0
        chr(255-0x73),          # ^11..111._    1 1
      )

    def __init__(self):
      self.data = ''
      
    def reset(self):
      self.data = ''
  
    def add(self, r, g, b):
      """byte order to send: g, r, b
         bit order to send: msb bits first.
      """

      for i in (6,4,2,0): self.data += self.doublebit[((g >> i) & 3)]
      for i in (6,4,2,0): self.data += self.doublebit[((r >> i) & 3)]
      for i in (6,4,2,0): self.data += self.doublebit[((b >> i) & 3)]
  
    def encode_rgb(self, r,g,b):
      """each rgb triple consumes 12 bytes"""
      x = []
      for i in (6,4,2,0): x.append(self.doublebit[((g >> i) & 3)])
      for i in (6,4,2,0): x.append(self.doublebit[((r >> i) & 3)])
      for i in (6,4,2,0): x.append(self.doublebit[((b >> i) & 3)])
      return x
    
    def set_rgb(self, pos, r,g,b):
      """each rgb triple consumes 12 bytes"""
      head = self.data[:12*pos]
      tail = self.data[12*pos+12:]
      self.data = head + ''.join(self.encode_rgb(r,g,b)) + tail
      
  def send(self, pattern):
    self.ser.write(pattern.data)
    self.ser.flush()
    time.sleep(.001)
    
  def close(self):
    self.ser.close()
    os.system("mmio -x 0x18020008 0x0024368F") # back to original 115200 baud
    
  def send(self, pattern):
    self.ser.write(pattern.data)
    self.ser.flush()
    time.sleep(.001)
    
  def close(self):
    self.ser.close()
    os.system("mmio -x 0x18020008 0x0024368F") # back to original 115200 baud

led = CarambolaUartWS2812()

def blink():
  p1 = led.Pattern()
  for i in range(8):
    p1.add(255,0,0)        # R
    p1.add(0,255,0)        # G
    p1.add(0,0,255)        # B
    p1.add(0,255,255)      # C
    p1.add(255,0,255)      # M
    p1.add(255,255,0)      # Y
    p1.add(0,0,0)          # K

  p2 = led.Pattern()
  for i in range(8):
    p2.add(0,255,0)        # G
    p2.add(0,0,255)        # B
    p2.add(0,255,255)      # C
    p2.add(255,0,255)      # M
    p2.add(255,255,0)      # Y
    p2.add(0,0,0)          # K
    p2.add(255,0,0)        # R

  p3 = led.Pattern()
  for i in range(8):
    p3.add(0,0,255)        # B
    p3.add(0,255,255)      # C
    p3.add(255,0,255)      # M
    p3.add(255,255,0)      # Y
    p3.add(0,0,0)          # K
    p3.add(255,0,0)        # R
    p3.add(0,255,0)        # G

  p4 = led.Pattern()
  for i in range(8):
    p4.add(0,255,255)      # C
    p4.add(255,0,255)      # M
    p4.add(255,255,0)      # Y
    p4.add(0,0,0)          # K
    p4.add(255,0,0)        # R
    p4.add(0,255,0)        # G
    p4.add(0,0,255)        # B

  p5 = led.Pattern()
  for i in range(8):
    p5.add(255,0,255)      # M
    p5.add(255,255,0)      # Y
    p5.add(0,0,0)          # K
    p5.add(255,0,0)        # R
    p5.add(0,255,0)        # G
    p5.add(0,0,255)        # B
    p5.add(0,255,255)      # C

  p6 = led.Pattern()
  for i in range(8):
    p6.add(255,255,0)      # Y
    p6.add(0,0,0)          # K
    p6.add(255,0,0)        # R
    p6.add(0,255,0)        # G
    p6.add(0,0,255)        # B
    p6.add(0,255,255)      # C
    p6.add(255,0,255)      # M

  p7 = led.Pattern()
  for i in range(8):
    p7.add(255,255,0)      # Y
    p7.add(0,0,0)          # K
    p7.add(255,0,0)        # R
    p7.add(0,255,0)        # G
    p7.add(0,0,255)        # B
    p7.add(0,255,255)      # C
    p7.add(255,0,255)      # M

  for i in range(40):
    for p in (p1, p2, p3, p4, p5, p6, p7):
      led.send(p)
      time.sleep(.04)


def orange_green():
  og = led.Pattern()
  for i in range(20):
    og.add(100,30,0) 
  for i in range(20):
    og.add(0,100,0) 
  og.add(10,10,10) 
  led.send(og)
  
def white():
  wh = led.Pattern()
  for i in range(50):
    wh.add(255,255,255)
  led.send(wh)
  
def red(length):
  rd = led.Pattern()
  for i in range(length):
    rd.add(70,0,0)
  led.send(rd)
 
def pingpong(length, rgb, speed):
  p = led.Pattern()
  for i in range(length):
    p.add(0,10,0)
  led.send(p)
  for j in range(2):
    for i in range(length):
      p.set_rgb(i, rgb[0], rgb[1], rgb[2])
      if i > 1:
        p.set_rgb(i-1, 1, 5, 1)
      led.send(p)
      time.sleep(.05)
    for i in reversed(range(length)):
      p.set_rgb(i, 1, 5, 1)
      if i > 1:
        p.set_rgb(i-1, rgb[0], rgb[1], rgb[2])
      led.send(p)
      time.sleep(.05)
  
def minimum():
  off = led.Pattern()
  for i in range(50):
    off.add(1,1,1) 
  led.send(off)

 
try:
  if len(sys.argv) > 1 and sys.argv[1] == 'blink':
    blink()
  elif len(sys.argv) > 1 and sys.argv[1] == 'orgn':
    orange_green()
  elif len(sys.argv) > 1 and sys.argv[1] == 'white':
    white()
  elif len(sys.argv) > 1 and sys.argv[1] == 'red':
    red(3*46)
  elif len(sys.argv) > 1 and sys.argv[1] == 'pingpong':
    pingpong(14+3*30, (255,0,0), 1)
  else:
    minimum()
except Exception as e:
  # just make sure we run led.close() 
  led.close()
  sys.exit(0)
  
led.close()
