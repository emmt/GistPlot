/*
 * $Id: gist/xbasic.h,v 1.3 2007-12-28 20:20:18 thiebaut Exp $
 * Declare the basic play engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_XBASIC_H
#define _GIST_XBASIC_H

#include <gist2.h>
#include <gist/engine.h>
#include <play2.h>

typedef struct _gp_x_engine gp_x_engine_t;
struct _gp_x_engine {
  gp_engine_t e;

  /* --------------- Specific to gp_x_engine_t ------------------- */

  pl_scr_t *s;
  pl_win_t *win;
  int width, height;  /* of (virtual page) graphics window */
  int wtop, htop;     /* of actual top-level window */
  int topMargin;   /* height of top menu bar, if any */
  int leftMargin;  /* width of left menu bar, if any */
  int x, y;        /* position of graphics relative to win */
  int dpi;         /* resolution of X window (dots per inch, 75 or 100) */
  int mapped, clipping;

  /* if w!=win, this is animation mode */
  pl_win_t *w;
  int a_width, a_height;        /* of animation Pixmap */
  int a_x, a_y;                 /* where it goes on graphics window */
  gp_transform_t swapped;          /* transform for graphics window while
                                 * in animation mode */

  /* if non-zero, these handlers can deal with input events */
  void (*HandleExpose)(gp_engine_t *engine, gd_drawing_t *drawing, int *xy);
  void (*HandleClick)(gp_engine_t *e,int b,int md,int x,int y, unsigned long ms);
  void (*HandleMotion)(gp_engine_t *e,int md,int x,int y);
  void (*HandleKey)(gp_engine_t *e,int k,int md);
};

PL_EXTERN pl_scr_t *gx_connect(char *displayName);
PL_EXTERN void gx_disconnect(pl_scr_t *s);

PL_EXTERN int gx_75dpi_width, gx_100dpi_width;    /* defaults are 450 and 600 pixels */
#define GX_DEFAULT_TOP_WIDTH(dpi) \
  (gx_75dpi_width<gx_100dpi_width?((dpi)*gx_100dpi_width)/100:gx_100dpi_width)
PL_EXTERN int gx75height, gx100height;  /* defaults are 450 and 600 pixels */
#define GX_DEFAULT_TOP_HEIGHT(dpi) \
  (gx_75dpi_width<gx_100dpi_width?((dpi)*gx100height)/100:gx100height)
#define GX_PIXELS_PER_NDC(dpi) ((dpi)/GP_ONE_INCH)

/* hack for pl_subwindow communication */
PL_EXTERN unsigned long gx_parent;
PL_EXTERN int gx_xloc, gx_yloc;

/* gp_engine_t which currently has mouse focus. */
PL_EXTERN gp_engine_t *gx_current_engine;

/* gx_new_engine creates an gp_x_engine_t and adds it to the list of GIST engines.
   The top window will generally be smaller than the graphics
   window created by gx_new_engine; specific engines are responsible for
   scrolling of the graphics window relative to the top window, although
   the initial location is passed in via (x, y).  The size argument is
   sizeof(gp_x_engine_t), or sizeof some derived engine class.  */
PL_EXTERN gp_x_engine_t *gx_new_engine(pl_scr_t *s, char *name, gp_transform_t *toPixels,
                           int x, int y,
                           int topMargin, int leftMargin, long size);

/* gx_input sets optional event handlers, and calls XSelectInput with
   the given eventMask.  HandleExpose, if non-zero, will be called
   instead of redrawing the gd_drawing_t associated with the gp_engine_t, which
   is the default action.  HandleResize, if non-zero, will be called
   instead of the default resize action (which is to recenter the
   graphics window).  HandleOther, if non-zero, will be called for
   keyboard, button, or other events not recogized by the default
   handler.  */
PL_EXTERN int gx_input(gp_engine_t *engine,
                     void (*HandleExpose)(gp_engine_t *, gd_drawing_t *, int *),
                     void (*HandleClick)(gp_engine_t *,
                                         int, int, int, int, unsigned long),
                     void (*HandleMotion)(gp_engine_t *, int, int, int),
                     void (*HandleKey)(gp_engine_t *, int, int));

PL_EXTERN gp_x_engine_t *gp_x_engine(gp_engine_t *engine);

PL_EXTERN void gx_expose(gp_engine_t *engine, gd_drawing_t *drawing, int *xy);
PL_EXTERN void gx_recenter(gp_x_engine_t *xEngine, int width, int height);

/* gx_animate creates an offscreen pixmap for the specified region of
   the window.  Subsequent drawing takes place on the pixmap, not
   on the graphics window.  gx_strobe copies the pixmap to the screen,
   optionally clearing it for the next frame of the animation.
   The viewport should be large enough to cover everything that will
   change as the animation proceeds, but no larger to get peak speed.
   gx_direct restores the usual direct-to-screen drawing mode.  */
PL_EXTERN int gx_animate(gp_engine_t *engine, gp_box_t *viewport);
PL_EXTERN int gx_strobe(gp_engine_t *engine, int clear);
PL_EXTERN int gx_direct(gp_engine_t *engine);

PL_EXTERN int gh_rgb_read(gp_engine_t *eng, gp_color_t *rgb, long *nx, long *ny);

#endif
