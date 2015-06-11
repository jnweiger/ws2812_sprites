#!/usr/bin/python

import sys
a = open(sys.argv[1])
print a.readline()
print a.readline()
print a.readline()
print a.readline()
txt=sys.argv[1]+' = {{'
for y in range(9):
  for x in range(30):
    txt += "%d" % ord(a.read(1))
    if x < 29: txt += ','
  if y < 8: txt += '},{'
txt += "}}"
print txt
