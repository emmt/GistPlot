/*
 * $Id: errors.c,v 1.1 2005-09-18 22:05:32 dhmunro Exp $
 * X11 error handling
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "playx.h"
#include "playu.h"
#include "play/std.h"

#include <string.h>

static char x11_errmsg[90];
static int x11_nerrs = 0;

int
_pl_x_err_handler(Display *dpy, XErrorEvent *event)
{
  if (!pl_signalling) {
    strcpy(x11_errmsg, "Xlib: ");
    XGetErrorText(dpy, event->error_code, x11_errmsg+6, 83);
    x11_errmsg[89] = '\0';
    _pl_u_errmsg = x11_errmsg;
    pl_signalling = PL_SIG_SOFT;
    x11_nerrs = 1;
  } else {
    ++x11_nerrs;
  }
  /* Xlib apparently ignores return value
   * - must return here, or Xlib internal data structures trashed
   * - therefore actual error processing delayed (u_error must return) */
  return 1;
}

int
_pl_x_panic(Display *dpy)
{
  pl_x_display_t *xdpy = _pl_x_dpy(dpy);

  if (xdpy) {
    /* attempt to close display to free all associated Xlib structs
     * -- study of xfree86 source indicates that first call to
     *    XCloseDisplay might well end up calling _pl_x_panic
     *    recursively, but a second call might succeed */
    xdpy->panic++;
    while (xdpy->screens) pl_disconnect(xdpy->screens);
    if (xdpy->panic<3) XCloseDisplay(dpy);
    xdpy->dpy = 0;
    pl_free(xdpy);  /* x_disconnect will not have done this */
  }

  pl_abort();
  return 1;  /* Xlib will call exit! */
}
