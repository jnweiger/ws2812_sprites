/*
 * usart_ws2812.c -- driving led-strips via the usar on arietta. at91sam9
 * This requires dma or pdc enabled on the device and a 
 * new ioctl() in the kernel to set exotic baud rates.
 *
 * (C) 2014 juewei@fabfolk.com
 * 
 * With enabled pdc, this blocks for several seconds, and outputs nothing.
 */
#include <linux/serial.h>	// struct serial_struct
#include <asm-generic/ioctls.h>	// TCGETX
#include <sys/ioctl.h>		// ioctl()
#include <stdlib.h>		// atol()
#include <unistd.h>		// open()/close()
#include <fcntl.h>		// O_RDWR
#include <stdio.h>		// printf()
#include <errno.h>		// errno
#include <assert.h>		// assert()

struct ws2812
{
  int fd;
  char *buf;
  int buflen;
  int n_leds;
  struct serial_struct s;
};

struct ws2812 *ws2812_init(char *dev, int n_leds)
{
  struct ws2812 *w = calloc(1, sizeof(struct ws2812));
  int r;

  w->fd = open(dev, O_RDWR);

  // set 2.55Mhz bit clock, 0.417us
  // set 4.255Mhz bit clock, 0.235us

  w->s.baud_base = 2550000;
  w->s.custom_divisor = 0;
  w->s.xmit_fifo_size = 0;
  errno = 0;

  r = ioctl(w->fd, TCSETX, &w->s);

  w->s.baud_base = 0;
  w->s.custom_divisor = 5;	// should result in ca 2.3 Mhz
  w->s.xmit_fifo_size = 0;

  r = ioctl(w->fd, TCGETX, &w->s);
  if (w->s.xmit_fifo_size < 256)
    {
      fprintf(stderr, "TCGETX r=%d, errno=%d\n", r, errno);
      fprintf(stderr, "ERROR: xmit_fifo_size=%d, (need min 256)\n", w->s.xmit_fifo_size);
      exit(1);
    }
  fprintf(stderr, "%s: using baud=%d\n", dev, w->s.baud_base);
  fprintf(stderr, "%s: use_dma_tx=%d\n", dev, w->s.type & 1);
  fprintf(stderr, "%s: use_pdc_tx=%d\n", dev, w->s.flags & 1);

  w->n_leds = n_leds;
  w->buflen = n_leds * 3 * 4;	// encode 1 bit as 4 bits
  w->buf = (char *)calloc(1, w->buflen);
  if (w->buf == NULL)
    {
      fprintf(stderr, "ERROR: calloc %d bytes failed\n", w->buflen);
      exit(1);
    }
  return w;
}

void ws2812_fini(struct ws2812 *w)
{
  if (!w) return;
  if (!w->buf) free((char *)w->buf);
  w->buf = (char *)0xdeadbeef;
  w->buflen = 0;
  w->n_leds = 0;

  w->s.custom_divisor = 0;
  w->s.baud_base = 115200;	// back to some standard baud rate
  ioctl(w->fd, TCGETX, &w->s);
  close(w->fd);
  w->fd = -1;
  free((char *)w);
}

// off and len are in bytes, should both be multiples of 3.
void ws2812_send(struct ws2812 *w, int off, char *rgb, int len)
{
  int i;
  static char doublebit[] = 
    {
        // the uart sends the bits lsb first. and we invert all.
        (255-0x10),          // ^....1..._    0 0
        (255-0x70),          // ^....111._    0 1
        (255-0x13),          // ^11..1..._    1 0
        (255-0x73),          // ^11..111._    1 1
    };

  char *p = w->buf;
  unsigned char r,g,b;

  if (off < 0)
    {
      len -= off;
      rgb += off;
      off = 0;
    }

  for (i = 0; i < w->n_leds; i++)
    {
      if (off > 0 || len < 3) 
        {
	  if (off > 0) off -= 3;
          r = 0; b = 0; g = 0;
	}
      else
        {
	  r = *rgb++;
	  g = *rgb++;
	  b = *rgb++;
	  len -= 3;
	}

      *p++ = doublebit[(g>>6) & 3];
      *p++ = doublebit[(g>>4) & 3];
      *p++ = doublebit[(g>>2) & 3];
      *p++ = doublebit[(g>>0) & 3];

      *p++ = doublebit[(r>>6) & 3];
      *p++ = doublebit[(r>>4) & 3];
      *p++ = doublebit[(r>>2) & 3];
      *p++ = doublebit[(r>>0) & 3];

      *p++ = doublebit[(b>>6) & 3];
      *p++ = doublebit[(b>>4) & 3];
      *p++ = doublebit[(b>>2) & 3];
      *p++ = doublebit[(b>>0) & 3];
    }

  assert((p == w->buf + w->buflen));	// Oops, fix the calculation of buflen in ws2812_init().
  i = w->buflen;
  p = w->buf;
  while (i)
    {
      int r = write(w->fd, p, i);
      if (r < 0)
        {
	  fprintf(stderr, "ERROR: ws2812_send: write failed: r=%d, errno=%d, off=%d\n", r, errno, p - w->buf);
	  exit(1);
	}
      if (r <= 1)
        {
	  fprintf(stderr, "WARNING: ws2812_send: short write: r=%d\n", r);
	}
      p += r;
      i -= r;
    }
  // usleep(1);	// sleep 1 microsecond TBD by caller.
}

int main(int ac, char **av)
{
  struct ws2812 * w = ws2812_init("/dev/ttyS2", 7);
  int i;

  static char p1[] = 
    {
      255,0,0,		// # R
      0,255,0,		// # G
      0,0,255,		// # B
      0,255,255,	// # C
      255,0,255,	// # M
      255,255,0		// # Y
    };
  static char p2[] =
    {
      0,255,0,		// # G
      0,0,255,		// # B
      0,255,255,	// # C
      255,0,255,	// # M
      255,255,0,	// # Y
      255,0,0		// # R
    };

  for (i = 0; i < 20; i++)
    {
      ws2812_send(w, 0, p1, sizeof(p1));
fprintf(stderr, "i=%d\n", i);
      usleep(1+500);	// sleep 1 microsecond
      ws2812_send(w, 0, p2, sizeof(p2));
      usleep(1+500);	// sleep 1 microsecond
    }

  ws2812_fini(w); 
  return 0;
}
