// (C) 2014, jw@suse.de, GPL-2.0
//
// ws2812.c 
// a driver for ws2812, using bit banging at 400mhz.
// 
// http://www.aliexpress.com/item/WS2811IC-Digital-RGB-led-strip-5m-input-12V-300pcs-IC-300pcs-SMD5050-led-IP65-waterproof-free/672648273.html
// http://de.aliexpress.com/item/4m-WS2811-LED-digital-strip-60leds-m-with-60pcs-WS2811-built-in-tthe-5050-smd-rgb/633124138.html
//
// http://partfusion.com/wp-uploads/2013/01/WS2812preliminary.pdf
//  T0H 0 code pulse: 0.35us +/- 0.15us
//  T0L 0 code pause: 0.8us  +/- 0.15us
//  T1H 1 code pulse: 0.7us  +/- 0.15us
//  T1L 1 code pause: 0.6us  +/- 0.15us
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
// 2014-01-04, jw -- Porting the arduino code to carambola


#include <stdio.h>
#include "mmio.h"

#define NSPRITES	20
#define LED2POS(n)	(((int32_t)(n))<<16)
#define POS2LED(n)	(((int32_t)(n))>>16)	// signed rshift! negative pos are valid!

// #define BLAUE_NACHT	1		// comment out, to use the normel code.
#define MICROSTEPS	8		// 8, 4, 2, 1 suppported.

#define SPEED_SCALE	(1<<5)		// 7 max, 1 min, (define this to overridde BUTH)

struct world;
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
  void (*move)(struct world *, struct sprite *);	// advance method
  void *data;		// optional data for move()
};

struct world
{
# define WS2812_NLEDS	240	// FIXMES: add offset for b
  uint8_t speed;
  uint8_t reverse;
  uint8_t nsprites;
  uint8_t rgb[WS2812_NLEDS * 3];
  struct sprite sprite[NSPRITES];
};

void sprite_wraparound(struct world *w, struct sprite *s)
{
  // Tricky: can have a sprite half and half on the edges
  // FIXME: The wrap is not perfectly smooth. Suspect an off-by-one error somewhere.
  int16_t svel = (w->reverse) ? -s->vel : s->vel;
  s->pos += (int32_t)svel * w->speed;
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

void sprite_ping_pong(struct world *w, struct sprite *s)
{
  int16_t svel = (w->reverse) ? -s->vel : s->vel;
  s->pos += (int32_t)svel * w->speed;

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

static void create_sprites(struct world *w)
{
  struct sprite *sprite = w->sprite;
  uint8_t nsprites = 0;

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

  uint8_t i;
  uint8_t *p;
  // create a sprite
  uint8_t flame_sprite[3*13];
  uint8_t green_puck[3*6];
  uint8_t blue_puck[3*6];
  uint8_t blue_wave[3*32];
  uint8_t yellow_puck[3*8];
#ifdef BLAUE_NACHT
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

  p = blue_wave; 
  for (i = 1; i <= 8; i++) { SET_RGB(p, 0, 0, (uint16_t)255*i>>3); }
  for (i = 1; i <= 8; i++) { SET_RGB(p, (uint16_t)47*i>>3, (uint16_t)63*i>>3, 255); }
  for (i = 8; i >= 1; i--) { SET_RGB(p, (uint16_t)47*i>>3, (uint16_t)63*i>>3, 255); }
  for (i = 8; i >= 1; i--) { SET_RGB(p, 0, 0, (uint16_t)255*i>>3); }
#ifdef BLAUE_NACHT
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
  sprite[nsprites].scale = 2;	// 10;
  sprite[nsprites].rgb = flame_sprite;
  sprite[nsprites].pos = LED2POS(33);
  sprite[nsprites].vel = 840;
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
  w->nsprites = nsprites;
}

void simulate_world(struct world *w)
{
  uint8_t i;

  for (i = 0; i < w->nsprites; i++)
    {
      struct sprite *s = w->sprite+i;
      if (s->move)
        (s->move)(w, s);
    }
}

// Downshift reduces brightness. It should be 0, unless we are supersampling...
void render_frame(struct world *w, uint8_t downshift, int16_t spread)
{
  // put all sprites to their respective position, 
  // TODO: add motion blurr for vel, and interpolate between neighbouring pixels
  uint8_t i;
  int16_t spread_pos = (LED2POS(1)>>downshift)*spread;

  for (i = 0; i < w->nsprites; i++)
    {
      uint8_t l;
      struct sprite *s = w->sprite+i;
      uint8_t *pb = w->buffer+3*POS2LED(s->pos+spread_pos);
      uint8_t *be = w->buffer+3*WS2812_NLEDS;
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
		  if (pb < w->buffer) 		// safety first
		    pb++;
		  else if (pb < be) 		// safety last
		    *pb++ = v;	
		  else
		    pb++;
		  if (pb == be && (s->flags & SPRITE_F_WRAP)) 
		    pb = w->buffer;
		}
	      if (rep) ps -= 3;
	    }
	}
    }
}

static void simulate_render(struct world * w)
{
#if MICROSTEPS == 8
  // with a total of 8 render_frame microsteps the
  // animation looks smooth! 4 is not quite enough...
  simulate_world(w);
  render_frame(w, 3, 0);
  simulate_world(w);
  render_frame(w, 3, 1);
  simulate_world(w);
  render_frame(w, 3, 2);
  simulate_world(w);
  render_frame(w, 3, 3);

  simulate_world(w);
  render_frame(w, 3, 4);
  simulate_world(w);
  render_frame(w, 3, 5);
  simulate_world(w);
  render_frame(w, 3, 6);
  simulate_world(w);
  render_frame(w, 3, 7);
#else
# if MICROSTEPS == 4
  simulate_world(w);
  render_frame(w, 2, 0);
  simulate_world(w);
  render_frame(w, 2, 1);
  simulate_world(w);
  render_frame(w, 2, 2);
  simulate_world(w);
  render_frame(w, 2, 3);
# else
#  if MICROSTEPS == 2
  simulate_world(w);
  render_frame(w, 1, 0);
  simulate_world(w);
  render_frame(w, 1, 1);
#  else
  simulate_world(w);
  render_frame(w, 0, 0);
#  endif
# endif
#endif
}


static inline send_bit(uint8_t on)
{
  :-( 
  not implemented.
}

// uint8_t buffer[3*numleds];
void send_leds(uint8_t *buffer, uint16_t numleds)
{
  int16_t i = numleds*3;
  for (; i >=0; i--)
    {
      uint8_t val =  *buffer++;

      uint8_t bit;
      for (bit = 0; bit < 8; bit++)
        {
	  send_bit(val & 0b10000000);	// 25sec red roundtrip
	  val <<= 1;
	}
    }
  _delay_us(50);
}


struct mmio_options
{
  struct mmio io;
};

int main(int argc, char **argv)
{
  struct world world;
  struct mmio_options mo;

  unsigned long tbase = 15500000UL;
  if (argc > 1) tbase = strtoul(argv[1], NULL, 0);

#define GPIO_RANGE 0x30
#define GPIO_BASE 0x18040000
#define GPIO_OE   0x00000000
#define GPIO_IN   0x00000004
#define GPIO_OUT  0x00000008
#define GPIO_SET  0x0000000C
#define GPIO_CLR  0x00000010

// AR9331.pdf p65 
// GPIO0  green led
// GPIO11 user button
// GPIO12 reset button
// GPIO13 orange led eth1
// GPIO14 orange led eth0

  memset(&mo, 0, sizeof(mo));
  mo.io.range = 1;
  mmio_map(&mo.io, GPIO_BASE, GPIO_RANGE);

  world.speed = SPEED_SCALE;
  world.reverse = 0;
  world.nsprites = 0;

  create_sprites();
  for (;;)
    {
      if (mmio_readl(&mo.io, GPIO_IN) & (1<<11))
        world.reverse = 1;
      else
        world.reverse = 0;

      simulate_render(&world);

      send_leds(world.rgb, WS2812_NLEDS);
      memset(world.rgb, 1, 3*WS2812_NLEDS);	// background
    }
  
#if 0
  unsigned long i;
  int j;
  for (j = 0; j < 10; j++)
    {
      fprintf(stderr, "CLR %d\n", j);
      for (i = tbase; i > 0; i--)
	{
	  mmio_writel(&mo.io, GPIO_CLR, (1L<<14)); 
	  mmio_writel(&mo.io, GPIO_CLR, (1L<<13)); 
	}
      fprintf(stderr, "SET %d\n", j);
      for (i = tbase; i > 0; i--)
	{
	  mmio_writel(&mo.io, GPIO_SET, (1L<<14));
	  mmio_writel(&mo.io, GPIO_CLR, (1L<<13)); 
	}
    }
  fprintf(stderr, "Hello! Oh world, done.\n");
#endif

  mmio_unmap(&mo.io);
  return 0;
}
