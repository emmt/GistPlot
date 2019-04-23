/*
 * $Id: clips.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * pl_clip for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"

void
pl_clip(pl_win_t *w, int x0, int y0, int x1, int y1)
{
  HDC dc = _pl_w_getdc(w, 0);
  if (dc) {
    HRGN rgn = (x1<=x0 || y1<=y0)? 0 : CreateRectRgn(x0, y0, x1, y1);
    SelectClipRgn(dc, rgn);
    if (rgn) DeleteObject(rgn);
  }
}
