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
  conn->view.w = 32;
  conn->view.h = 32;

  struct draw_ledpanel_data draw_ledpanel_data;
  draw_ledpanel_data.lut = NULL;
  draw_ledpanel_data.fd = open("/sys/class/ledpanel/rgb_buffer", O_WRONLY);
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

  char buf[32*32*3];
  read_rgb_file(32, 32, av[1], buf);
  // draw_ttyc8(32,32,buf,32);
  draw_ttyramp(32,32,buf,32);
#endif
  return TRUE;
}

