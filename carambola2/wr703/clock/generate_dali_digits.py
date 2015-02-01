#!/usr/bin/python
# (c) 2015 juewei@fabfolk.com
#
# digit pattern generator.

import time
import sys
import os
from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw
import StringIO

#Load a TTF font
# font = { 'size':13, 'off':[0,0.7], 'name':'Ubuntu-B.ttf' }
# font = { 'size':13, 'off':[0,0], 'name':'LiberationSans-Regular.ttf' }
font = { 'size':13, 'off':[1,0.5], 'name':'LiberationSans-Bold.ttf' }

font['font'] = ImageFont.truetype(font['name'], font['size'])


lines=9
columns=6

def gamma255_sq():
    r=[]
    for i in range(256):
        r.append(int(i*i/256))
    return r

# gamma_lut = gamma255_sq()
class img:
    w=columns
    h=lines
    gamma=gamma255_sq()

    def __init__(self, data):
	self.data=data

    def val(self, x, y):
	""" Return the value of a pixel. x,y must be integer coordinates.
	    Out of range pixels are returned as 0
	"""
	if (x < 0 or x >= self.w or
	    y < 0 or y >= self.h): return 0
	return self.data[self.w*y+x]

    def set(self, x, y, val):
	self.data[self.w*y+x] = val

    def sub_x_val(self, x, y):
	""" Return an interpolated subpixel value as a float [0..255].
            y must be integer, x can be float.
	    Out of range subpixels interpolate towards 0.
	""" 
	v1 = self.val(int(x)+0,y)
	v2 = self.val(int(x)+1,y)
        d = x - int(x) 
	return (1-d)*v1 + d*v2

    def ascii(self):
	"""Returns a crude ascii represntation as a printable string.
	   Each pixel is shown as a doubled character, so that it appears approximatly square.
	"""
        ascii_ramp = " .-:=+*#%@"
	out = ""
        for y in range(self.h):
            for x in range(self.w):
                v = self.data[self.w*y+x]
	        ch = ascii_ramp[v*len(ascii_ramp)/256]
		out += "%c%c" % (ch,ch)
	    out += "\n"
	return out


class textimg(img):
    def __init__(self, font, text):
        im = Image.new("L", (self.w, self.h), "black")
        draw = ImageDraw.Draw(im)
        fontwidth, fontheight = font['font'].getsize(text)
        # print fontwidth, fontheight

        # put text on its baseline, and horizontally centered.
        draw.text(((self.w-fontwidth)/2+font['off'][0],self.h-fontheight+font['off'][1]), text, 255, font=font['font'])

        if (self.gamma): 
     	    self.data=map(lambda x:self.gamma[x], list(im.getdata()))
	else:
	    self.data=list(im.getdata())



def lua_list(seq):
    return '{' + ','.join(map(lambda x:str(x),seq)) + '}'


def subpixel_offset(threshold, val1, val2):
    """Returns a float value in the range [0..1] if threshold is in the 
       range [val1..val2] representing the proportion.
       val1 and val2 can be in either order.
       Returns nil, if threshold is out of range.
    """
    if val1 < val2:
	if threshold < val1 or threshold > val2: return nil
	if val2 == val1: return val1
	return (threshold-val1)/(val2-val1)
    else:
	if threshold > val1 or threshold < val2: return nil
	if val2 == val1: return val1
	return (threshold-val2)/(val1-val2)


def interpolate_1d(f, v1, v2):
    if f < 0.0 or f > 1.0: raise ValueError
    return (1-f)*v1 + f*v2


def interpolate_dali(img1, img2, f):
    w = img1.w
    h = img1.h
    if img2.w != w: raise ValueError
    if img2.h != h: raise ValueError

    out = img([0] * (w*h))
    threshold=100
    for y in range(h):
        xstart1=0
	xstart2=0
	xend1=w-1
	xend2=w-1
        for x in range(w): 
            if img1.val(x,y) > threshold: xstart1 = x-subpixel_offset(threshold, img1.val(x,y), img1.val(x-1,y)); break
        for x in reversed(range(xstart1,w)): 
            if img1.val(x,y) > threshold: xend1   = x+subpixel_offset(threshold, img1.val(x,y), img1.val(x+1,y)); break
        for x in range(w):
            if img2.val(x,y) > threshold: xstart2 = x-subpixel_offset(threshold, img2.val(x,y), img2.val(x-1,y)); break
        for x in reversed(range(xstart2,w)): 
            if img2.val(x,y) > threshold: xend2   = x+subpixel_offset(threshold, img2.val(x,y), img2.val(x+1,y)); break
	# print "y=",y, xstart1,xend1, xstart2,xend2

        xstart = interpolate_1d(f, xstart1, xstart2)
        xend   = interpolate_1d(f, xend1,   xend2)

	if xend == xstart:
          if xstart > 0:
            xstart -= .1
          else:
            xend += .1

        scale1 = float(xend1-xstart1)/(xend-xstart)
        scale2 = float(xend2-xstart2)/(xend-xstart)

	for x in range(0, img1.w):
	    # x1 is x mapped into the corresponding float position in img1
	    # so that x1==xstart1 if x==xstart and so that x1==xend1 if x==xend.
	    # x2 is x mapped into the corresponding float position in img2
	    # so that x2==xstart2 if x==xstart and so that x2==xend2 if x==xend.

	    x1 = (x-xstart)*scale1+xstart1
	    x2 = (x-xstart)*scale2+xstart2

            # print(x, ":", xsrc1,xf1, w*y+min(xsrc1+1,w-1), '-', xsrc2,xf2, w*y+min(xsrc2+1,w-1) )
	    v = interpolate_1d(f, img1.sub_x_val(x1,y), img2.sub_x_val(x2,y))
            out.set(x,y,int(min(255, v)))
    return out


def morph_digit(img1, img2, steps):
    r = []
    for i in range(steps):
        m=interpolate_dali(img1, img2, float(i)/(steps-1))
	r.append(m)
        # print m.ascii()
        # print "--------------------"
    return r


def lua_morph_digit(imglist, digit, advance, nsteps):
    """ returns a list of lists as a lua-formatted string, containing
        { img[digit], ... interpolated steps ..., img[digit+advance] }
    """
    digit2 = (digit+advance) % 10
    # print imglist[digit].ascii()
    # print imglist[digit2].ascii()
    # print ""
    lua_lol = lua_list([lua_list(imglist[digit].data)] 
                     + map(lambda x:lua_list(x.data), morph_digit(imglist[digit], imglist[digit2], nsteps)) 
                     + [lua_list(imglist[digit2].data)])
    return lua_lol


steps=15
imglist=[]
for i in "0123456789":
    imglist.append(textimg(font, i))

dali_digits = """-- dali_digits.lua

-- CAUTION: autogenerated by """ + sys.argv[0] + """
--
-- dali_digits is a """ + str(columns) + ' x ' + str(lines) + """ rendering using the font
-- """ + font['name'] + " at size " + str(font['size']) + """. It is represented as 
-- antialiased grayscale maps with background 0 and forground 255.

-- counting in lua starts at index 1.
-- dali_digits[1] represents the digit '0' and its morphings toward digit '1'.
--  ... and so on ...
-- dali_digits[10] represents the digit '9' and its morphings toward digit '0'.

-- dali_digits[n][1] is the original digit, the morphings are in 
-- dali_digits[n][2] .. dali_digits[n][#dali_digits[n]] where the last element
-- is identical to dali_digits[(n+1)%10][1]

dali_digits = {
"""

for i in range(10):
    print imglist[i].ascii()
    
    dali_digits += "    " + lua_morph_digit(imglist, i, +1, steps)
    if (i < 9): dali_digits += ','
    dali_digits += "\n"
dali_digits += "}\n"

print font['name'] + ", 10 digits, morphing steps: ", steps
f=open('dali_digits.lua', 'w')
print >>f, dali_digits
f.close()

