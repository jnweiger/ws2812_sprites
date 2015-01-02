#! /usr/bin/lua
--
-- Translated from python to lua. See ws2812_draiveris_test.py
-- (C) 2015, juewei@fablab.com
--

bit=require('bit')
socket=require('socket')

os.execute("insmod ws2812-draiveris gpios=7,14,15 inverted=1")
socket.sleep(0.5)	-- give udev time to create the device
dev = io.open("/dev/ws2812", "wb")
dev:setvbuf("no")

function rep3(r,g,b, n)
  if n <= 0 then return "" end
  s = string.char(r,g,b)
  return string.rep(s,n)
end

width=30
val=5
v_2=bit.rshift(val,2)

while true do

  for i = 0, width-1 do
    data =         rep3(1,1,0, i) .. rep3(val,0,0, 1) .. rep3(1,1,0, width-1-i)
    data = data .. rep3(1,1,0, i) .. rep3(0,val,0, 1) .. rep3(1,1,0, width-1-i)
    data = data .. rep3(1,1,0, i) .. rep3(0,0,val, 1) .. rep3(1,1,0, width-1-i)

    data = data .. rep3(0,0,3, i)         .. rep3(0,val,val, 1) .. rep3(0,0,3, width-1-i)
    data = data .. rep3(0,0,3, width-1-i) .. rep3(val,0,val, 1) .. rep3(0,0,3, i)
    data = data .. rep3(0,0,3, i)         .. rep3(val,val,0, 1) .. rep3(0,0,3, width-1-i)

    data = data .. rep3(0,3,0, i) .. rep3(v_2,v_2,v_2, 1)       .. rep3(0,3,0, width-1-i)
    data = data .. rep3(0,3,0, i) .. rep3(val,val,val, 1)       .. rep3(0,3,0, width-1-i)
    data = data .. rep3(0,3,0, i) .. rep3(width-i,width-i,i, 1) .. rep3(0,3,0, width-1-i)
    dev:write(data)
    socket.sleep(0.02)
  end

  for i = 0, width-1 do
    data =         rep3(1,1,0, width-1-i) .. rep3(val,0,0, 1) .. rep3(1,1,0, i)
    data = data .. rep3(1,1,0, width-1-i) .. rep3(0,val,0, 1) .. rep3(1,1,0, i)
    data = data .. rep3(1,1,0, width-1-i) .. rep3(0,0,val, 1) .. rep3(1,1,0, i)

    data = data .. rep3(1,0,0, width-1-i) .. rep3(0,val,val, 1) .. rep3(1,0,0, i)
    data = data .. rep3(1,0,0, i)         .. rep3(val,0,val, 1) .. rep3(1,0,0, width-1-i)
    data = data .. rep3(1,0,0, width-1-i) .. rep3(val,val,0, 1) .. rep3(1,0,0, i)

    data = data .. rep3(0,3,0, width-1-i) .. rep3(v_2,v_2,v_2, 1) .. rep3(0,3,0, i)
    data = data .. rep3(0,3,0, width-1-i) .. rep3(val,val,val, 1) .. rep3(0,3,0, i)
    data = data .. rep3(0,3,0, width-1-i) .. rep3(i,i,width-i, 1) .. rep3(0,3,0, i)
    dev:write(data)
    socket.sleep(0.02)
  end

end
dev:close()

