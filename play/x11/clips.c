/*
 * $Id: clips.c,v 1.1 2005-09-18 22:05:34 dhmunro Exp $
 * pl_clip for X11
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "playx.h"

void
pl_clip(pl_win_t *w, int x0, int y0, int x1, int y1)
{
  pl_scr_t *s = w->s;
  Display *dpy = s->xdpy->dpy;
  GC gc = _pl_x_getgc(s, (pl_win_t *)0, FillSolid);
  w->xyclip[0] = x0;
  w->xyclip[1] = y0;
  w->xyclip[2] = x1;
  w->xyclip[3] = y1;
  _pl_x_clip(dpy, gc, x0, y0, x1, y1);
  s->gc_w_clip = w;
}

void
_pl_x_clip(Display *dpy, GC gc, int x0, int y0, int x1, int y1)
{
  XRectangle xr;
  if (x1>x0 && y1>y0) {
    xr.width = x1 - x0;
    xr.height = y1 - y0;
    xr.x = x0;
    xr.y = y0;
    XSetClipRectangles(dpy, gc, 0,0, &xr, 1, YXBanded);
  } else {
    XSetClipMask(dpy, gc, None);
  }
  if (pl_signalling != PL_SIG_NONE) pl_abort();
}
