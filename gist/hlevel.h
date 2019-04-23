/*
 * $Id: gist/hlevel.h,v 1.4 2007-12-28 20:20:18 thiebaut Exp $
 * Declare routines for recommended GIST interactive interface
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_HLEVEL_H
#define _GIST_HLEVEL_H

#include <gist2.h>

/* See README for description of these control functions */

PL_EXTERN void gh_before_wait(void);
PL_EXTERN void gh_fma(void);
PL_EXTERN void gh_redraw(void);
PL_EXTERN void gh_hcp(void);
PL_EXTERN void gh_fma_mode(int hcp, int animate); /* 0 off, 1 on, 2 nc, 3 toggle*/

/* The pldevice call should create the necessary engines, and set their
   gp_engine_t pointers in gh_devices, then call gh_set_plotter to set the
   current device, deactivating the old device.  gh_set_plotter returns
   0 if successful, 1 if neither display nor hcp engine has been defined
   for the requested device.  gh_get_plotter returns the number of the
   current plot device, or -1 if none has been set.  */

typedef struct _gh_device gh_device_t;
struct _gh_device {
  gd_drawing_t *drawing;
  gp_engine_t *display, *hcp;
  int doLegends;
  int fmaCount;
  void *hook;
};

/* Allow up to GH_NDEVS windows per application */
#ifndef GH_NDEVS
# define GH_NDEVS 64
#endif
PL_EXTERN gh_device_t gh_devices[GH_NDEVS];

PL_EXTERN int gh_set_plotter(int number);
PL_EXTERN int gh_get_plotter(void);

/* The default hardcopy device is used for hcp commands whenever the
   current device has no hcp engine of its own.  */
PL_EXTERN gp_engine_t *gh_hcp_default;

PL_EXTERN void gh_dump_colors(int n, int hcp, int pryvate);
PL_EXTERN void gh_set_palette(int n, pl_col_t *palette, int nColors);
PL_EXTERN int gh_read_palette(int n, const char *gpFile,
                           pl_col_t **palette, int maxColors);
PL_EXTERN int gh_get_palette(int n, pl_col_t **palette);
PL_EXTERN int gh_get_colormode(gp_engine_t *engine);
PL_EXTERN void gh_set_hcp_palette(void);
PL_EXTERN void gh_delete_palette(int n);

/* A high-level error handler takes down an X-window before calling
   the user-installed error handler.  This prevents a huge blast of
   errors when a window is detroyed bby a window manager (for example),
   but is obviously a litle more fragile than a smart error handler
   could be.  */
PL_EXTERN int gh_set_xhandler(void (*XHandler)(char *msg));

/* For each of the D level drawing primitives, a set of
   default parameter settings is maintained, and can be installed
   with the appropriate GhGet routine.  The GhSet routines set the
   defaults themselves.  gd_add_cells does not use any attributes,
   and gd_add_contours uses the same attributes as gd_add_lines.
   gd_add_fillmesh uses line attributes for edges (if any).  */
PL_EXTERN void gh_get_lines(void);
PL_EXTERN void gh_get_text(void);
PL_EXTERN void gh_get_mesh(void);
PL_EXTERN void gh_get_vectors(void);
PL_EXTERN void gh_get_fill(void);

PL_EXTERN void gh_set_lines(void);
PL_EXTERN void gh_set_text(void);
PL_EXTERN void gh_set_mesh(void);
PL_EXTERN void gh_set_vectors(void);
PL_EXTERN void gh_set_fill(void);

/* The gp_new_fx_engine (fancy X engine) has controls for the zoom factor
   and a function for initiating a point-and-click sequence.  */

PL__EXTERN gp_real_t gx_zoom_factor;   /* should be >1.0, default is 1.5 */

/* The gx_point_click function initiates an interactive point-and-click
   session with the window -- it will not return until a button has
   been pressed, then released.  It returns non-zero if the operation
   was aborted by pressing a second button before releasing the first.
     engine --   an X engine whose display is to be used
     style --    1 to draw a rubber box, 2 to draw a rubber line,
                 otherwise, no visible indication of operation
     system --   system number to which the world coordinates should
                 be transformed, or -1 to use the system under the
                 pointer -- the release coordinates are always in the
                 same system as the press coordinates
     CallBack -- function to be called twice, first when the button is
                 pressed, next when it is released -- operation will
                 be aborted if CallBack returns non-zero
                 Arguments passed to CallBack:
                   engine  -- in which press/release occurred
                   system  -- system under pointer, if above system -1
                   release -- 0 on press, 1 on release
                   x, y    -- coordinates of pointer relative to system
                   butmod  -- 1 - 5 on press to tell which button
                              mask to tell which modifiers on release:
                              1 shift, 2 lock, 4 control, 8 - 128 mod1-5
                   xn, yn  -- NDC coordinates of pointer
 */
PL__EXTERN int gx_point_click(gp_engine_t *engine, int style, int system,
                           int (*CallBack)(gp_engine_t *engine, int system,
                                           int release, gp_real_t x, gp_real_t y,
                                           int butmod, gp_real_t xn, gp_real_t yn));

/* Variables to store engine which currently has mouse focus and
   coordinate system and mouse coordinates after last mouse motion. */
PL__EXTERN gp_engine_t *gx_current_engine;
PL__EXTERN int gx_current_sys;
PL__EXTERN gp_real_t gx_current_x, gx_current_y;

/* The gh_get_mouse function stores the current coordinate system and
   mouse position at SYS, X and Y repectively (any of them can be
   NULL) and returns the device number which has the mouse focus.
   If no device currently has the focus, -1 is returned. */
PL_EXTERN int gh_get_mouse(int *sys, double *x, double *y);

PL__EXTERN int gh_rgb_read(gp_engine_t *eng, gp_color_t *rgb, long *nx, long *ny);
PL_EXTERN void (*gh_on_idle)(void);

/* auto disconnect, see xbasic.c */
PL_EXTERN void (*gh_pending_task)(void);

#endif
