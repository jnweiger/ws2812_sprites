/*
 * tiny_vncview.h -- a simple vnc viewer library.
 *
 * (C) 2014 Juergen Weigert <jw@owncloud.com>
 * Distribute under LGPL-2.0+ or ask.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>	// open()
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>	// ioctl FIONREAD

// #define HAVE_ZLIB 	// Unused: We don't support compressed streams.

#ifdef HAVE_ZLIB
# include <zlib.h>	// BuildRequires: zlib-devel
#endif

typedef struct VncConnectionPrivate VncConnectionPrivate;

typedef struct VncView
{
  int x, y, w, h;
  int moved;	// FIXME: every move here currently triggers a full update.
} VncView;


typedef struct VncConnection
{
  int fd;

  int msec_refresh;
  VncView view;

  void (*expose_cb)(VncView *view, unsigned char *rgb, int stride, void *expose_cb_data);
  void *expose_cb_data;

  VncConnectionPrivate *priv;
} VncConnection;

VncConnection *connect_vnc_server(char *hostname, char *str_port);
int vnc_connection_initialize(VncConnection *conn);
int vnc_connection_framebuffer_update_request(VncConnection *conn, 
    int incremental, u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height);
int vnc_connection_server_message(VncConnection *conn, int extra_fd);
void vnc_view_moveto(VncConnection *conn, int x, int y);

