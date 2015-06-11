#! /usr/bin/lua
--
-- Translated from python to lua. See ws2812_sprites.py
-- (C) 2015-06-11, juewei@fablab.com
--
-- portability: 5.1 only has unpack(), 5.2 has both table.unpack() and unpack()

socket=require('socket')
require('class')
require('fablab/FAB_30x9')
require('fablab/LAB_30x9')
require('fablab/FABLAB_30x9')

fablab = { FAB_30x9, LAB_30x9, FABLAB_30x9 }

os.execute("insmod ws2812-draiveris gpios=7,14,15 inverted=1")
socket.sleep(0.5)	-- give udev time to create the device
dev = io.open("/dev/ws2812", "wb")
dev:setvbuf("no")

bgval=1
Walker = class({ panel_w=30, max_x=30, panel_h=9 })

function Walker:init(dx, dy, r, g, b, args)
    -- optional prameters:
    -- xbounce=false wraps left and right
    -- ybounce=false wraps top and bottom
    
    if type(args) == 'table' then
      for key,val in pairs(args) do
          self[key] = val
      end
    end
    self.dx,self.dy      = dx,dy
    self.x,self.y        = startx or 0, starty or 0
    self.r,self.g,self.b = r,g,b
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

function Walker:walk(rgb, bright)
    self:advance()

    if self.xbounce then
      if self.x >= self.max_x-1 then
        self.dx = -self.dx	-- pingpong
	self.x = 2*self.max_x-2-self.x
      end
      if self.x < 0 then
        self.dx = -self.dx	-- pingpong
	self.x = -self.x
      end
    else
      if self.x >= self.max_x then self.x = self.x - self.max_x end
      if self.x < 0              then self.x = self.x + self.max_x end
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

    if rgb then self:draw(rgb, bright) end
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
    rgb[o+3] = rgb[o+3] + b
    if rgb[o+1] > 255 then rgb[o+1] = 255 end
    if rgb[o+2] > 255 then rgb[o+2] = 255 end
    if rgb[o+3] > 255 then rgb[o+3] = 255 end
end
    
function Walker:draw(rgb, bright)
    ix = math.floor(self.x)
    iy = math.floor(self.y)
    xfrac = self.x - ix
    yfrac = self.y - iy
    self:_paint_pixel(rgb, ix+0, iy+0, bright*(1-xfrac)*(1-yfrac))
    self:_paint_pixel(rgb, ix+1, iy+0, bright*(  xfrac)*(1-yfrac))
    self:_paint_pixel(rgb, ix+0, iy+1, bright*(1-xfrac)*(  yfrac))
    self:_paint_pixel(rgb, ix+1, iy+1, bright*(  xfrac)*(  yfrac))
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
  
  wheel = { {1,0,0}, {1,1,0}, {0,2,0}, {0,1,1}, {0,0,2}, {1,0,1}, {2,0,0} }
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

function veclen(x,y)
  return math.sqrt(x*x+y*y)
end

function fast_veclen(x,y)
  -- Similar to veclen, but not using a square root, 
  -- Do not use. It repeatedly over estimates, thus shortening normals over time.
  x,y = math.abs(x),math.abs(y)
  if x < y then x,y=y,x end
  return x+y/2
end

function vecreflect(x,y, nx,ny)
  -- reflect vector (x,y) at the wall normal (nx,ny)
  -- nx,ny is expeccted to be a normalized normal already.

  -- nl=veclen(nx,ny)
  -- nx, ny = nx/nl, ny/nl		-- normalize normal.
  dot = x*nx+y*ny
  return x-2*nx*dot, y-2*ny*dot
end


RotatingWalker = class(Walker)
RotatingWalker.xy_deg = 0

function RotatingWalker:advance()
    rspeed=0.85
    xspeed=0.017
    scale=0.06
    self.xy_deg = (self.xy_deg+rspeed) % 360
    x,y = xy_pol(scale, self.xy_deg)
    self.x = self.x + x + xspeed
    self.y = self.y + y
end

CollidingWalker = class(Walker)
function CollidingWalker:advance()
    self.x = self.x + self.dx
    self.y = self.y + self.dy
    if not(self.collide) then return end
    if not(self.ignore) then self.ignore={} end

    for _,w in ipairs(self.collide) do repeat
      if w==self then break end

      d = veclen(self.x-w.x, self.y-w.y)
      if d < 1.5 then
        if not(self.ignore[w]) then
          self.dx,self.dy = vecreflect(self.dx,self.dy, (self.x-w.x)/d,(self.y-w.y)/d)
          -- print("ding",self.dx,self.dy)
          self.ignore[w] = true
        end
      else
        self.ignore[w] = false
      end
    until true end	-- the break above is a continue in disguise.
end

r1,g1,b1 = 150,100,0
r2,g2,b2 = 150,50,0
r3,g3,b3 = 250,0,0

walkers = {
  CollidingWalker(0.16,0.10, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=30 } ),
  CollidingWalker(0.16,0.11, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=29 } ),
  CollidingWalker(0.16,0.12, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=28 } ),
  CollidingWalker(0.16,0.13, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=27 } ),
  CollidingWalker(0.16,0.14, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=26 } ),
  CollidingWalker(0.16,0.15, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=25 } ),
  CollidingWalker(0.16,0.16, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.16,0.17, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.16,0.18, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.16,0.19, 	r1,g1,b1,  { xbounce=true, ybounce=true, max_x=24 } ),

  CollidingWalker(0.15,0.12, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.13, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.14, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.15, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.16, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.17, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.18, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.19, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.15,0.10, 	r2,g2,b2,  { xbounce=true, ybounce=true, max_x=24 } ),

  CollidingWalker(0.11,0.1, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.12,0.1, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.10,0.1, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.13,0.1, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.14,0.1, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),

  CollidingWalker(0.13,0.11, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.10,0.11, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.12,0.11, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } ),
  CollidingWalker(0.11,0.11, 	r3,g3,b3,  { xbounce=true, ybounce=true, max_x=24 } )
}

CollidingWalker.collide = walkers

function render_digit(digit, width, data, xoff, r,g,b)
  -- r,g,b in range [0..1]
  for y = 1,9 do
    for x = 1,width do
      -- print("digit x="..x.." y="..y.." "..digit[9][1].." yeah")
      o = 1 + 3 * (xoff + (x-1) + 30 * (y-1))
      -- print(i, digit[i], o)
      rr = math.floor(digit[y][x] * r + data[o+0])
      gg = math.floor(digit[y][x] * g + data[o+1])
      bb = math.floor(digit[y][x] * b + data[o+2])
      if rr > 255 then rr = 255 end
      if gg > 255 then gg = 255 end
      if bb > 255 then bb = 255 end
      data[o+0] = rr
      data[o+1] = gg
      data[o+2] = bb
    end
  end
end

page     = 1
wait     = 0

ddeg=0
deg=0
while true do
  r,g,b = rgb_hue(bgval, deg)
  if false then
    -- this string conversion is expensive. The for loop below initializes data much cheaper.
    -- overall system load is below 0.3 with the loop, and exceeds 0.4 with this hack.
    data = {string.char(r,g,b):rep(30*9):byte(1, 3*30*90)} -- hack to rep() a table ..
  else
    data = {}
    for i = 1,3*30*9,3 do
      data[i+0], data[i+1], data[i+2] = r,g,b
    end
  end

  dlum=0.9
  walker_bright = 1
  dr, dg, _ = xy_pol(0.5*dlum, ddeg)
  if page < 4 then
    render_digit(fablab[page], 30, data, 0,  dr+0.5*dlum, dg+0.5*dlum, dlum-dr+0.5*dlum)
    walker_bright = 0.3
  end
  if page == 5 then
    render_digit(fablab[3], 30, data, 0,  dr+0.5*dlum, dg+0.5*dlum, dlum-dr+0.5*dlum)
    walker_bright = 0.3
  end
  if page == 7 then
    render_digit(fablab[3], 30, data, 0,  dr+0.5*dlum, dg+0.5*dlum, dlum-dr+0.5*dlum)
    walker_bright = 0.3
  end

  wait = wait + 1
  if wait > 20 then
    wait = 0
    page = page + 1
    if page > 20 then
      page = 1
    end
  end

  deg = deg + 3
  if deg > 360 then deg = deg - 360 end
  ddeg = ddeg + .11
  if ddeg > 360 then ddeg = ddeg - 360 end

  for _,w in pairs(walkers) do w:walk(data, walker_bright) end
  dev:write(string.char(unpack(data)))
  -- socket.sleep(0.05)
end
