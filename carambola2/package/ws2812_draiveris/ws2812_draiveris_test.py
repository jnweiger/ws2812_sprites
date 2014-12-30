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

class walker:
  panel_w=30
  panel_h=9

  def __init__(self, dx, dy, r, g, b, xbounce=False, ybounce=False, startx=0, starty=0):
    """ xbounce=False wraps left and right
        ybounce=False wraps top and bottom
    """
    (self.x,self.y,self.dx,self.dy)=(startx,starty,dx,dy)
    (self.r,self.g,self.b)=(r,g,b)
    (self.xbounce,self.ybounce)=(xbounce,ybounce)
    if (dx >= self.panel_w or dx <= -self.panel_w): 
      raise ValueError("dx must be smaller than panel width="+str(self.panel_w))
    if (dy >= self.panel_h or dy <= -self.panel_h): 
      raise ValueError("dy must be smaller than panel height="+str(self.panel_h))


  def walk(self,rgb=None):
    self.x += self.dx
    self.y += self.dy

    if self.xbounce:
      if (self.x >= self.panel_w-1): 
        self.dx = -self.dx	# pingpong
	self.x = 2*self.panel_w-2-self.x
      if (self.x < 0):             
        self.dx = -self.dx	# pingpong
	self.x = -self.x
    else:
      if (self.x >= self.panel_w): self.x -= self.panel_w   
      if (self.x < 0):             self.x += self.panel_w

    if self.ybounce:
      if (self.y >= self.panel_h-1): 
        self.dy = -self.dy	# pingpong
	self.y = 2*self.panel_h-2-self.y
      if (self.y < 0):             
        self.dy = -self.dy	# pingpong
	self.y = -self.y
    else:
      if (self.y >= self.panel_h): self.y -= self.panel_h
      if (self.y < 0):             self.y += self.panel_h

    if rgb: self.draw(rgb)
 

  def _paint_pixel(self, rgb, ix, iy, bright):
    r = int(bright*self.r)
    g = int(bright*self.g)
    b = int(bright*self.b)
    if ix >= self.panel_w: 
      if self.xbounce: ix = self.panel_w - 1
      else: ix = 0
    if iy >= self.panel_h: 
      if self.ybounce: self.panel_hh -1
      else: iy = 0
    o = 3 * (ix + iy * self.panel_w)
    rgb[o+0] += g
    rgb[o+1] += r
    rgb[o+2] += b
    if rgb[o+0] > 255: rgb[o+0] = 255
    if rgb[o+1] > 255: rgb[o+1] = 255
    if rgb[o+2] > 255: rgb[o+2] = 255
    
  def draw(self, rgb):
    ix = int(self.x)
    iy = int(self.y)
    xfrac = self.x - ix
    yfrac = self.y - iy
    self._paint_pixel(rgb, ix+0, iy+0, (1-xfrac)*(1-yfrac))
    self._paint_pixel(rgb, ix+1, iy+0, (  xfrac)*(1-yfrac))
    self._paint_pixel(rgb, ix+0, iy+1, (1-xfrac)*(  yfrac))
    self._paint_pixel(rgb, ix+1, iy+1, (  xfrac)*(  yfrac))
   

walkers = (
  walker(0.2,0.1, 	100,0,0,  xbounce=True, ybounce=True),
  walker(0.01,0.033, 	100,40,0, ybounce=True),
  walker(0.19,0.11, 	0,0,200),
)


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
    for w in walkers: w.walk(data)
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
    for w in walkers: w.walk(data)
    os.write(dev, bytearray(data))
    time.sleep(0.02)

