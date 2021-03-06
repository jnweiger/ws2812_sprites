#! /usr/bin/python
# FROM https://github.com/Lukse/carambola2_ws2812_driver/blob/master/Makefile
# 
# insmod ws2812_draiveris gpio_number=22
# mknod /dev/ws2812 c 152 0

import os, time, math

os.system("insmod ws2812-draiveris gpios=20,21,22 inverted=1")
# os.system("mknod /dev/ws2812 c 152 0")
dev = os.open("/dev/ws2812", os.O_RDWR)

bgval=5

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

  def advance(self):	# subclass me if you want to have more fancy movements.
    self.x += self.dx
    self.y += self.dy

  def walk(self,rgb=None):
    self.advance()

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
    rgb[o+0] += r
    rgb[o+1] += g
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
   

def xy_pol(val, degree):
  """ same as rgb_hue, but for 2D-polar coordinates
      0:x=val, 90:y=val, ...
      Using a 'good-enough-octagon' instead of proper trigonometry.
  """
  wheel = ( [ val,0], [ 0.7*val, 0.7*val], [0, val], [-0.7*val, 0.7*val], 
            [-val,0], [-0.7*val,-0.7*val], [0,-val], [ 0.7*val,-0.7*val], [val,0] )
  degree = (degree % 360)/45.
  w1 = wheel[int(degree)]
  w2 = wheel[int(degree)+1]
  frac = degree-int(degree)
  x = w1[0]*(1-frac)+w2[0]*frac
  y = w1[1]*(1-frac)+w2[1]*frac
  return [x,y]

def rgb_hue(val, degree):
  """degree is a value [0..359], it defines the hue of the color.
     0:red,  60:yellow,  120:green,  180:cyan,  240:blue, 300:magenta, (360:red)
  """
  wheel = ( [1,0,0], [1,1,0], [0,1,0], [0,1,1], [0,0,1], [1,0,1], [1,0,0] )
  degree = (degree % 360)/60.
  w1 = wheel[int(degree)]
  w2 = wheel[int(degree)+1]
  frac = degree-int(degree)
  r = w1[0]*(1-frac)+w2[0]*frac
  g = w1[1]*(1-frac)+w2[1]*frac
  b = w1[2]*(1-frac)+w2[2]*frac
  return [int(r*val),int(g*val),int(b*val)]

def veclen(x,y):
  return math.sqrt(x*x+y*y)

def fast_veclen(x,y):
  """ Similar to veclen, but not using a square root, 
      Do not use. It repeatedly over estimates, thus shortening normals over time.
  """
  x,y = abs(x),abs(y)
  if x < y: x,y=y,x
  return x+y/2

def vecreflect(x,y, nx,ny):
  """
   reflect vector (x,y) at the wall normal (nx,ny)
   nx,ny is expeccted to be a normalized normal already.

   nl=veclen(nx,ny)
   nx, ny = nx/nl, ny/nl		-- normalize normal.
  """
  dot = x*nx+y*ny
  return x-2*nx*dot, y-2*ny*dot


class rotating_walker(walker):
  xy_deg = 0

  def advance(self):
    rspeed=0.85
    xspeed=0.017
    scale=0.06
    self.xy_deg = (self.xy_deg+rspeed) % 360
    (x,y)=xy_pol(scale, self.xy_deg)
    self.x += x + xspeed
    self.y += y

class colliding_walker(walker):
  collide=None
  ignore={}

  def advance(self):
    self.x = self.x + self.dx
    self.y = self.y + self.dy
    if self.ignore is None: return
    
    for w_idx in range(len(self.collide)):
      w = self.collide[w_idx]
      if w == self: continue
      d = veclen(self.x-w.x, self.y-w.y)
      if d < 1.5:
        if not w_idx in self.ignore  or  not self.ignore[w_idx]:
          self.dx,self.dy = vecreflect(self.dx,self.dy, (self.x-w.x)/d,(self.y-w.y)/d)
          # print "ding",self.dx,self.dy
          self.ignore[w_idx]=True
      else:
        self.ignore[w_idx]=False


walkers = (
  walker(0.09,0.04, 	100,0,60,  xbounce=True, ybounce=True),
  walker(0.01,0.033, 	100,40,0, ybounce=True),
  walker(0.19,0.11, 	0,0,100),
  rotating_walker(0.07,0.07, 	0,150,0),
  colliding_walker(0.12,0.1, 	150,0,0,  xbounce=True, ybounce=True)
)

walkers[-1].collide = walkers


deg=0
while True:
  data  = rgb_hue(bgval, deg) * 30 * 9;
  deg += .3
  if deg > 360: deg -= 360 
  for w in walkers: w.walk(data)
  os.write(dev, bytearray(data))
  time.sleep(0.02)
