# NeoPixel library strandtest example
# Author: Tony DiCola (tonlllllllllllllllly@tonydicola.com)
#
# Direct port of the Arduino NeoPixel library strandtest example.  Showcases
# various animations on a strip of NeoPixels.

import numpy
import random
import math

import alsaaudio, time, audioop, datetime

from neopixel import *


# LED strip configuration:
LED_COUNT   = 209       # Number of LED pixels.
LED_PIN     = 18      # GPIO pin connected to the pixels (must support PWM!).
LED_FREQ_HZ = 800000  # LED signal frequency in hertz (usually 800khz)
LED_DMA     = 5       # DMA channel to use for generating signal (try 5)
LED_INVERT  = True   # True to invert the signal (when using NPN transistor level shift)


# Define functions which animate LEDs in various ways.
def colorWipe(strip, color, wait_ms=50):
	"""Wipe color across display a pixel at a time."""
	for i in range(strip.numPixels()):
		strip.setPixelColor(i, color)
		#time.sleep(wait_ms/1000.0)

	strip.show()

def theaterChase(strip, color, wait_ms=50, iterations=10):
	"""Movie theater light style chaser animation."""
	for j in range(iterations):
		for q in range(3):
			for i in range(0, strip.numPixels(), 3):
				strip.setPixelColor(i+q, color)
			strip.show()
			time.sleep(wait_ms/1000.0)
			for i in range(0, strip.numPixels(), 3):
				strip.setPixelColor(i+q, 0)

def wheel(pos):
	"""Generate rainbow colors across 0-255 positions."""
	if pos < 85:
		return Color(pos * 3, 255 - pos * 3, 0)
	elif pos < 170:
		pos -= 85
		return Color(255 - pos * 3, 0, pos * 3)
	else:
		pos -= 170
		return Color(0, pos * 3, 255 - pos * 3)

def rainbow(strip, wait_ms=20, iterations=1):
	"""Draw rainbow that fades across all pixels at once."""
	for j in range(256*iterations):
		for i in range(strip.numPixels()):
			strip.setPixelColor(i, wheel((i+j) & 255))
		strip.show()
		time.sleep(wait_ms/1000.0)

def rainbowCycle(strip, wait_ms=20, iterations=5):
	"""Draw rainbow that uniformly distributes itself across all pixels."""
	for j in range(256*iterations):
		for i in range(strip.numPixels()):
			strip.setPixelColor(i, wheel(((i * 256 / strip.numPixels()) + j) & 255))
		strip.show()
		time.sleep(wait_ms/1000.0)

def theaterChaseRainbow(strip, wait_ms=50):
	"""Rainbow movie theater light style chaser animation."""
	for j in range(256):
		for q in range(3):
			for i in range(0, strip.numPixels(), 3):
				strip.setPixelColor(i+q, wheel((i+j) % 255))
			strip.show()
			time.sleep(wait_ms/1000.0)
			for i in range(0, strip.numPixels(), 3):
				strip.setPixelColor(i+q, 0)

def clip(sample):
	"""Clips a value greater then 255"""
	if(sample > 255):
		return 255
	elif sample < 0:
		return 0
	else:
		return sample

class walker:
	'Common base class for all walkers'
	Count = 0

	def __init__(self, initalPosition, color, length, stripLength):
      		self.position = initalPosition
		self.length = length
      		walker.Count += 1
		self.stripLength = stripLength
		self.SSmpls = 8

	def mov(self, deltapos):
		self.position += deltapos

	def chngLength(self, length):
		self.length = length

	def render(self):
		ssWalker= [0.0 for x in range(self.stripLength*self.SSmpls)]
		ssStart = int( self.position * self.SSmpls )
		ssLength = int( self.length * self.SSmpls )

		if ssStart + ssLength > self.stripLength*self.SSmpls:
			self.position = 0.0
			ssStart = 0

		#for i in range(ssStart,ssEnd):
		#	ssWalker[i] = 255.0
			
    		for i in range(ssStart,ssStart + ssLength / 2):
			ssWalker[i] = ((i-ssStart)/(self.length*self.SSmpls/2.0)) * 255.0

    		for i in range(ssStart + ssLength / 2,ssStart + ssLength):
			ssWalker[i] = (2.0 - ((i-ssStart)/(self.length*self.SSmpls/2.0))) * 255.0

		rStrip = [Color(0,0,0) for x in range(self.stripLength)]
		j = 0

		for i in range((self.stripLength-1)*self.SSmpls):
			rStrip[j] += ssWalker[i]
			if (i%self.SSmpls) == 0:
				#rStrip[j] = Color(int( clip( abs( (1.0*rStrip[j])/self.SSmpls ))),
				#		   int( clip( abs( (1.0*rStrip[j])/self.SSmpls ))),
				#		   int( clip( abs( (1.0*rStrip[j])/self.SSmpls ))))

				rStrip[j] = Color( int( clip(  (1.0*rStrip[j])/self.SSmpls )),
						   int( clip(  (1.0*rStrip[j])/self.SSmpls - self.length*255/16.0)),
						   int( clip(  (1.0*rStrip[j])/self.SSmpls - self.length*255/16.0)))

				j += 1
		
		rStrip[len(rStrip)-1] = Color(0,0,0)

		return rStrip

'''
Listing One
 void ScaleLine(PIXEL *Target, PIXEL *Source, int SrcWidth, int TgtWidth)
{
  int NumPixels = TgtWidth;
  int IntPart = SrcWidth / TgtWidth;
  int FractPart = SrcWidth % TgtWidth;
  int E = 0;
  while (NumPixels-- > 0) {
    *Target++ = *Source;
    Source += IntPart;

    E += FractPart;
    if (E >= TgtWidth) {
      E -= TgtWidth;
      Source++;
    } /* if */
  } /* while */
}
Back to Article
'''


		
def scaleLine(tgtPixelsSize , srcPixels):
	tgtPixels = [0 for x in range(tgtPixelsSize)]

	#print "tgtPixelsSize: ", tgtPixelsSize
	#print "srcPixels: ", len(srcPixels)
	
	epsilon = 0.000001
	scale = (1.0*(len(srcPixels)-1)/(len(tgtPixels)-1+epsilon))


	for tgtIndex in range(len(tgtPixels)):
		estSrcIndex = tgtIndex * scale
		srcIndex = int(estSrcIndex)
		delta = estSrcIndex - srcIndex

		#print srcIndex, tgtIndex, estSrcIndex
		tgtPixels[tgtIndex] = (1-delta)*srcPixels[srcIndex]+delta*srcPixels[srcIndex+1]
		tgtPixels[tgtIndex] = tgtPixels[tgtIndex] * tgtPixels[tgtIndex] * 0.0000001

	return tgtPixels

# Main program logic follows:
if __name__ == '__main__':
	# Create NeoPixel object with appropriate configuration.
	strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT)
	# Intialize the library (must be called once before other functions).
	strip.begin()

	card = 'sysdefault:CARD=Set'
        inp = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NONBLOCK, card)
        inp.setchannels(1)
	inp.setrate(4000)
        inp.setformat(alsaaudio.PCM_FORMAT_S16_LE)
	#inp.setperiodsize(160)

	print 'Press Ctrl-C to quit.'

	#MyWalker = walker(0, Color(255,255,255) , 8, strip.numPixels() );
	#t = 0.0
	#autoModulator = 0

	while True:
		
                length,data = inp.read()
		#print length
		

		if length & int(length) > 0:
			#print 'Cycle Start with Audio#Bytes:'
			#print length
			try:
				a = numpy.fromstring(data, dtype='int16')
			except:
				print 'Strange behavior (buffer empty ?)'

	
			ft = abs(numpy.fft.fft(a))
			ft = ft[16:69]
			
			
			#if autoModulator % 8 == 0:
			#	amC = 0
			#	maxA = numpy.max(a)

			#ft = 0.0000001*(ft*ft)
			
			#print nft

			strechedFT = scaleLine(strip.numPixels() , ft)
			#print strechedFT, "\n"

			for i in range(len(strechedFT)):
				strip.setPixelColor(i,Color(	clip( int( abs( strechedFT[i]))),
								clip( int( abs( strechedFT[i]))),
								clip( int( abs( strechedFT[i])))));
			
			
			strip.show()
			#autoModulator += 1
			
		
		#MyWalker.mov(0.8)
		#l = abs(math.sin(t))
		#MyWalker.chngLength(int(16.0*l))
		#spriteWalker = MyWalker.render()
		
		#t += 0.1

		#print spriteWalker

		#for i in range(strip.numPixels()):
		#	strip.setPixelColor(i,spriteWalker[i])

		#strip.show()

		#perlinNoise(strip, 0)

		#mean = 0
                #print mean
                #colorWipe(strip, Color( 255 - mean, mean, 255) ); #random shit
                
		# Color wipe animations.
		#colorWipe(strip, Color(127, 32, 0))  # Red wipe
		#time.sleep(0.05)
		#colorWipe(strip, Color(0, 255, 0))  # Blue wipe
		#time.sleep(0.05)
		#colorWipe(strip, Color(0, 0, 255))  # Green wipe
		#time.sleep(0.05)
		#colorWipe(strip, Color(0, 0, 0))  # Black wipe

		#print 'Cycle Complete'

		#colorWipe(strip, Color(0, 0, 255))  # Green wipe
		# Theater chase animations.
		#theaterChase(strip, Color(127, 127, 127))  # White theater chase
		#theaterChase(strip, Color(127,   0,   0))  # Red theater chase
		#theaterChase(strip, Color(  0,   0, 127))  # Blue theater chase
		# Rainbow animations.
		#rainbow(strip)
		#rainbowCycle(strip)
		#theaterChaseRainbow(strip)
