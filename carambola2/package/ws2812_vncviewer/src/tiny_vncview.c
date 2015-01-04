/*
 * tiny_vncview.c -- a simple vnc viewer library.
 *
 * (C) 2014 Juergen Weigert <jw@owncloud.com>
 * Distribute under LGPL-2.0+ or ask.
 *
 */
/*
 * Usage:
 * x11vnc -clip 640x480+0+0 -cursor none -repeat -loop
 *
 * Code taken from GTK VNC Widget.
 * Below is the copyright of github/gtk-vnc/src/vncconnection.c
 */
/*
 * GTK VNC Widget
 *
 * Copyright (C) 2006  Anthony Liguori <anthony@codemonkey.ws>
 * Copyright (C) 2009-2010 Daniel P. Berrange <dan@berrange.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */


/*
 * FIXME:
 * - add gamma lookup tables to draw_ledpanel_data
 * - view.moved: triggers a full refresh: It should move the ledpanel contents 
 *   locally and only send refresh requests for missing parts.
 */
#include "tiny_vncview.h"

typedef struct VncPixelFormat
{
  int bits_per_pixel;
  int depth;
  int byte_order;
  int true_color_flag;
  int red_max;
  int green_max;
  int blue_max;
  int red_shift;
  int green_shift;
  int blue_shift;
} VncPixelFormat;

typedef struct VncConnectionPrivate
{
  int absPointer;
  int major;
  int minor;
  int auth_type;
  int sharedFlag;
  int width;
  int height;
  int has_error;
  VncPixelFormat fmt;
  char *name;
  unsigned char *rgb;

#ifdef HAVE_ZLIB
  z_stream *strm;
  z_stream streams[5];
#endif

  struct {
        int incremental;
        u_int16_t x;
        u_int16_t y;
        u_int16_t width;
        u_int16_t height;
  } lastUpdateRequest;

} VncConnectionPrivate;

// FROM /usr/include/glib-2.0/glib/gmacros.h
#define FALSE (0)
#define TRUE  (!FALSE)
// FROM /usr/include/glib-2.0/glib/gtypes.h
#define G_BIG_ENDIAN	4321
#define G_LITTLE_ENDIAN	1234

// FROM man getaddrinfo
VncConnection *connect_vnc_server(char *hostname, char *str_port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s;

  if (!str_port) str_port = "5900";

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;          /* Any protocol */

  s = getaddrinfo(hostname, str_port, &hints, &result);
  if (s != 0)
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
      exit(EXIT_FAILURE);
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1) continue;

      if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) break;                  /* Success */

      close(sfd);
    }

  if (rp == NULL)
    {               /* No address succeeded */
      fprintf(stderr, "Could not connect\n");
      exit(EXIT_FAILURE);
    }

  freeaddrinfo(result);           /* No longer needed */

  static VncConnection conn;
  static VncConnectionPrivate priv;

  conn.view.x = 0;
  conn.view.y = 0;
  conn.view.w = 32;
  conn.view.h = 32;
  conn.view.moved = 1;		// start with a full update request
  conn.expose_cb = NULL;
  conn.expose_cb_data = NULL;
  priv.rgb = NULL;
  priv.sharedFlag = TRUE;
  priv.has_error = FALSE;
  conn.priv = &priv;
  conn.fd = sfd;
  conn.msec_refresh = 200;
  return &conn;
}


int vnc_connection_flush(VncConnection *conn)
{
  // we don't buffer for now.
  return TRUE;
}

int vnc_connection_read(VncConnection *conn, char *buf, int len)
{
  int rr = 0;
  while (len > 0)
    {
      int r = read(conn->fd, buf, len);
      if (r <= 0) 
        {
          conn->priv->has_error = TRUE;
          return -1;
        }
      len -= r;
      buf += r;
      rr += r;
    }
  return rr;
}

u_int8_t vnc_connection_read_u8(VncConnection *conn)
{
  u_int8_t value = 0;
  int r = vnc_connection_read(conn, (char *)&value, sizeof(value));
  if (r != sizeof(value)) conn->priv->has_error = TRUE;
  return value;
}

u_int16_t vnc_connection_read_u16(VncConnection *conn)
{
  u_int16_t value = 0;
  int r = vnc_connection_read(conn, (char *)&value, sizeof(value));
  if (r != sizeof(value)) conn->priv->has_error = TRUE;
  return ntohs(value);
}

u_int32_t vnc_connection_read_u32(VncConnection *conn)
{
  u_int32_t value = 0;
  int r = vnc_connection_read(conn, (char *)&value, sizeof(value));
  if (r != sizeof(value)) conn->priv->has_error = TRUE;
  return ntohl(value);
}

int32_t vnc_connection_read_s32(VncConnection *conn)
{
  int32_t value = 0;
  int r = vnc_connection_read(conn, (char *)&value, sizeof(value));
  if (r != sizeof(value)) conn->priv->has_error = TRUE;
  return ntohl(value);
}


int vnc_connection_write(VncConnection *conn, char *buf, int len)
{
  int rr = 0;
  while (len > 0)
    {
      int r = write(conn->fd, buf, len);
      if (r <= 0) 
        {
          conn->priv->has_error = TRUE;
          return -1;
        }
      len -= r;
      buf += r;
      rr += r;
    }
  return rr;
}

int vnc_connection_write_u8(VncConnection *conn, u_int8_t value)
{
  return vnc_connection_write(conn, (char *)&value, sizeof(value));
}

int vnc_connection_write_u16(VncConnection *conn, u_int32_t value)
{
  u_int16_t v16 = htons(value);
  return vnc_connection_write(conn, (char *)&v16, sizeof(v16));
}

int vnc_connection_write_u32(VncConnection *conn, u_int32_t value)
{
  value = htonl(value);
  return vnc_connection_write(conn, (char *)&value, sizeof(value));
}

int vnc_connection_write_s32(VncConnection *conn, int32_t value)
{
  value = htonl(value);
  return vnc_connection_write(conn, (char *)&value, sizeof(value));
}


// FROM gtk-vnc/src/vncconnection.h

#define VNC_CONNECTION_AUTH_NONE 1

typedef enum {
    VNC_CONNECTION_ENCODING_RAW = 0,
    VNC_CONNECTION_ENCODING_COPY_RECT = 1,
    VNC_CONNECTION_ENCODING_RRE = 2,
    VNC_CONNECTION_ENCODING_CORRE = 4,
    VNC_CONNECTION_ENCODING_HEXTILE = 5,
    VNC_CONNECTION_ENCODING_TIGHT = 7,
    VNC_CONNECTION_ENCODING_ZRLE = 16,
} VncConnectionEncoding;

// FROM gtk-vnc/src/vncconnection.c

typedef enum {
    VNC_CONNECTION_SERVER_MESSAGE_FRAMEBUFFER_UPDATE = 0,
    VNC_CONNECTION_SERVER_MESSAGE_SET_COLOR_MAP_ENTRIES = 1,
    VNC_CONNECTION_SERVER_MESSAGE_BELL = 2,
    VNC_CONNECTION_SERVER_MESSAGE_SERVER_CUT_TEXT = 3,
    VNC_CONNECTION_SERVER_MESSAGE_QEMU = 255,
} VncConnectionServerMessage;

typedef enum {
    VNC_CONNECTION_CLIENT_MESSAGE_SET_PIXEL_FORMAT = 0,
    VNC_CONNECTION_CLIENT_MESSAGE_SET_ENCODINGS = 2,
    VNC_CONNECTION_CLIENT_MESSAGE_FRAMEBUFFER_UPDATE_REQUEST = 3,
    VNC_CONNECTION_CLIENT_MESSAGE_KEY = 4,
    VNC_CONNECTION_CLIENT_MESSAGE_POINTER = 5,
    VNC_CONNECTION_CLIENT_MESSAGE_CUT_TEXT = 6,
    VNC_CONNECTION_CLIENT_MESSAGE_QEMU = 255,
} VncConnectionClientMessage;

static int vnc_connection_before_version (VncConnection *conn, int major, int minor)
{
  VncConnectionPrivate *priv = conn->priv;

  return (priv->major < major) || (priv->major == major && priv->minor < minor);
}

static int vnc_connection_after_version (VncConnection *conn, int major, int minor)
{
  return !vnc_connection_before_version (conn, major, minor+1);
}

int vnc_connection_has_error(VncConnection *conn)
{
    VncConnectionPrivate *priv = conn->priv;

    return priv->has_error;
}

static void vnc_connection_read_pixel_format(VncConnection *conn, VncPixelFormat *fmt)
{
    char pad[3];

    fmt->bits_per_pixel  = vnc_connection_read_u8(conn);
    fmt->depth           = vnc_connection_read_u8(conn);
    fmt->byte_order      = vnc_connection_read_u8(conn) ? G_BIG_ENDIAN : G_LITTLE_ENDIAN;
    fmt->true_color_flag = vnc_connection_read_u8(conn);

    fmt->red_max         = vnc_connection_read_u16(conn);
    fmt->green_max       = vnc_connection_read_u16(conn);
    fmt->blue_max        = vnc_connection_read_u16(conn);

    fmt->red_shift       = vnc_connection_read_u8(conn);
    fmt->green_shift     = vnc_connection_read_u8(conn);
    fmt->blue_shift      = vnc_connection_read_u8(conn);

    vnc_connection_read(conn, pad, 3);

    fprintf(stderr, "Pixel format BPP: %d,  Depth: %d, Byte order: %d, True color: %d\n"
              "             Mask  red: %3d, green: %3d, blue: %3d\n"
              "             Shift red: %3d, green: %3d, blue: %3d\n",
              fmt->bits_per_pixel, fmt->depth, fmt->byte_order, fmt->true_color_flag,
              fmt->red_max, fmt->green_max, fmt->blue_max,
              fmt->red_shift, fmt->green_shift, fmt->blue_shift);
}


int vnc_connection_check_auth_result(VncConnection *conn)
{
  VncConnectionPrivate *priv = conn->priv;
  u_int32_t result = vnc_connection_read_u32(conn);
  if (!result) return TRUE;

  if (priv->minor >= 8)
    {
      char reason[1024];
      int len = vnc_connection_read_u32(conn);

      if (len < 1 || len >= sizeof(reason)) { fprintf(stderr, "auth error: unkonw\n"); exit(4); }
      vnc_connection_read(conn, reason, len);
      reason[len] = '\0';
      fprintf(stderr, "auth error: Server says: %s\n", reason);
      return FALSE;
    }
  return FALSE;
}

int vnc_connection_perform_auth(VncConnection *conn)
{
  VncConnectionPrivate *priv = conn->priv;
  unsigned int nauth, i;
  unsigned int auth[10];

  if (priv->minor <= 6) {
    nauth = 1;
    auth[0] = vnc_connection_read_u32(conn);
  } else {
    int auth_type_none_seen = 0;
    nauth = vnc_connection_read_u8(conn);
    if (nauth == 0) { fprintf(stderr, "Connection refused or already connected?\n"); }
    if (nauth < 1 || nauth > 10) { fprintf(stderr, "nauth=%d out of range [1..10]\n", nauth); exit(2); }
    for (i = 0 ; i < nauth ; i++)
      {
        auth[i] = vnc_connection_read_u8(conn);
        if (auth[i] == VNC_CONNECTION_AUTH_NONE) auth_type_none_seen = 1;
      }
    if (!auth_type_none_seen) { fprintf(stderr, "Warn: only auth_type=1 (none) implemented. not offered by server.\n"); }
    priv->auth_type = VNC_CONNECTION_AUTH_NONE;
  }

  if (priv->minor > 6) {
    vnc_connection_write_u8(conn, priv->auth_type);
    vnc_connection_flush(conn);
  }

  if (priv->minor == 8)
    return vnc_connection_check_auth_result(conn);
  return TRUE;
}

static int vnc_connection_set_encodings(VncConnection *conn, int n_encoding, u_int32_t *encoding)
{
    int i;
    char pad[1] = {0};
    vnc_connection_write_u8(conn, VNC_CONNECTION_CLIENT_MESSAGE_SET_ENCODINGS);
    vnc_connection_write(conn, pad, 1);
    vnc_connection_write_u16(conn, n_encoding);
    for (i = 0; i < n_encoding; i++) {
        vnc_connection_write_s32(conn, encoding[i]);
    }
    vnc_connection_flush(conn);
    return !vnc_connection_has_error(conn);
}


int vnc_connection_initialize(VncConnection *conn)
{
  VncConnectionPrivate *priv = conn->priv;
  int ret;
  char version[13];

  vnc_connection_read(conn, version, 12);

  version[12] = 0;
  ret = sscanf(version, "RFB %03d.%03d\n", &priv->major, &priv->minor);
  if (ret != 2) {
        fprintf(stderr, "Error while parsing server version\n");
	exit(2);
  }

  fprintf(stderr, "Server version: %d.%d\n", priv->major, priv->minor);

  if (vnc_connection_before_version(conn, 3, 3)) {
        fprintf(stderr, "Server version is not supported (%d.%d)\n", priv->major, priv->minor);
        exit(3);
    } else if (vnc_connection_before_version(conn, 3, 7)) {
        priv->minor = 3;
    } else if (vnc_connection_after_version(conn, 3, 8)) {
        priv->major = 3;
        priv->minor = 8;
    }
  snprintf(version, 13, "RFB %03d.%03d\n", priv->major, priv->minor);
  vnc_connection_write(conn, version, 12);
  vnc_connection_flush(conn);

  fprintf(stderr, "Using version: %d.%d\n", priv->major, priv->minor);

  if (!vnc_connection_perform_auth(conn)) {
        fprintf(stderr,"Auth failed\n"); exit(4);
  }
  printf("authentication successful\n");
  vnc_connection_write_u8(conn, priv->sharedFlag);	// "\1"
  vnc_connection_flush(conn);

  priv->width = vnc_connection_read_u16(conn);	// "\0@"
  priv->height = vnc_connection_read_u16(conn);	// "\0@"

  if (vnc_connection_has_error(conn))
    return FALSE;
  fprintf(stderr, "Initial desktop size %dx%d\n", priv->width, priv->height);
  priv->rgb = (unsigned char *)calloc(3*priv->width, priv->height);

  vnc_connection_read_pixel_format(conn, &priv->fmt);

  int n_name = vnc_connection_read_u32(conn);
  if (n_name > 4096)
    return FALSE;
  priv->name = (char *)calloc(sizeof(char), n_name + 1);

  vnc_connection_read(conn, priv->name, n_name);
  priv->name[n_name] = 0;
  fprintf(stderr, "Display name '%s'\n", priv->name);

  if (vnc_connection_has_error(conn))
    return FALSE;

#ifdef HAVE_ZLIB
  memset(&priv->streams, 0, sizeof(priv->streams));	// typo in gtk-vnc/src/vncconnection?
  /* FIXME what level? */
  for (i = 0; i < 5; i++)
    inflateInit(&priv->streams[i]);
  priv->strm = NULL;
#endif

  // vncdisplay.c:on_initialized()
  u_int32_t encodings[] = { VNC_CONNECTION_ENCODING_RAW };	// , VNC_CONNECTION_ENCODING_COPY_RECT ;

  vnc_connection_set_encodings(conn, 1, encodings);

  return TRUE;
}

static int vnc_connection_validate_boundary(VncConnection *conn,
                                                 u_int16_t x, u_int16_t y,
                                                 u_int16_t width, u_int16_t height)
{
    VncConnectionPrivate *priv = conn->priv;

    if ((x + width) > priv->width || (y + height) > priv->height) {
        fprintf(stderr, "Framebuffer update %dx%d at %d,%d outside boundary %dx%d\n",
                  width, height, x, y, priv->width, priv->height);
        priv->has_error = TRUE;
    }

    return !vnc_connection_has_error(conn);
}

static void vnc_framebuffer_copyrect(VncConnectionPrivate *priv, int sx, int sy, int x, int y, int w, int h)
{
  // NOT impl.
  fprintf(stderr, "r");
}

static void vnc_framebuffer_blt(VncConnectionPrivate *priv, u_int8_t *dst, int d, int x, int y, int w, int h)
{
  // may see multiple calls per update
  unsigned char *rgb = priv->rgb + x*3 + y*priv->width*3;
  if (priv->fmt.bits_per_pixel == 32)
    {
      u_int32_t *src = (u_int32_t *)dst;
      while (h-- > 0)
        {
          for (x = 0; x < w; x++)
	    {
	      u_int32_t v = *src++;
	      *rgb++ = (v>>priv->fmt.red_shift) & 0xff;
	      *rgb++ = (v>>priv->fmt.green_shift) & 0xff;
	      *rgb++ = (v>>priv->fmt.blue_shift) & 0xff;
	    }
          rgb += priv->width*3 - w*3;
        }
    }
  else
    {
      fprintf(stderr, "vnc_framebuffer_blt fmt.bits_per_pixel=%d not implemented\n", priv->fmt.bits_per_pixel);
      exit(11);
    }
  // fprintf(stderr, "b");
}

static void vnc_connection_update(VncConnection *conn, int x, int y, int w, int h)
{
  // always called one per updated
  // fprintf(stderr, "vnc_connection_update, x,y=%d,%d w,h=%d,%d\n", x,y,w,h);
  conn->expose_cb(&(conn->view), conn->priv->rgb, 3*conn->priv->width, conn->expose_cb_data);
}

static void vnc_connection_raw_update(VncConnection *conn,
                                      u_int16_t x, u_int16_t y,
                                      u_int16_t width, u_int16_t height)
{
    VncConnectionPrivate *priv = conn->priv;

    /* optimize for perfect match between server/client
       FWIW, in the local case, we ought to be doing a write
       directly from the source framebuffer and a read directly
       into the client framebuffer
    */
    u_int8_t *dst;
    int i;

    dst = (u_int8_t *)malloc(width * (priv->fmt.bits_per_pixel / 8));
    for (i = 0; i < height; i++)
      {
            vnc_connection_read(conn, (char *)dst, width * (priv->fmt.bits_per_pixel / 8));
            vnc_framebuffer_blt(priv, dst, 0, x, y + i, width, 1);
      }
    free(dst);
}


static void vnc_connection_copyrect_update(VncConnection *conn,
                                           u_int16_t dst_x, u_int16_t dst_y,
                                           u_int16_t width, u_int16_t height)
{
    VncConnectionPrivate *priv = conn->priv;
    int src_x, src_y;

    src_x = vnc_connection_read_u16(conn);
    src_y = vnc_connection_read_u16(conn);

    vnc_framebuffer_copyrect(priv,
                             src_x, src_y,
                             dst_x, dst_y,
                             width, height);
}



static int vnc_connection_framebuffer_update(VncConnection *conn, u_int16_t etype,
                                                  u_int16_t x, u_int16_t y,
                                                  u_int16_t width, u_int16_t height)
{
    VncConnectionPrivate *priv = conn->priv;

    // fprintf(stderr, "FramebufferUpdate type=%d area (%dx%d) at location %d,%d\n",
    //           etype, width, height, x, y);

    if (vnc_connection_has_error(conn))
        return !vnc_connection_has_error(conn);

    switch (etype) {
    case VNC_CONNECTION_ENCODING_RAW:
        if (!vnc_connection_validate_boundary(conn, x, y, width, height))
            break;
        vnc_connection_raw_update(conn, x, y, width, height);
        vnc_connection_update(conn, x, y, width, height);
        break;
    case VNC_CONNECTION_ENCODING_COPY_RECT:
        if (!vnc_connection_validate_boundary(conn, x, y, width, height))
            break;
        vnc_connection_copyrect_update(conn, x, y, width, height);
        vnc_connection_update(conn, x, y, width, height);
        break;
#if 0
    case VNC_CONNECTION_ENCODING_RRE:
        if (!vnc_connection_validate_boundary(conn, x, y, width, height))
            break;
        vnc_connection_rre_update(conn, x, y, width, height);
        vnc_connection_update(conn, x, y, width, height);
        break;
    case VNC_CONNECTION_ENCODING_HEXTILE:
        if (!vnc_connection_validate_boundary(conn, x, y, width, height))
            break;
        vnc_connection_hextile_update(conn, x, y, width, height);
        vnc_connection_update(conn, x, y, width, height);
        break;
    case VNC_CONNECTION_ENCODING_ZRLE:
        if (!vnc_connection_validate_boundary(conn, x, y, width, height))
            break;
        vnc_connection_zrle_update(conn, x, y, width, height);
        vnc_connection_update(conn, x, y, width, height);
        break;
    case VNC_CONNECTION_ENCODING_TIGHT:
        if (!vnc_connection_validate_boundary(conn, x, y, width, height))
            break;
        vnc_connection_tight_update(conn, x, y, width, height);
        vnc_connection_update(conn, x, y, width, height);
        break;
#endif
    default:
        fprintf(stderr, "Unknown VNC_CONNECTION_ENCODING_ type: %d\n", etype);
        priv->has_error = TRUE;
        break;
    }

    return !vnc_connection_has_error(conn);
}

int vnc_connection_framebuffer_update_request(VncConnection *conn,
                                                   int incremental,
                                                   u_int16_t x, u_int16_t y,
                                                   u_int16_t width, u_int16_t height)
{
    VncConnectionPrivate *priv = conn->priv;

    // fprintf(stderr, "Requesting framebuffer update at %d,%d size %dx%d, incremental %d\n",
    //            x, y, width, height, (int)incremental);

    priv->lastUpdateRequest.incremental = incremental;
    priv->lastUpdateRequest.x = x;
    priv->lastUpdateRequest.y = y;
    priv->lastUpdateRequest.width = width;
    priv->lastUpdateRequest.height = height;

    vnc_connection_write_u8(conn, VNC_CONNECTION_CLIENT_MESSAGE_FRAMEBUFFER_UPDATE_REQUEST);
    vnc_connection_write_u8(conn, incremental ? 1 : 0);
    vnc_connection_write_u16(conn, x);
    vnc_connection_write_u16(conn, y);
    vnc_connection_write_u16(conn, width);
    vnc_connection_write_u16(conn, height);
    vnc_connection_flush(conn);

    return !vnc_connection_has_error(conn);
}

int vnc_connection_server_message(VncConnection *conn, int extra_fg)
{
  int n;
  fd_set rfds;
  struct timeval tval;


  VncConnectionPrivate *priv = conn->priv;
  if (vnc_connection_has_error(conn))
    return FALSE;

  FD_ZERO(&rfds);
  FD_SET(conn->fd, &rfds);
  n = conn->fd;
  if (conn->fifo >= 0)
    {
      int nbytes = 0;
      ioctl(conn->fifo, FIONREAD, &nbytes);
      if (nbytes > 0)
        {
          if (conn->fifo > n) n = conn->fifo;
          FD_SET(conn->fifo, &rfds);
        }
    }
  tval.tv_sec = conn->msec_refresh / 1000;
  tval.tv_usec = 1000 * (conn->msec_refresh - 1000 * tval.tv_sec);

  n = select(n+1, &rfds, NULL, NULL, &tval);
  if (n == 0)
    {
      // timeout
      // request incremental updates (or full updates if view.moved).
      vnc_connection_framebuffer_update_request(conn, conn->view.moved ? 0 : 1, conn->view.x, conn->view.y, conn->view.w, conn->view.h);
      conn->view.moved = 0;
      return !vnc_connection_has_error(conn);
    }

  if (conn->fifo >= 0 && FD_ISSET(conn->fifo, &rfds))
    {
      char buf[1024];
      int x = conn->view.x;
      int y = conn->view.y;

      n = read(conn->fifo, buf, sizeof(buf)-1);
      if (n < 0) return FALSE;
      if (n > 0)
        {
          buf[n] = '\0';
          n = sscanf(buf, "%d %d\n", &x, &y);
	  if (n >= 1)
            {
	      // fprintf(stderr, "View (%d,%d) moved from (%d,%d) to (%d,%d) \n", conn->view.w,conn->view.h,
	      //	conn->view.x,conn->view.y, x,y);
	      if (x < 0) x = 0;
              if (y < 0) y = 0;
	      if (x+conn->view.w > conn->priv->width)  x = conn->priv->width  - conn->view.w;
	      if (y+conn->view.h > conn->priv->height) y = conn->priv->height - conn->view.h;
	      if (x != conn->view.x || y != conn->view.y) conn->view.moved = 1;
              conn->view.x = x; 
              conn->view.y = y; 
            }
        }
      return TRUE;
    }

  int msg = vnc_connection_read_u8(conn);
  switch (msg) {
    case VNC_CONNECTION_SERVER_MESSAGE_FRAMEBUFFER_UPDATE: {
        char pad[1];
        u_int16_t n_rects;
        int i;
        // fprintf(stderr, "VNC_CONNECTION_SERVER_MESSAGE_FRAMEBUFFER_UPDATE\n");

        vnc_connection_read(conn, pad, 1);
        n_rects = vnc_connection_read_u16(conn);
        for (i = 0; i < n_rects; i++) {
            u_int16_t x, y, w, h;
            int32_t etype;

            x = vnc_connection_read_u16(conn);
            y = vnc_connection_read_u16(conn);
            w = vnc_connection_read_u16(conn);
            h = vnc_connection_read_u16(conn);
            etype = vnc_connection_read_s32(conn);

            if (!vnc_connection_framebuffer_update(conn, etype, x, y, w, h))
                break;
        }
    }   break;

    case VNC_CONNECTION_SERVER_MESSAGE_SERVER_CUT_TEXT: {
        char pad[3];
        u_int32_t n_text;
        char *data;

        vnc_connection_read(conn, pad, 3);
        n_text = vnc_connection_read_u32(conn);
	if (n_text > (32 << 20)) { fprintf(stderr, "Closing: cutText > allowed\n"); exit(9); }

        data = (char *)calloc(sizeof(char), n_text + 1);
        vnc_connection_read(conn, data, n_text);
        data[n_text] = 0;
        // fprintf(stderr, "VNC_CONNECTION_SERVER_MESSAGE_SERVER_CUT_TEXT: %d bytes '%s'\r", n_text, data);
	free(data);
    }	break;

    default:
	// most likely a protocol error...
        fprintf(stderr, "Received an unknown message: %u\n", msg);
        priv->has_error = TRUE;
        break;
  } // switch(msg)

  return !vnc_connection_has_error(conn);
}

int draw_ascii_art(VncView *view, unsigned char *rgb, int stride, void *data)
{
  rgb += view->x * 3;
  rgb += view->y * stride;
  draw_ttyc8(32,32,rgb,stride);
  return TRUE;
}

void vncview_moveto(VncConnection *conn, int x, int y)
{
  // fprintf(stderr, "View (%d,%d) moved from (%d,%d) to (%d,%d) \n", conn->view.w,conn->view.h,
  //	conn->view.x,conn->view.y, x,y);
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x+conn->view.w > conn->priv->width)  x = conn->priv->width  - conn->view.w;
  if (y+conn->view.h > conn->priv->height) y = conn->priv->height - conn->view.h;
  if (x != conn->view.x || y != conn->view.y) conn->view.moved = 1;
  conn->view.x = x; 
  conn->view.y = y; 
}
