/*
 * ws2812-tiny-vncview.c -- a simple vnc viewer for non-X11 displays.
 *
 * (C) 2014 Juergen Weigert <jw@owncloud.com>
 * Distribute under LGPL-2.0+ or ask.
 *
 */
#include "tiny_vncview.h"

#include <sys/ioctl.h>	// ioctl FIONREAD
#include <sys/stat.h>	// mkfifo()
#include <fcntl.h>	// open()
#include <netdb.h>

// #define USE_GAMMA_LUT	// does not work well with the arietta ledpanel. we have not enough bits.
#if 0	// arietta
# define PANEL_W		32
# define PANEL_H		32
# define PANEL_DEVICE_NAME "/sys/class/ledpanel/rgb_buffer"
#else
# define PANEL_W		30
# define PANEL_H		9
# define PANEL_DEVICE_NAME "/dev/ws2812"
#endif

void cursor_up(int n)
{
  if (n < 1) n = 1;
  fprintf(stdout, "\x1b[%dA", n);
  fflush(stdout);
}

void rgb_ttyramp(int r, int g, int b)
{
  // http://paulbourke.net/dataformats/asciiart/
  unsigned int v = (r+g+b)/3;
  //static char ascii_ramp[] = " .-:=+*#%@";
  static char ascii_ramp[] = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/|()1{}[]?-_+~<>i!lI;:,\"^'. ";
  unsigned int i = v * (sizeof(ascii_ramp)-2) / 255;
  i = sizeof(ascii_ramp)-2-i;
  fputc(ascii_ramp[i], stdout);
  fputc(ascii_ramp[i], stdout);
}

void rgb_tty8c(int r, int g, int b)
{
  int dim = 1;
  int ansi = 0;

  if (r < 100) r = 0;
  if (g < 100) g = 0;
  if (b < 100) b = 0;
  if ((r >= 200) || (g >= 200) || (b >= 200)) dim = 0;
  if (r && g && b) ansi = 37;	// white
  else if (r && g) ansi = 33;	// yellow
  else if (r && b) ansi = 35;	// magenta
  else if (g && b) ansi = 36;	// cyan
  else if (r)      ansi = 31;	// red
  else if (g)      ansi = 32;	// green
  else if (b)      ansi = 34;	// blue
  else             ansi = 30;	// black

  fprintf(stdout, "\x1b[%d;1;%dm%s\x1b[0m", ansi, ansi+10, dim?"  ":"@@");
}

void draw_ttyc8(int ww, int hh, unsigned char *img, int stride)
{
  int h, w;
  cursor_up(hh+1);
  for (h = 0; h < hh; h++)
    {
      for (w = 0; w < ww; w++)
        {
          unsigned int r = *img++;
          unsigned int g = *img++;
          unsigned int b = *img++;
	  rgb_tty8c(r,g,b);
        }
      img += stride - 3*ww;
      fprintf(stdout, "\r\n");
    }
  fflush(stdout);
}

void draw_ttyramp(int ww, int hh, unsigned char *img, int stride)
{
  int h, w;
  cursor_up(hh+1);
  for (h = 0; h < hh; h++)
    {
      for (w = 0; w < ww; w++)
        {
          unsigned int r = *img++;
          unsigned int g = *img++;
          unsigned int b = *img++;
	  rgb_ttyramp(r,g,b);
        }
      img += stride - 3*ww;
      fprintf(stdout, "\r\n");
    }
  fflush(stdout);
}


void read_rgb_file(int w, int h, char *fname, unsigned char *buf)
{
  int i;
  FILE *fp = fopen(fname, "r");
  for (i = 0; i < w*h*3; i++)
    *buf++ = fgetc(fp);
  fclose(fp);
}

struct draw_ledpanel_data
{
  int fd;
  unsigned char *lut;
};

#ifdef USE_GAMMA_LUT
// rg, gg, bg, are gamma values in the range of [-16..0..16], -16 is brightest, 0 is linear, 16 is darkest.
unsigned char *mkgamma_lut(int rg, int gg, int bg)
{
  unsigned char *lut = (unsigned char *)calloc(3, 256);

  unsigned int i;

#define L_POW4(i)    ((i)*(i)*(i)/255*(i)/(255*255))
#define L_POW3(i)    ((i)*(i)*(i)/        (255*255))
#define L_POW2(i)        ((i)*(i)/        (255))

// adjust the output values to not use the full [0..255] range.
#define L_RANGE(offset,i)	(32-(offset)+(i)*7/8)

  if (rg < 0)
    for (i = 0; i < 255; i++) lut[i+0*256] = L_RANGE(8, 255-((16+rg)*(255-i)-rg*L_POW2(255-i))/16);
  else
    for (i = 0; i < 255; i++) lut[i+0*256] = L_RANGE(8,     ((16-rg)*i      +rg*L_POW2(    i))/16);

  if (gg < 0)
    for (i = 0; i < 255; i++) lut[i+1*256] = L_RANGE(8, 255-((16+gg)*(255-i)-gg*L_POW2(255-i))/16);
  else
    for (i = 0; i < 255; i++) lut[i+1*256] = L_RANGE(8,     ((16-gg)*i      +gg*L_POW2(    i))/16);

  if (bg < 0)
    for (i = 0; i < 255; i++) lut[i+2*256] = L_RANGE(8, 255-((16+bg)*(255-i)-bg*L_POW2(255-i))/16);
  else
    for (i = 0; i < 255; i++) lut[i+2*256] = L_RANGE(8,     ((16-bg)*i      +bg*L_POW2(    i))/16);

  return lut;
}
#endif

int draw_ledpanel(VncView *view, unsigned char *rgb, int stride, void *data)
{
  // FIXME: need gamma curves here!
  struct draw_ledpanel_data *d = (struct draw_ledpanel_data *)data;
  static unsigned char led[PANEL_W*PANEL_H*3];
  unsigned char *p = led;
  int h = PANEL_H;

  rgb += view->x * 3;
  rgb += view->y * stride;

#ifdef USE_GAMMA_LUT
  // with only 7 values, all on the bright side, gamma correction is hard.
  if (!d->lut) d->lut = mkgamma_lut(8,8,8);
#endif
  while (h-- > 0)
    {
#ifdef USE_GAMMA_LUT
      int x;
      for (x = 0; x < 3*PANEL_W; x+=3)
        {
          p[x+0] = d->lut[rgb[x+0]+0*256];
          p[x+1] = d->lut[rgb[x+1]+1*256];
          p[x+2] = d->lut[rgb[x+2]+2*256];
        }
#else
      memcpy(p, rgb, 3*PANEL_W);
#endif
      rgb += stride;
      p += 3*PANEL_W;
    }
  // lseek(d->fd, 0, 0);
  write(d->fd, led, 3*PANEL_W*PANEL_H);
  return TRUE;
}


int main(int ac, char **av)
{
  int fifo_fd = -1;
  int r;

  if (!av[1])
    {
      fprintf(stderr,
"vnc_tiny_view connects your Arietta ledpanel to a remote X-Server.\n\
\n\
Usage:\n\
  # On the X-Server HOST run a VNC server e.g. like this:\n\
  x11vnc -clip 640x480x0x0 -cursor none -loop\n\
\n\
  # On Arietta:\n\
  VNC_TINY_CFG=/tmp/fifo %s HOST [5900] &\n\
\n\
  # To reposition the viewport:\n\
  echo 100 100 > /tmp/fifo\n", av[0]);
      exit(0);
    }

#if 1
  VncConnection *conn = connect_vnc_server(av[1], av[2]);	// hostname [port]

  if (getenv("VNC_TINY_CFG"))
    {
      char *fifo = getenv("VNC_TINY_CFG");
      mkfifo(fifo, 0777);
      fifo_fd = open(fifo, O_RDONLY|O_NONBLOCK);
    }
  conn->view.x = 0;
  conn->view.y = 0;
  conn->view.w = PANEL_W;
  conn->view.h = PANEL_H;

  struct draw_ledpanel_data draw_ledpanel_data;
  draw_ledpanel_data.lut = NULL;
  draw_ledpanel_data.fd = open(PANEL_DEVICE_NAME, O_WRONLY);
  if (getenv("VNC_TINY_STDOUT") || draw_ledpanel_data.fd < 0)
    {
      conn->expose_cb = draw_ascii_art;
    }
  else
    {
      conn->expose_cb = draw_ledpanel;
      conn->expose_cb_data = (void *)&draw_ledpanel_data;
    }

  if (!vnc_connection_initialize(conn)) exit(8);

  // non-incremental to begin with.
  vnc_connection_framebuffer_update_request(conn, 0, conn->view.x, conn->view.y, conn->view.w, conn->view.h);

  while ((r = vnc_connection_server_message(conn, fifo_fd)) >= 0)
    {
      if (r == fifo_fd)
	{
	  char buf[1024];
	  int x = conn->view.x;
	  int y = conn->view.y;

	  int n = read(fifo_fd, buf, sizeof(buf)-1);
	  if (n < 0) break;
	  if (n > 0)
	    {
	      buf[n] = '\0';
	      n = sscanf(buf, "%d %d\n", &x, &y);
	      if (n >= 1) vncview_moveto(conn, x, y);
	    }
	  continue;
        }
    }

#else

  char buf[PANEL_W*PANEL_H*3];
  read_rgb_file(PANEL_W, PANEL_H, av[1], buf);
  // draw_ttyc8(PANEL_W,PANEL_H,buf,PANEL_W);
  draw_ttyramp(PANEL_W,PANEL_H,buf,PANEL_W);
#endif
  return 0;
}

