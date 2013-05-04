// (C) 2013, jw@suse.de, GPL-2.0
//
// ws2812.c 
// a driver for ws2812, using bit banging at 16mhz.
// 
// https://www.olimex.com/Products/Duino/AVR/AVR-T32U4/
//  PB7 <D11> aka OC0A // unavailable, OC0A is used as TOP.
//  PD0 <D3>  aka OC0B 
//  PD5 <PD5> green TXLED
//  PB0 <D14> yellow RXLED
//  PF0 <A5>  gn, out bitbang WS2812, far half
//  PF1 <A4>  ye, out bitbang WS2812, near half
//  PC7 <D13> far switch
//  PD1 <D2>  near switch
//
// Button pins: 
// White: Switch
// Brown: GND
// green/yellow: LEDs
//
// http://www.mikrocontroller.net/topic/271872
// http://www.aliexpress.com/item/WS2811IC-Digital-RGB-led-strip-5m-input-12V-300pcs-IC-300pcs-SMD5050-led-IP65-waterproof-free/672648273.html
// http://de.aliexpress.com/item/4m-WS2811-LED-digital-strip-60leds-m-with-60pcs-WS2811-built-in-tthe-5050-smd-rgb/633124138.html
//
// http://partfusion.com/wp-uploads/2013/01/WS2812preliminary.pdf
//  T0H 0 code pulse: 0.35us +/- 0.15us
//  T0L 0 code pause: 0.8us  +/- 0.15us
//  T1H 1 code pulse: 0.7us  +/- 0.15us
//  T1L 1 code pause: 0.6us  +/- 0.15us
// 
// 2013-02-23, jw@suse.de initial draugth, hw pwm is too slow.
// 2013-03-05, bit banging. send_leds() done.
// 2013-03-06, bit banging with a ws2812_refresh(hz) timer. Now we are fast enough.
// 2013-03-07, sprite machine drafted.
// 2013-03-20, sprite machine up and running. synchronous refreshes.
//             8x supersampling coded, but has no effect, huh?
// 2013-03-21, far, near button added, sprite flag added. Range checks done. 
//             Wrapping works.
// 2013-03-22, 8x supersampling fixed. (4x is not enough, it has an ugly flicker)
// 
// 2013-04-03, dual channel mode added and optimized. SEND_BIT_S2Dnm is the winner.
//             Second channel repeats a portion of the first for testing.
//             Cost 1.25:1, adjusted vel's to compensate.
// 2013-05-04, soft flash on far button. Direction change on near button.
//             sprite->scale added. #defined BLAUE_NACHT added.
//
// 100 LEDS, at 5V:
// 	  1% white  0.2A
// 	 12% white  0.6A
// 	 25% white  1.2A
// 	 50% white  2.0A
// 	100% red    1.7A 
// 	100% or     2.0A 
// 	100% yellow 2.2A 
// 	100% green  1.5A 
// 	100% blue   1.6A 
// 	100% cyan   2.2A 
// 	100% magenta 2.2A 
//


#define F_CPU 16000000
#include <util/delay.h>			// needs F_CPU from cpu_mhz.h
#include <avr/io.h>
#include <avr/interrupt.h>		// sei()
#include <string.h>			// memset()

#define NSPRITES	20
#define LED2POS(n)	(((int32_t)(n))<<16)
#define POS2LED(n)	(((int32_t)(n))>>16)	// signed rshift! negative pos are valid!

#define BLAUE_NACHT	1
#define MICROSTEPS	8		// 8, 4, 2, 1 suppported.

#define VEL_SCALE(n)	(((int32_t)(n))*256)

#define BUTH_PORT	PORTE
#define BUTH_PIN	PINE
#define BUTH_DDR	DDRE
#define BUTH_BITS	(1<<PE2)

#define BUTN_PORT	PORTD
#define BUTN_PIN	PIND
#define BUTN_DDR	DDRD
#define BUTN_BITS	(1<<PD1)

#define BUTF_PORT	PORTC
#define BUTF_PIN	PINC
#define BUTF_DDR	DDRC
#define BUTF_BITS	(1<<PC7)

#define TXLED_PORT 	PORTD
#define TXLED_DDR  	DDRD
#define TXLED_BITS	(1<<PD5)

#define RXLED_PORT 	PORTB
#define RXLED_DDR  	DDRB
#define RXLED_BITS	(1<<PB1)

#define WS2812_PORT 	PORTF
#define WS2812_DDR  	DDRF
#define WS2812_BITS  	(1<<PF0)	// Arduino A5
#define WS2812_BITS2  	(1<<PF1)	// Arduino A4
#define WS2812_MASK 	(WS2812_BITS | WS2812_BITS2)
#define WS2812_NLEDS	240	// FIXMES: add offset for b
#define WS2812_NLEDSa	240
#define WS2812_NLEDSb	234

#define ON   WS2812_PORT |=  WS2812_BITS		// arduino A5
#define OFF  WS2812_PORT &= ~WS2812_BITS		// 2 #clocks: sbi(), cbi()

#define ON2  WS2812_PORT |=  WS2812_BITS2		// arduino A4
#define OFF2 WS2812_PORT &= ~WS2812_BITS2		// 2 #clocks: sbi(), cbi()

#define L00  WS2812_PORT = 0
#define L01  WS2812_PORT = WS2812_BITS2
#define L10  WS2812_PORT = WS2812_BITS
#define L11  WS2812_PORT = WS2812_MASK

#define _M(a)	do { a; } while (0)

#define SET_RGB(_ptr, _r, _g, _b) _M(*(_ptr)++ = (_g); *(_ptr)++ = (_r); *(_ptr)++ = (_b) )

#define SETpBK	SET_RGB(p,   0,  0,  0)

#define SETpRE	SET_RGB(p, 255,  0,  0)
#define SETpRE1	SET_RGB(p,  64,  0,  0)
#define SETpRE2	SET_RGB(p,  16,  0,  0)
#define SETpRE3	SET_RGB(p,   4,  0,  0)

#define SETpGR	SET_RGB(p,   0,255,  0)
#define SETpGR1	SET_RGB(p,   0, 64,  0)
#define SETpGR2	SET_RGB(p,   0, 16,  0)
#define SETpGR3	SET_RGB(p,   0,  4,  0)

#define SETpBL	SET_RGB(p,   0,  0,255)
#define SETpBL1	SET_RGB(p,   0,  0, 64)
#define SETpBL2	SET_RGB(p,   0,  0, 16)
#define SETpBL3	SET_RGB(p,   0,  0,  4)

#define SETpYE	SET_RGB(p, 255,145, 16)
#define SETpYE1	SET_RGB(p, 128, 80,  8)
#define SETpYE2	SET_RGB(p,  64, 38,  0)
#define SETpYE3	SET_RGB(p,  32, 22,  0)

#define SETpCY	SET_RGB(p,   0,255,255)
#define SETpCY1	SET_RGB(p,   0,128,128)
#define SETpCY2	SET_RGB(p,   0, 64, 64)
#define SETpCY3	SET_RGB(p,   0, 32, 32)

#define SETpMA	SET_RGB(p, 255,  0,255)
#define SETpMA1	SET_RGB(p, 128,  0,128)
#define SETpMA2	SET_RGB(p,  64,  0, 64)
#define SETpMA3	SET_RGB(p,  32,  0, 32)

#define SETpOR	SET_RGB(p, 255, 63,  0)
#define SETpOR1	SET_RGB(p, 128, 32,  0)
#define SETpOR2	SET_RGB(p,  64, 24,  0)
#define SETpOR3	SET_RGB(p,  32, 12,  0)

#define SETpGY8	SET_RGB(p,   1,  1,  1)
#define SETpGY7	SET_RGB(p,   2,  2,  2)
#define SETpGY6	SET_RGB(p,   4,  4,  4)
#define SETpGY5	SET_RGB(p,   8,  8,  8)
#define SETpGY4	SET_RGB(p,  16, 16, 16)
#define SETpGY3	SET_RGB(p,  32, 32, 32)
#define SETpGY2	SET_RGB(p,  64, 64, 64)
#define SETpGY1	SET_RGB(p, 128,128,128)

#define SETpWH	SET_RGB(p, 255,191,127)

#if F_CPU == 16000000
// This is robust: Making ON period one more or one less is still ok.
// Same for OFF period. Good.
# define SEND_BIT_S2_save(on) _M( ON; ON; if ((on)) { ON;ON;ON;} else {OFF;OFF;} OFF; OFF; )
# define SEND_BIT_S2_sav2(on) _M( ON; ON; if ((on)) { ON;ON; } OFF; OFF; )
# define SEND_BIT_S2(on) _M( ON; ON; if ((on)) { ON;ON; } OFF; OFF; )
# define SEND_BIT_S2D(on1, on2) _M( ON; ON; if ((on1)) { ON; ON; } OFF; ON2; ON2; if ((on2)) { ON2; ON2; } OFF2; )

// # define SEND_BIT_S2D00 _M( L11;L11;L11;L11;L00;L00;L00;L00; )		// 30sec red roundtrip
// # define SEND_BIT_S2D01 _M( L11;L11;L11;L11;L01;L01;L01;L01;L00; )
// # define SEND_BIT_S2D10 _M( L11;L11;L11;L11;L10;L10;L10;L10;L00; )
// # define SEND_BIT_S2D11 _M( L11;L11;L11;L11;L11;L11;L11;L11;L00; )

// the other strip needs longer on times, than what I measured above
# define SEND_BIT_S2D00 _M( L11;L11;L11;L11;L11;L00;L00;L00;L00; )		// 30sec red roundtrip
# define SEND_BIT_S2D01 _M( L11;L11;L11;L11;L11;L01;L01;L01;L01;L00; )
# define SEND_BIT_S2D10 _M( L11;L11;L11;L11;L11;L10;L10;L10;L10;L00; )
# define SEND_BIT_S2D11 _M( L11;L11;L11;L11;L11;L11;L11;L11;L11;L00; )
#else
# error "SEND_BIT_S2 only defined for 16Mhz"
#endif

// uint8_t buffer[3*numleds];
void send_leds(uint8_t *buffer, uint16_t numleds, uint8_t *buffer2, uint16_t numleds2)
{
  int16_t i = numleds*3;
  int16_t j = numleds2*3;
  int16_t m = (j > i) ? j : i;
  for (; m >=0; m--)
    {
      uint8_t val =  (i-- > 0) ? *buffer++  : 0;
      uint8_t val2 = (j-- > 0) ? *buffer2++ : 0;

      uint8_t bit;
      for (bit = 0; bit < 8; bit++)
        {
#if 1	// DUAL_CHAIN				
	  // 30sec red roundtrip with SEND_BIT_S2Dnm;
	  // 33sec red roundtrip with SEND_BIT_S2D(n,m);
	  if (val & 0b10000000)
	    { 
	      if (val2 & 0b10000000)
	        SEND_BIT_S2D11;
	      else
	        SEND_BIT_S2D10;
	    }
	  else
	    {
	      if (val2 & 0b10000000)
	        SEND_BIT_S2D01;
	      else
	        SEND_BIT_S2D00;
	    }
#else
	  SEND_BIT_S2(val & 0b10000000);	// 25sec red roundtrip
#endif
	  val <<= 1;
	  val2 <<= 1;
	}
    }
  // _delay_us(50);
}

static void inline fixup_bootloader()
{
  // BUG ALERT: the default bootloader of 
  // olimex AVR-T32U4 boards leaves us some bits in UDIEN enabled,
  // this will crash, when we just say sei()
  // We manually disable what the bootloader should have done.
  USBCON = 0;	// disable USBE, VBUSTE
  USBINT = 0;	// disable VBUSTI
  UDIEN  = 0;	// disable UPRSME, EORSME, WAKEUPE, EORSTE, SOFE, SUSPE
  UEIENX = 0;	// disable NAKINE, NAKOUTE

  PORTB &= ~0x01;	// turn off forgotten RXLED
  PORTD &= ~0x20;	// turn off forgotten TXLED
}

uint8_t rgb[WS2812_NLEDS * 3];
uint8_t refresh_hz;

#if 0
ISR(TIMER1_CAPT_vect)		// IRC1
{
  static int n;
  send_leds(rgb, WS2812_NLEDS);
  if ((n++ * 2) == (refresh_hz & 0xfe))
    {
      PORTB |=  0x01;		// 	RXLED on
    } 
  else if (n == refresh_hz)
    {
      PORTB &= ~0x01;		// 	RXLED off
      n = 0;
    }
}

void ws2812_refresh(uint8_t hz)
{
  refresh_hz = hz;
  cli();
  // Mode 12: WGM=1100 CTC TOP=ICR1A
  TCCR1A = 0b00000000;	// WGM10=00
  TCCR1B = 0b00011101;	// WGM32=11, CS210=101 = prescaler 1:1024
  ICR1 = 15625/hz;	// 50hz @ 16mhz
  TIMSK1 = 0b00100000;	// enable Capt1_vector

  sei();
}
#endif


struct sprite
{
#define SPRITE_F_WRAP      0x01		// if pos+len overlaps end, make it reappear at start
#define SPRITE_F_COLLIDE_A 0x02		// 3 different layers, where collisions may happen
#define SPRITE_F_COLLIDE_B 0x04		// if (s1->flags & SPRITE_F_COLL_MASK & s2->flags)
#define SPRITE_F_COLLIDE_C 0x08		//    s1 and s2 collide
#define SPRITE_F_COLL_MASK (SPRITE_F_COLLIDE_C|SPRITE_F_COLLIDE_B|SPRITE_F_COLLIDE_A)
#define SPRITE_F_INF_MASS  0x10		// this sprite 'wins' collisions.
  uint8_t flags;	// wrap, collision layer
  uint8_t len;		// how many pixels the sprite is wide
  uint8_t scale;	// integer upscale. Use 0 to hide the sprite.
  uint8_t *rgb;		// 3*len bytes of R,G,B data
  int32_t pos;		// current position of the sprite 
  int16_t vel;		// current velocity of the sprite
  void (*move)(struct sprite *, uint8_t);	// advance method
  void *data;		// optional data for move()
};

void sprite_wraparound(struct sprite *s, uint8_t speed)
{
  // Tricky: can have a sprite half and half on the edges
  // FIXME: The wrap is not perfectly smooth. Suspect an off-by-one error somewhere.
  int16_t svel = (BUTN_PIN & BUTN_BITS) ? s->vel : -s->vel;
  s->pos += (int32_t)svel * (1<<speed);
  if (svel < 0)
    {
      if (s->pos < 0)
        {
	  s->pos = LED2POS(WS2812_NLEDS);
	}
    }
  else
    {
      if (s->pos >= LED2POS(WS2812_NLEDS))
        {
	  s->pos = 0;
	}
    }
}

void sprite_ping_pong(struct sprite *s, uint8_t speed)
{
  int16_t svel = (BUTN_PIN & BUTN_BITS) ? s->vel : -s->vel;
  s->pos += (int32_t)svel * (1<<speed);

  if (svel < 0)
    {
      if (s->pos < 0)
        {
	  s->pos = 0;
	  s->vel = -s->vel;
	}
    }
  else
    {
      if (s->pos > LED2POS(WS2812_NLEDS-s->len))
        {
	  s->pos = LED2POS(WS2812_NLEDS-s->len);
	  s->vel = -s->vel;
	}
    }
}

struct sprite sprite[NSPRITES];
uint8_t nsprites = 0;

void simulate_world(uint8_t speed)
{
  uint8_t i;

  for (i = 0; i < nsprites; i++)
    {
      struct sprite *s = sprite+i;
      if (s->move)
        (s->move)(s, speed);
    }
}

// uint8_t buffer[3*numleds];
// Downshift reduces brightness. It should be 0, unless we are supersampling...
void render_frame(uint8_t *buffer, uint8_t downshift, int16_t spread)
{
  // put all sprites to their respective position, 
  // TODO: add motion blurr for vel, and interpolate between neighbouring pixels
  uint8_t i;
  int16_t spread_pos = (LED2POS(1)>>downshift)*spread;

  for (i = 0; i < nsprites; i++)
    {
      uint8_t l;
      struct sprite *s = sprite+i;
      uint8_t *pb = buffer+3*POS2LED(s->pos+spread_pos);
      uint8_t *be = buffer+3*WS2812_NLEDS;
      uint8_t *ps = s->rgb;
      for (l = s->len; l > 0; l--)
        {
	  uint8_t rep = s->scale;
	  while (rep-- > 0)
	    {
	      uint8_t colors = 0;
	      while (colors++ < 3)
		{
		  // add light quantity of this sprite, clipping at bright white
		  uint16_t v = (uint16_t)*pb + (*ps++ >> downshift);
		  if (v > 255) v = 255;		// clip at white
		  if (pb < buffer) 		// safety first
		    pb++;
		  else if (pb < be) 		// safety last
		    *pb++ = v;	
		  else
		    pb++;
		  if (pb == be && (s->flags & SPRITE_F_WRAP)) 
		    pb = buffer;
		}
	      if (rep) ps -= 3;
	    }
	}
    }
}

int main()
{
  fixup_bootloader();
  // ws2812_refresh(75);

  BUTH_DDR   &= ~(BUTH_BITS);	// input
  BUTH_PORT  |=  (BUTH_BITS);	// pullup
  BUTN_DDR   &= ~(BUTN_BITS);	// input
  BUTN_PORT  |=  (BUTN_BITS);	// pullup
  BUTF_DDR   &= ~(BUTF_BITS);	// input
  BUTF_PORT  |=  (BUTF_BITS);	// pullup

  TXLED_DDR  |= TXLED_BITS;	// output
  RXLED_DDR  |= RXLED_BITS;	// output
  WS2812_DDR |= WS2812_MASK;	// output

  uint8_t flash = 0;
  uint8_t i;
  uint8_t *p;
  // create a sprite
  uint8_t flame_sprite[3*13];
  uint8_t green_puck[3*6];
  uint8_t blue_puck[3*6];
#ifdef BLAUE_NACHT
  uint8_t blue_wave[3*32];
  uint8_t yellow_puck[3*8];
#endif
  uint8_t red_puck[3*6];
  p = flame_sprite;
	SETpRE3; SETpRE2; SETpRE1; SETpRE; SETpOR; SETpYE; SETpWH; 
	SETpYE; SETpOR; SETpRE; SETpRE1; SETpRE2; SETpRE3;
  p = green_puck;
  	SETpGR2; SETpGR1; SETpGR; SETpGR; SETpGR1; SETpGR2;
  p = blue_puck;
  	SETpBL2; SETpBL1; SETpBL; SETpBL; SETpBL1; SETpBL2;
  p = red_puck;
  	SETpRE2; SETpRE1; SETpRE; SETpRE; SETpRE1; SETpRE2;
  p = yellow_puck;
  	SETpYE3; SETpYE2; SETpYE1; SETpYE; SETpYE; SETpYE1; SETpYE2; SETpYE3;

#ifdef BLAUE_NACHT
  p = blue_wave; 
  for (i = 1; i <= 8; i++) { SET_RGB(p, 0, 0, (uint16_t)255*i>>3); }
  for (i = 1; i <= 8; i++) { SET_RGB(p, (uint16_t)47*i>>3, (uint16_t)63*i>>3, 255); }
  for (i = 8; i >= 1; i--) { SET_RGB(p, (uint16_t)47*i>>3, (uint16_t)63*i>>3, 255); }
  for (i = 8; i >= 1; i--) { SET_RGB(p, 0, 0, (uint16_t)255*i>>3); }
  sprite[nsprites].len = 32;
  sprite[nsprites].scale = 3;
  sprite[nsprites].rgb = blue_wave;
  sprite[nsprites].pos = LED2POS(11);
  sprite[nsprites].vel = 81;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  sprite[nsprites].len = 32;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = blue_wave;
  sprite[nsprites].pos = LED2POS(55);
  sprite[nsprites].vel = 111;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  sprite[nsprites].len = 32;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = blue_wave;
  sprite[nsprites].pos = LED2POS(11);
  sprite[nsprites].vel = 120;
  sprite[nsprites].move = sprite_ping_pong;
  sprite[nsprites].flags = 0;
  nsprites++;

  sprite[nsprites].len = 6;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = blue_puck;
  sprite[nsprites].pos = LED2POS(111);
  sprite[nsprites].vel = 520;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  sprite[nsprites].len = 6;
  sprite[nsprites].scale = 2;
  sprite[nsprites].rgb = green_puck;
  sprite[nsprites].pos = LED2POS(111);
  sprite[nsprites].vel = 55;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  sprite[nsprites].len = 8;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = yellow_puck;
  sprite[nsprites].pos = LED2POS(211);
  sprite[nsprites].vel = 30;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

#else

  sprite[nsprites].len = 13;
  sprite[nsprites].scale = 1;	// 10;
  sprite[nsprites].rgb = flame_sprite;
  sprite[nsprites].pos = LED2POS(33);
  sprite[nsprites].vel = 840;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;
#endif

  sprite[nsprites].len = 6;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = blue_puck;
  sprite[nsprites].pos = LED2POS(11);
  sprite[nsprites].vel = 120;
  sprite[nsprites].move = sprite_ping_pong;
  sprite[nsprites].flags = 0;
  nsprites++;

  sprite[nsprites].len = 6;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = green_puck;
  sprite[nsprites].pos = LED2POS(1);
  sprite[nsprites].vel = 144;
  sprite[nsprites].move = sprite_ping_pong;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  sprite[nsprites].len = 6;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = red_puck;
  sprite[nsprites].pos = LED2POS(22);
  sprite[nsprites].vel = 24;
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  sprite[nsprites].len = 1;
  sprite[nsprites].scale = 1;
  sprite[nsprites].rgb = flame_sprite+3;
  sprite[nsprites].pos = LED2POS(33);
  sprite[nsprites].vel = 18;	// speed 18, used for the red roundtrip timings.
  sprite[nsprites].move = sprite_wraparound;
  sprite[nsprites].flags = SPRITE_F_WRAP;
  nsprites++;

  for (;;)
    {
      uint8_t speed = 6;		// 7 = full speed, 0 = slowest
      if (!(BUTH_PIN & BUTH_BITS)) 	// slow simulator while BUT pressed
        speed = 0;

#if MICROSTEPS == 8
      // with a total of 8 render_frame microsteps the
      // animation looks smooth! 4 is not quite enough...
      simulate_world(speed);
      render_frame(rgb, 3, 0);
      simulate_world(speed);
      render_frame(rgb, 3, 1);
      simulate_world(speed);
      render_frame(rgb, 3, 2);
      simulate_world(speed);
      render_frame(rgb, 3, 3);

      simulate_world(speed);
      render_frame(rgb, 3, 4);
      simulate_world(speed);
      render_frame(rgb, 3, 5);
      simulate_world(speed);
      render_frame(rgb, 3, 6);
      simulate_world(speed);
      render_frame(rgb, 3, 7);
#else
# if MICROSTEPS == 4
      simulate_world(speed);
      render_frame(rgb, 2, 0);
      simulate_world(speed);
      render_frame(rgb, 2, 1);
      simulate_world(speed);
      render_frame(rgb, 2, 2);
      simulate_world(speed);
      render_frame(rgb, 2, 3);
# else
#  if MICROSTEPS == 2
      simulate_world(speed);
      render_frame(rgb, 1, 0);
      simulate_world(speed);
      render_frame(rgb, 1, 1);
#  else
      simulate_world(speed);
      render_frame(rgb, 0, 0);
#  endif
# endif
#endif

      // dual channel mode by calling send_leds() twice: 41sec red roundtrip
      // FIXME: use offset for b instead of rgb+0
      send_leds(rgb, WS2812_NLEDSa, rgb+0, WS2812_NLEDSb);
      if (!(BUTF_PIN & BUTF_BITS)) flash = 255;
      else flash = (uint16_t)(flash*15)>>4;
      if (flash < 1) flash = 1;
      memset(rgb, flash, 3*WS2812_NLEDS);	// background
      // if (!(BUTN_PIN & BUTN_BITS))  { p = rgb+0; SETpOR2; }
      // if (!(BUTF_PIN & BUTF_BITS))  { p = rgb+3*WS2812_NLEDS-3; SETpCY2; }
    }
}

