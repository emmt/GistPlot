/*
 * $Id: colors.c,v 1.1 2005-09-18 22:05:32 dhmunro Exp $
 * color handling for X11
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "playx.h"

GC
_pl_x_getgc(pl_scr_t *s, pl_win_t *w, int fillstyle)
{
  GC gc = s->gc;
  if (w && w != s->gc_w_clip) {
    _pl_x_clip(s->xdpy->dpy, gc,
           w->xyclip[0], w->xyclip[1], w->xyclip[2], w->xyclip[3]);
    s->gc_w_clip = w;
  }
  if (fillstyle != s->gc_fillstyle) {
    /* note: if gc_color == PL_GRAYB--PL_GRAYC, then gc_fillstyle
     *       will not reflect the actual fillstyle in gc
     * the fillstyle would be unnecessary but for rotated text,
     * which requires stippled fills, and therefore may not work
     * properly with PL_GRAYB--PL_GRAYC colors -- see textout.c */
    XSetFillStyle(s->xdpy->dpy, gc, fillstyle);
    s->gc_fillstyle = fillstyle;
  }
  return gc;
}

void
pl_color(pl_win_t *w, pl_col_t color)
{
  pl_scr_t *s = w->s;
  GC gc = s->gc;
  pl_col_t old_color = s->gc_color;

  if (color != old_color) {
    pl_col_t pixel;

    s->gc_color = -1; /* invalidate; also when w->pixels changes */
    pixel = _pl_x_getpixel(w, color);

    if (color==PL_XOR) XSetFunction(s->xdpy->dpy, gc, GXxor);
    else if (old_color==PL_XOR ||
             old_color==-1) XSetFunction(s->xdpy->dpy, gc, GXcopy);

    if ((color==PL_GRAYC || color==PL_GRAYB) && s->gui_flags) {
      XSetFillStyle(s->xdpy->dpy, gc, FillOpaqueStippled);
      XSetStipple(s->xdpy->dpy, gc, s->gray);
      XSetBackground(s->xdpy->dpy, gc, s->colors[3].pixel);
      /* note that s->gc_fillstyle cannot be set here */
    } else if ((old_color==PL_GRAYC || old_color==PL_GRAYB) && s->gui_flags) {
      XSetFillStyle(s->xdpy->dpy, gc, FillSolid);
      XSetBackground(s->xdpy->dpy, gc, s->colors[0].pixel);
    }

    XSetForeground(s->xdpy->dpy, gc, pixel);
    s->gc_color = color;
  }
}

pl_col_t
_pl_x_getpixel(pl_win_t *w, pl_col_t color)
{
  pl_scr_t *s = w->s;
  pl_col_t pixel;
  if (w->parent) w = w->parent;  /* offscreens use parent pixels */
  if (!PL_IS_RGB(color)) {       /* standard and indexed color models */
    pixel = w->pixels[color];

  } else {                      /* rgb color model */
    unsigned int r = PL_R(color);
    unsigned int g = PL_G(color);
    unsigned int b = PL_B(color);
    if (s->vclass==TrueColor || s->vclass==DirectColor) {
      pixel = (s->pixels[r]&s->rmask) |
        (s->pixels[g]&s->gmask) | (s->pixels[b]&s->bmask);
    } else if (s->vclass!=PseudoColor) {
      pixel = s->pixels[(r+g+b)/3];
    } else if (w->rgb_pixels || _pl_x_rgb_palette(w)) {
      /* use precomputed 5-9-5 color brick */
      r = (r+32)>>6;
      g = (g+16)>>5;
      b = (b+32)>>6;
      g += b+(b<<3);
      pixel = w->rgb_pixels[r+g+(g<<2)];  /* r + 5*g * 45*b */
    } else {
      pixel = s->colors[1].pixel;
    }
  }
  return pixel;
}
