/*
 * $Id: pfill.c,v 1.1 2005-09-18 22:05:37 dhmunro Exp $
 * pl_fill for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"

/* ARGSUSED */
void
pl_fill(pl_win_t *w, int convexity)
{
  int n = _pl_w_pt_count;
  HDC dc = _pl_w_getdc(w, 12);
  if (dc) {
    /* SetPolyFillMode(dc, ALTERNATE); */
    Polygon(dc, _pl_w_pt_list, n);
  }
}
