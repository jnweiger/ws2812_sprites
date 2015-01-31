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
    # print fontwidth, fontheight

    # put text on its baseline, and horizontally centered.
    draw.text(((columns-fontwidth)/2,lines-fontheight+.5), text, 255, font=font)
    return list(im.getdata())

def print_ascii(img, w, h):
    ascii_ramp = " .-:=+*#%@"
    stride = w
    for y in range(h):
        for x in range(w):
            v = img[stride*y+x]
	    ch = ascii_ramp[v*len(ascii_ramp)/256]
	    sys.stdout.write("%c%c" % (ch,ch))
	print ""
 
def interpolate_1d(f, v1, v2):
    if (f < 0.0 or f > 1.0): raise ValueError
    return (1-f)*v1 + f*v2

def interpolate_dali(img1, img2, w, h, f):
    out=[0] * (w*h)
    threshold=50
    for y in range(h):
        xstart1=0
	xstart2=0
	xend1=w-1
	xend2=w-1
        for x in range(w): 
            if img1[w*y+x] > threshold: xstart1=x; break
        for x in reversed(range(xstart1,w)): 
            if img1[w*y+x] > threshold: xend1=x; break
        for x in range(w):
            if img2[w*y+x] > threshold: xstart2=x; break
        for x in reversed(range(xstart2,w)): 
            if img2[w*y+x] > threshold: xend2=x; break
	# print "y=",y, xstart1,xend1, xstart2,xend2

        xstart=int(interpolate_1d(f, xstart1, xstart2)+0.5)
        xend=int(interpolate_1d(f, xend1, xend2)+0.5)

        stepw1=float(xend1-xstart1)/(xend-xstart)
        stepw2=float(xend2-xstart2)/(xend-xstart)

	for x in range(xstart, xend+1):
            xsrc1=(x-xstart)*stepw1+xstart1
            xf1=xsrc1-int(xsrc1)
            xsrc1=int(xsrc1)

            xsrc2=(x-xstart)*stepw2+xstart2
            xf2=xsrc2-int(xsrc2)
            xsrc2=int(xsrc2)

            # print(x, ":", xsrc1,xf1, w*y+min(xsrc1+1,w-1), '-', xsrc2,xf2, w*y+min(xsrc2+1,w-1) )
	    v = interpolate_1d(f,
		  interpolate_1d(xf1, img1[w*y+xsrc1],img1[ w*y+min(xsrc1+1,w-1) ]),
		  interpolate_1d(xf2, img2[w*y+xsrc2],img2[ w*y+min(xsrc2+1,w-1) ]))
            out[w*y+x] = int(min(255, v))
    return out


img=[]
for i in "0123456789":
    img.append(render_text(columns, lines, font, i))

# print(img)

steps=10
digit=1
print_ascii(img[digit], columns, lines)  
print ""
print_ascii(img[(digit+1)%10], columns, lines)  
print ""
for i in range(steps):
    m=interpolate_dali(img[digit], img[(digit+1)%10], columns, lines, float(i)/(steps-1))
    print_ascii(m, columns, lines)  
    print "--------------------"
