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

text=sys.argv[1]
lines=9
columns=6
im = Image.new("RGB", (columns, lines), "blue")
#Create a draw object to draw primitives on the new image 
draw = ImageDraw.Draw(im)
fontwidth, fontheight = font.getsize(text)
print fontwidth, fontheight

# put text on its baseline, and horizontally centered.
draw.text(((columns-fontwidth)/2,lines-fontheight+.5), text, (255,255,255), font=font)

output = StringIO.StringIO()

#a RGB byte array
output.truncate(0)
im.save(output, format='PPM')
buf=output.getvalue()

out_file = open("output.ppm","w")
out_file.seek(0)
#Discard the first 13 header byte and use just the RGB
#byte array
# out_file.write(buf[13:])
out_file.write(buf)
out_file.close()

