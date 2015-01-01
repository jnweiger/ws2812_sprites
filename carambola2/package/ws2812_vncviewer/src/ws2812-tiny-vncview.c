/*
 * ws2812-tiny-vncview.c -- a simple vnc viewer for non-X11 displays.
 *
 * (C) 2014 Juergen Weigert <jw@owncloud.com>
 * Distribute under LGPL-2.0+ or ask.
 *
 */
#include <tiny_vncview.h>

#include <sys/ioctl.h>	// ioctl FIONREAD
#include <sys/stat.h>	// mkfifo()
#include <fcntl.h>	// open()
#include <netdb.h>

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

