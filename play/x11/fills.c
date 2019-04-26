/*
 * $Id: fills.c,v 1.1 2005-09-18 22:05:32 dhmunro Exp $
 * pl_fill for X11
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "playx.h"

/* convexity 0 may self-intersect, 1 may not, 2 must be convex */
static int x_shape[3] = { Complex, Nonconvex, Convex };

void
pl_fill(pl_win_t *w, int convexity)
{
  pl_scr_t *s = w->s;
  Display *dpy = s->xdpy->dpy;
  GC gc = _pl_x_getgc(s, w, FillSolid);
  int nmx = (XMaxRequestSize(dpy)-3)/2;
  int n = _pl_x_pt_count;
  _pl_x_pt_count = 0;
  /* note: this chunking does not produce a correct plot, but
   *       it does prevent Xlib from crashing an R4 server
   * you just can't fill a polygon with too many sides */
  while (n>2) {
    if (n<nmx) nmx = n;
    XFillPolygon(dpy, w->d, gc, _pl_x_pt_list, nmx,
                 x_shape[convexity], CoordModeOrigin);
    n -= nmx;
  }
  if (pl_signalling != PL_SIG_NONE) pl_abort();
}
