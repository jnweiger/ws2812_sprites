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
font = ImageFont.truetype('Ubuntu-B.ttf', 13)

lines=9
columns=6

def render_text(columns, lines, font, text):
    im = Image.new("L", (columns, lines), "black")
    draw = ImageDraw.Draw(im)
    fontwidth, fontheight = font.getsize(text)
    print fontwidth, fontheight

    # put text on its baseline, and horizontally centered.
    draw.text(((columns-fontwidth)/2,lines-fontheight+.5), text, 255, font=font)
    return list(im.getdata())

def print_ascii(img, w, h):
    ascii_ramp = " .-:=+*#%@"
    stride = w
    print len(img), stride
    for y in range(h):
        for x in range(w):
            v = img[stride*y+x]
	    ch = ascii_ramp[v*len(ascii_ramp)/256]
	    print "%c%c" % (ch,ch),
	print ""
 

img=[]
for i in "0123456789":
    img.append(render_text(columns, lines, font, i))

# print(img)


for i in range(1):
    print_ascii(img[0], columns, lines)  
    print "--------------------"
