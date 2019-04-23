/*
 * $Id: gist/xfancy.h,v 1.4 2007-12-28 20:20:19 thiebaut Exp $
 * Declare the fancy X windows engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_XFANCY_H
#define _GIST_XFANCY_H

#include <gist/xbasic.h>

typedef struct _gp_fx_engine gp_fx_engine_t;
struct _gp_fx_engine {
  gp_x_engine_t xe;

  /* --------------- Specific to gp_fx_engine_t ------------------- */

  int baseline;    /* y coordinate for text in button and message windows */
  int heightButton, widthButton;  /* shape of button */
  /* height of both button and message windows is xe.topMargin */

  int xmv, ymv;
  int pressed;      /* 0 - none, 1 - in button, 2 - in graphics */
  int buttonState;  /* 0 -  button inactive
                       1 -  pointer in button, button ready
                       2 -  button pressed, button armed  */
  int iSystem;      /* <0 for unlocked, else locked system number */
  char msgText[96]; /* current text displayed in message window */
  int msglen;       /* non-zero if displaying a command as message */
  int zoomState;    /* button number if zoom or pan in progress, else 0 */
  int zoomSystem;   /* system number in xe.drawing */
  int zoomAxis;     /* 1 for x-axis, 2 for y-axis, 3 for both */
  gp_real_t zoomX, zoomY; /* initial coordinates for zoom/pan operation */
};

/* zoom factor for point-and-click zooming */
PL_EXTERN gp_real_t gx_zoom_factor;

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
PL_EXTERN int gx_point_click(gp_engine_t *engine, int style, int system,
                          int (*CallBack)(gp_engine_t *engine, int system,
                                          int release, gp_real_t x, gp_real_t y,
                                          int butmod, gp_real_t xn, gp_real_t yn));

/* Variables to store coordinate system and mouse coordinates after
   last mouse motion (also see gx_current_engine in gist/xbasic.h). */
PL_EXTERN int gx_current_sys;
PL_EXTERN gp_real_t gx_current_x, gx_current_y;

#endif