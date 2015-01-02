#! /usr/bin/lua
--
-- Translated from python to lua. See ws2812_sprites.py
-- (C) 2015, juewei@fablab.com
--

socket=require('socket')
require('class')

os.execute("insmod ws2812-draiveris gpios=7,14,15 inverted=1")
socket.sleep(0.5)	-- give udev time to create the device
dev = io.open("/tmp/ws2812", "wb")
dev:setvbuf("no")

bgval=5
Walker = class({ panel_w=30, panel_h=9 })

function Walker:init(dx, dy, r, g, b, args)
    -- optional prameters:
    -- xbounce=False wraps left and right
    -- ybounce=False wraps top and bottom
    
    if type(args) == 'table' then
      xbounce, ybounce = args.xbounce, args.ybounce
      startx, starty = args.startx, args.starty
    end
    self.x,self.y,self.dx,self.dy = startx or 0, starty or 0, dx,dy
    self.r,self.g,self.b = r,g,b
    self.xbounce,self.ybounce = xbounce or false, ybounce or false
    if  dx >= self.panel_w or dx <= -self.panel_w then
      error("dx must be smaller than panel width="..self.panel_w)
    end
    if dy >= self.panel_h or dy <= -self.panel_h then
      error("dy must be smaller than panel height="..self.panel_h)
    end
end

function Walker:advance()	-- subclass me if you want to have more fancy movements.
    self.x = self.x + self.dx
    self.y = self.y + self.dy
end

function Walker:walk(rgb)
    self:advance()

    if self.xbounce then
      if self.x >= self.panel_w-1 then
        self.dx = -self.dx	-- pingpong
	self.x = 2*self.panel_w-2-self.x
      end
      if self.x < 0 then
        self.dx = -self.dx	-- pingpong
	self.x = -self.x
      end
    else
      if self.x >= self.panel_w then self.x = self.x - self.panel_w end
      if self.x < 0             then self.x = self.x + self.panel_w end
    end

    if self.ybounce then
      if self.y >= self.panel_h-1 then
        self.dy = -self.dy	-- pingpong
	self.y = 2*self.panel_h-2-self.y
      end
      if self.y < 0 then
        self.dy = -self.dy	-- pingpong
	self.y = -self.y
      end
    else
      if self.y >= self.panel_h then self.y = self.y - self.panel_h end
      if self.y < 0             then self.y = self.y + self.panel_h end
    end

    if rgb then self:draw(rgb) end
end 

function Walker:_paint_pixel(rgb, ix, iy, bright)
    r = math.floor(bright*self.r)
    g = math.floor(bright*self.g)
    b = math.floor(bright*self.b)
    if ix >= self.panel_w then
      if self.xbounce then ix = self.panel_w - 1
      else ix = 0 end
    end
    if iy >= self.panel_h then
      if self.ybounce then iy = self.panel_h - 1 
      else iy = 0 end
    end
    o = 3 * (ix + iy * self.panel_w)
    rgb[o+1] = rgb[o+1] + r
    rgb[o+2] = rgb[o+2] + g
    rgb[o+2] = rgb[o+3] + b
    if rgb[o+1] > 255 then rgb[o+1] = 255 end
    if rgb[o+2] > 255 then rgb[o+2] = 255 end
    if rgb[o+2] > 255 then rgb[o+3] = 255 end
end
    
function Walker:draw(rgb)
    ix = math.floor(self.x)
    iy = math.floor(self.y)
    xfrac = self.x - ix
    yfrac = self.y - iy
    self:_paint_pixel(rgb, ix+0, iy+0, (1-xfrac)*(1-yfrac))
    self:_paint_pixel(rgb, ix+1, iy+0, (  xfrac)*(1-yfrac))
    self:_paint_pixel(rgb, ix+0, iy+1, (1-xfrac)*(  yfrac))
    self:_paint_pixel(rgb, ix+1, iy+1, (  xfrac)*(  yfrac))
end 

function xy_pol(val, degree)
  -- same as rgb_hue, but for 2D-polar coordinates
  -- 0:x=val, 90:y=val, ...
  -- Using a 'good-enough-octagon' instead of proper trigonometry.
 
  wheel = { { 1,0}, { 0.7, 0.7}, {0, 1}, {-0.7, 0.7}, 
            {-1,0}, {-0.7,-0.7}, {0,-1}, { 0.7,-0.7}, {1,0} }
  degree = (degree % 360)/45.
  idegree = math.floor(degree)
  w1 = wheel[idegree+1]
  w2 = wheel[idegree+2]
  frac = degree-idegree
  x = w1[1]*(1-frac)+w2[1]*frac
  y = w1[2]*(1-frac)+w2[2]*frac
  return x*val, y*val
end

function rgb_hue(val, degree)
  -- degree is a value [0..359], it defines the hue of the color.
  --   0:red,  60:yellow,  120:green,  180:cyan,  240:blue, 300:magenta, (360:red)
  
  wheel = { {1,0,0}, {1,1,0}, {0,1,0}, {0,1,1}, {0,0,1}, {1,0,1}, {1,0,0} }
  degree = (degree % 360)/60.
  idegree = math.floor(degree)
  w1 = wheel[idegree+1]
  w2 = wheel[idegree+2]
  frac = degree-idegree
  r = w1[1]*(1-frac)+w2[1]*frac
  g = w1[2]*(1-frac)+w2[2]*frac
  b = w1[3]*(1-frac)+w2[3]*frac
  return math.floor(r*val), math.floor(g*val), math.floor(b*val)
end  

RotatingWalker = class(Walker)
RotatingWalker.xy_deg = 0

function RotatingWalker:advance()
    rspeed=0.11
    xspeed=-0.17
    scale=0.15
    self.xy_deg = (self.xy_deg+.8*rspeed) % 360
    x,y = xy_pol(scale*rspeed, self.xy_deg)
    self.x = self.x + x-xspeed
    self.y = self.y + y
end

walkers = {
  Walker(0.2,0.1, 	100,0,0,  { xbounce=True, ybounce=True } ),
  Walker(0.01,0.033, 	100,40,0, { ybounce=True } ),
  Walker(0.19,0.11, 	0,0,100),
  RotatingWalker(0.07,0.07, 	0,150,0)
}

deg=0
while true do
  r,g,b = rgb_hue(bgval, deg)
  data = {string.char(r,g,b):rep(30*9):byte(1, 3*30*90)} -- hack to rep() a table ..
  deg = deg + .3
  if deg > 360 then deg = deg - 360 end
  for _,w in pairs(walkers) do w:walk(data) end
  dev:write(string.char(table.unpack(data)))
  socket.sleep(0.02)
end
