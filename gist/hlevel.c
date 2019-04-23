/*
 * $Id: hlevel.c,v 1.2 2007-12-28 20:20:18 thiebaut Exp $
 * Define routines for recommended GIST interactive interface
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist/hlevel.h"
#include "play/std.h"

#ifndef NO_XLIB
#ifndef ANIMATE_H
#include "gist/xbasic.h"
#else
#include ANIMATE_H
#endif
#else
#include "gist/engine.h"
static int gx_animate(gp_engine_t *engine, gp_box_t *viewport);
static int gx_strobe(gp_engine_t *engine, int clear);
static int gx_direct(gp_engine_t *engine);
static int gx_animate(gp_engine_t *engine, gp_box_t *viewport)
{ return 0; }
static int gx_strobe(gp_engine_t *engine, int clear)
{ return 0; }
static int gx_direct(gp_engine_t *engine)
{ return 0; }
#endif

static void UpdateOrRedraw(int changesOnly);

/* Here is a kludge to detect when gd_draw is about to be called to
   walk a gd_drawing_t.  The how argument has these meanings:
     even: called just before gd_draw
     odd:  called after following flush (or other related actions)
     0/1:  called from UpdateOrRedraw (just before prompt)
     2/3:  called from gh_fma (explicit frame advance)
     4/5:  called from gh_hcp (with hardcopy gp_engine_t, not display engine)
 */
extern void (*gdraw_hook)(gp_engine_t *display, int how);
void (*gdraw_hook)(gp_engine_t *display, int how)= 0;

/* ------------------------------------------------------------------------ */
/* See README for description of these control functions */

gh_device_t gh_devices[GH_NDEVS];

gp_engine_t *gh_hcp_default= 0;

static int currentDevice= -1;

static int hcpOn= 0;
static int animateOn= 0;

static int fmaCount= 0;

static void UpdateOrRedraw(int changesOnly)
{
  gp_engine_t *display= currentDevice<0? 0 : gh_devices[currentDevice].display;
  if (!display) return;
  gp_preempt(display);
  if (gdraw_hook) gdraw_hook(display, 0);
  gd_draw(changesOnly);
  gp_flush(0);
  if (gdraw_hook) gdraw_hook(display, 1);
  gp_preempt(0);
}

void (*gh_on_idle)(void) = 0;

void gh_before_wait(void)
{
#ifndef NO_XLIB
  extern void (*g_pending_task)(void);  /* auto disconnect, see xbasic.c */
  if (gh_on_idle) gh_on_idle();
  if (g_pending_task) g_pending_task();
#else
  if (gh_on_idle) gh_on_idle();
#endif
  if (currentDevice<0 || !gh_devices[currentDevice].display ||
      animateOn) return;  /* nothing happens in animate mode until
                           * explicit call to gh_fma */
  UpdateOrRedraw(1);
}

void gh_fma(void)
{
  gp_engine_t *display;
  gp_engine_t *hcp= 0;

  if (currentDevice<0) return;
  display= gh_devices[currentDevice].display;
  if (animateOn && !display) animateOn= 0;

  if (hcpOn) {
    hcp= gh_devices[currentDevice].hcp;
    if (!hcp) hcp= gh_hcp_default;
    if (hcp) gp_activate(hcp);
  }

  if (gdraw_hook) gdraw_hook(display, 2);
  gd_draw(1);
  if (hcpOn && hcp && gh_devices[currentDevice].doLegends)
    gd_draw_legends(hcp);
  if (animateOn) gx_strobe(display, 1);
  gp_flush(0);
  if (animateOn!=1) gd_clear(0);
  else gd_clear_system();
  if (gdraw_hook) gdraw_hook(display, 3);

  if (hcpOn && hcp) {
    gp_clear(hcp, GP_CONDITIONALLY);
    gp_deactivate(hcp);
  }

  gh_devices[currentDevice].fmaCount++;
  if (++fmaCount > 100) {  /* clean house once in a while */
    fmaCount= 0;
    ga_free_scratch();
  }
}

void gh_redraw(void)
{
  UpdateOrRedraw(-1);
}

void gh_hcp(void)
{
  gp_engine_t *hcp= currentDevice<0? 0 : gh_devices[currentDevice].hcp;
  if (!hcp) hcp= gh_hcp_default;
  if (!hcp) return;
  gp_preempt(hcp);
  if (gdraw_hook) gdraw_hook(hcp, 4);
  gd_draw(0);
  /* NB- must be very careful not to Preempt twice with gd_draw_legends */
  if (gh_devices[currentDevice].doLegends) gd_draw_legends(0);
  gp_clear(0, GP_ALWAYS);
  gp_flush(0);
  if (gdraw_hook) gdraw_hook(hcp, 5);
  gp_preempt(0);
}

void gh_fma_mode(int hcp, int animate)
{
  /* 0 off, 1 on, 2 no change, 3 toggle */
  if (hcp&2) hcpOn^= (hcp&1);   /* if 2 bit, XOR operation */
  else hcpOn= (hcp&1);          /* else, COPY operation */

  if ((animate&3)!=2) {
    gp_engine_t *display= currentDevice<0? 0 : gh_devices[currentDevice].display;
    if (!display) return;

    if ((animate&2) || (!animateOn)!=(!(animate&1))) {
      /* animation mode will actually change */
      animateOn= !animateOn;
      if (animateOn) {
        gp_box_t aport;
        gp_box_t *port= gd_clear_system();
        /* Can only animate using gd_clear_system if there is a current
           system; gd_clear_system returns appropriate box to animate,
           or 0 if can't.
           If no current system, then animate entire picture using
           ordinary gp_clear.  */
        aport.xmin= 0.0;
        aport.xmax= 2.0;
        aport.ymin= 0.0;
        aport.ymax= 2.0;
        /* if (!port) { just animate everything */
          port= &aport;
          animateOn= 2;
          /* } */
        if (gx_animate(display, port)) animateOn= 0;
      } else {
        gx_direct(display);
      }
    }
  }
}

int gh_set_plotter(int number)
{
  if (number<0 || number>=GH_NDEVS) return 1;

  if (currentDevice>=0) {
    if (gh_devices[currentDevice].display) {
      gd_set_drawing(gh_devices[currentDevice].drawing);
      gh_before_wait();
      gp_deactivate(gh_devices[currentDevice].display);
    }
    if (gh_devices[currentDevice].hcp)
      gp_deactivate(gh_devices[currentDevice].hcp);
  }
  if (gh_hcp_default) gp_deactivate(gh_hcp_default);

  currentDevice= number;
  if (gh_devices[number].display) gp_activate(gh_devices[number].display);
  return gd_set_drawing(gh_devices[number].drawing);
}

int gh_get_plotter(void)
{
  return currentDevice;
}

/* Query mouse system and position. */
int
gh_get_mouse(int *sys, double *x, double *y)
{
#ifdef NO_XLIB
  if (sys) *sys = -1;
  if (x) *x = 0.0;
  if (y) *y = 0.0;
  return -1;
#else
  int j, win = -1;
  if (gx_current_engine != NULL) {
    for (j = 0; j < GH_NDEVS; ++j) {
      if (gh_devices[j].display == gx_current_engine) {
	win = j;
	break;
      }
    }
  }
  if (win >= 0) {
    if (sys) *sys = gx_current_sys;
    if (x) *x = gx_current_x;
    if (y) *y = gx_current_y;
  } else {
    if (sys) *sys = -1;
    if (x) *x = 0.0;
    if (y) *y = 0.0;
  }
  return win;
#endif
}


#ifndef NO_XLIB
/* xbasic.c supplies a hook in its error handlers to allow the hlevel
   to clear its display devices */
static void ShutDownDev(gp_engine_t *engine);
extern void (*_gh_hook)(gp_engine_t *engine);

static void ShutDownDev(gp_engine_t *engine)
{
  int i;

  if (gh_hcp_default==engine) gh_hcp_default= 0;
  for (i=0 ; i<GH_NDEVS ; i++) {
    if (gh_devices[i].display==engine) {
      if (i==currentDevice) currentDevice= -1;
      gh_devices[i].display= 0;
    }
    if (gh_devices[i].hcp==engine) {
      if (!gh_devices[i].display && i==currentDevice) currentDevice= -1;
      gh_devices[i].hcp= 0;
    }
  }
}

int gh_set_xhandler(void (*handler)(char *msg))
{
  gp_set_xhandler(handler);
  _gh_hook = &ShutDownDev;
  return 0;
}

#else
/* ARGSUSED */
int gh_set_xhandler(void (*handler)(char *msg))
{
  /* no-op */
  return 0;
}
#endif

/* ------------------------------------------------------------------------ */
/* Default management */

static gp_line_attribs_t lDefault= { PL_FG, GP_LINE_SOLID, 1.0 };
static gp_marker_attribs_t mDefault= { PL_FG, 0, 1.0 };
static gp_fill_attribs_t fDefault= { PL_FG, GP_FILL_SOLID };
static gp_text_attribs_t tDefault= { PL_FG, 0, 0.0156,
                                   GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_NORMAL, GP_VERT_ALIGN_NORMAL };
static ga_line_attribs_t dlDefault= { 0, 0, 0, 0.16, 0.14, 0,
                                    0.13, 0.11375, 1.0, 1.0 };
static ga_vect_attribs_t vectDefault= { 0, 0.125 };
static gp_line_attribs_t edgeDefault= { PL_FG, GP_LINE_NONE, 1.0 };

void gh_get_lines(void)
{
  ga_attributes.l= lDefault;
  ga_attributes.m= mDefault;
  ga_attributes.dl= dlDefault;
}

void gh_get_text(void)
{
  ga_attributes.t= tDefault;
}

void gh_get_mesh(void)
{
  ga_attributes.l= lDefault;
}

void gh_get_vectors(void)
{
  ga_attributes.l= lDefault;
  ga_attributes.f= fDefault;
  ga_attributes.vect= vectDefault;
}

void gh_get_fill(void)
{
  ga_attributes.e= edgeDefault;
}

void gh_set_lines(void)
{
  lDefault= ga_attributes.l;
  mDefault= ga_attributes.m;
  mDefault.type= 0;    /* never a default marker */
  dlDefault= ga_attributes.dl;
}

void gh_set_text(void)
{
  tDefault= ga_attributes.t;
}

void gh_set_mesh(void)
{
  lDefault= ga_attributes.l;
}

void gh_set_vectors(void)
{
  lDefault= ga_attributes.l;
  fDefault= ga_attributes.f;
  vectDefault= ga_attributes.vect;
}

void gh_set_fill(void)
{
  edgeDefault= ga_attributes.e;
}

/* ------------------------------------------------------------------------ */

void gh_dump_colors(int n, int hcp, int pryvate)
{
  gp_engine_t *engine;
  if (n>=0 && n<GH_NDEVS) {
    if (hcp) engine= gh_devices[n].hcp;
    else engine= gh_devices[n].display;
  } else {
    engine= gh_hcp_default;
  }
  if (engine) gp_dump_colors(engine, pryvate);
}

int gh_get_colormode(gp_engine_t *engine)
{
  return engine->colorMode;
}

void gh_set_palette(int n, pl_col_t *palette, int nColors)
{
  if (n==-1) n = currentDevice;
  if (n<0 || n>=GH_NDEVS) return;
  if (gh_devices[n].display && gh_devices[n].display->palette!=palette) {
    gp_set_palette(gh_devices[n].display, palette, nColors);
    if (!gh_devices[n].display->colorMode) gh_redraw();
  }
  if (gh_devices[n].hcp && gh_devices[n].hcp->palette!=palette)
    gp_set_palette(gh_devices[n].hcp, palette, nColors);
}

int gh_read_palette(int n, const char *gpFile,
                  pl_col_t **palette, int maxColors)
{
  int paletteSize= 0;
  if (n==-1) n = currentDevice;
  else if (n<0 || n>=GH_NDEVS) return 0;
  if (gh_devices[n].display) {
    paletteSize= gp_read_palette(gh_devices[n].display, gpFile,
                               &gh_devices[n].display->palette, maxColors);
    if (gh_devices[n].hcp)
      gp_set_palette(gh_devices[n].hcp,
                   gh_devices[n].display->palette,
                   gh_devices[n].display->nColors);
    if (palette) *palette= gh_devices[n].display->palette;
    /* override the (possibly truncated) value returned by gp_read_palette */
    paletteSize= gh_devices[n].display->nColors;
    if (!gh_devices[n].display->colorMode) gh_redraw();
  } else if (gh_devices[n].hcp) {
    paletteSize= gp_read_palette(gh_devices[n].hcp, gpFile,
                               &gh_devices[n].hcp->palette, maxColors);
    if (palette) *palette= gh_devices[n].hcp->palette;
    /* override the (possibly truncated) value returned by gp_read_palette */
    paletteSize= gh_devices[n].hcp->nColors;
  }
  return paletteSize;
}

int gh_get_palette(int n, pl_col_t **palette)
{
  *palette= 0;
  if (n==-1) n = currentDevice;
  if (n<0 || n>=GH_NDEVS) return 0;
  if (gh_devices[n].display)
    return gp_get_palette(gh_devices[n].display, palette);
  else if (gh_devices[n].hcp)
    return gp_get_palette(gh_devices[n].hcp, palette);
  else
    return 0;
}

void gh_delete_palette(int n)
{
  pl_col_t *palette= 0;
  if (n<0 || n>=GH_NDEVS) return;
  if (gh_devices[n].display) palette= gh_devices[n].display->palette;
  else if (gh_devices[n].hcp) palette= gh_devices[n].hcp->palette;
  if (palette) {
    int i;

    /* clear palette for this device */
    if (gh_devices[n].display)
      gp_set_palette(gh_devices[n].display, (pl_col_t *)0, 0);
    if (gh_devices[n].hcp)
      gp_set_palette(gh_devices[n].hcp, (pl_col_t *)0, 0);

    /* free the palette if there are no other references to it */
    for (i=0 ; i<GH_NDEVS ; i++)
      if ((gh_devices[i].display &&
           gh_devices[i].display->palette==palette) ||
          (gh_devices[i].hcp &&
           gh_devices[i].hcp->palette==palette)) break;
    if (i>=GH_NDEVS) {
      if (gh_hcp_default && palette==gh_hcp_default->palette)
        gp_set_palette(gh_hcp_default, (pl_col_t *)0, 0);
      pl_free(palette);
    }
  }
}

void gh_set_hcp_palette(void)
{
  if (gh_hcp_default && currentDevice>=0) {
    pl_col_t *palette= 0;
    int nColors= 0;
    if (gh_devices[currentDevice].display) {
      palette= gh_devices[currentDevice].display->palette;
      nColors= gh_devices[currentDevice].display->nColors;
    } else if (gh_devices[currentDevice].hcp) {
      palette= gh_devices[currentDevice].hcp->palette;
      nColors= gh_devices[currentDevice].hcp->nColors;
    }
    gp_set_palette(gh_hcp_default, palette, nColors);
  }
}

/* ------------------------------------------------------------------------ */
