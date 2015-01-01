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

#define HAVE_ZLIB 0	// 0: unused. We don't support compressed streams.

#ifdef HAVE_ZLIB
# include <zlib.h>	// BuildRequires: zlib-devel
#endif


typedef struct VncConnection
{
  int fd;

  int msec_refresh;
  VncView view;

  int (*expose_cb)(VncView *view, unsigned char *rgb, int stride, void *expose_cb_data);
  void *expose_cb_data;

  VncConnectionPrivate *priv;
} VncConnection;

int vnc_connection_framebuffer_update_request(VncConnection *conn, 
    int incremental, u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height);

int vnc_connection_set_encodings(VncConnection *conn, int n_encoding, u_int32_t *encoding);

int vnc_connection_server_message(VncConnection *conn);
void vncview_moveto(int x, int y);
