/*
 * $Id: xfancy.c,v 1.7 2008-10-28 04:02:30 dhmunro Exp $
 * Implement the basic X windows engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist/xfancy.h"
#include "gist/draw.h"

#include <string.h>

/* possibly should just include stdio.h, math.h */
extern int sprintf(char *s, const char *format, ...);
extern double log10(double);

#ifndef NO_EXP10
  extern double exp10(double);
#else
# define exp10(x) pow(10.,x)
  extern double pow(double,double);
#endif

/* ------------------------------------------------------------------------ */

static void HandleExpose(gp_engine_t *engine, gd_drawing_t *drawing, int *xy);
static void HandleClick(gp_engine_t *e,int b,int md,int x,int y, unsigned long ms);
static void HandleMotion(gp_engine_t *e,int md,int x,int y);
static void HandleKey(gp_engine_t *e,int k,int md);

static void RedrawMessage(gp_fx_engine_t *fxe);
static void MovePointer(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
                        int md,int x,int y);
static void PressZoom(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
                      int b,int md,int x,int y, unsigned long ms);
static void ReleaseZoom(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
                        int b,int md,int x,int y, unsigned long ms);

static void redraw_seps(gp_fx_engine_t *fxe);
static void check_clipping(gp_fx_engine_t *fxe);
static void RedrawButton(gp_fx_engine_t *fxe);
static void EnterButton(gp_fx_engine_t *fxe);
static void LeaveButton(gp_fx_engine_t *fxe);
static void PressButton(gp_fx_engine_t *fxe,
                        int b,int md,int x,int y, unsigned long ms);
static void ReleaseButton(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
                          int b,int md,int x,int y, unsigned long ms);

static void ButtonAction(gp_fx_engine_t *fxe, gd_drawing_t *drawing);
static void HighlightButton(gp_fx_engine_t *fxe);
static void UnHighlightButton(gp_fx_engine_t *fxe);

static void ResetZoom(gp_fx_engine_t *fxe);
static void DoZoom(gp_real_t factor, gp_real_t w0, gp_real_t w1,
                   gp_real_t *wmin, gp_real_t *wmax);
static void AltZoom(gp_real_t w0, gp_real_t w1, gp_real_t *wmin, gp_real_t *wmax);
static int FindSystem(gp_fx_engine_t *fxe, gd_drawing_t *drawing, int x, int y,
                      ge_system_t **system, gp_real_t *xr, gp_real_t *yr);
static void Find1System(gp_fx_engine_t *fxe, gd_drawing_t *drawing, int iSystem,
                        int x, int y, ge_system_t **system,
                        gp_real_t *xr, gp_real_t *yr);
static ge_system_t *GetSystemN(gd_drawing_t *drawing, int n);
static int FindAxis(ge_system_t *system, gp_real_t x, gp_real_t y);
static void FindCoordinates(ge_system_t *system, gp_real_t xNDC, gp_real_t yNDC,
                            gp_real_t *xWC, gp_real_t *yWC);
static gp_real_t GetFormat(char *format, gp_real_t w,
                        gp_real_t wmin, gp_real_t wmax, int isLog);
static void DrawRubber(gp_fx_engine_t *fxe, int x, int y);

/* ------------------------------------------------------------------------ */

gp_engine_t *
gp_new_fx_engine(char *name, int landscape, int dpi, char *displayName)
{
  pl_scr_t *s = gx_connect(displayName);
  int topWidth= GX_DEFAULT_TOP_WIDTH(dpi);   /* not including button, message */
  int topHeight= GX_DEFAULT_TOP_HEIGHT(dpi);
  gp_transform_t toPixels;
  int x, y;
  gp_fx_engine_t *fxe;
  int heightButton, widthButton, baseline;
  int width, height, ascent, descent, hints;

  if (!s) return 0;
  descent = pl_txheight(s, PL_GUI_FONT, 15, &ascent);
  descent -= ascent;
  width = pl_txwidth(s, "System", 6, PL_GUI_FONT, 15);
  baseline = ascent+2;
  heightButton = baseline+descent+4;  /* leave at least 2 lines
                                       * above and below text */
  widthButton = width+8;  /* leave at least 4 pixels before and after */

  /* set toPixels as by SetXTransform(&toPixels, landscape, dpi) */
  toPixels.viewport = landscape? gp_landscape_box : gp_portrait_box;
  toPixels.window.xmin = 0.0;
  toPixels.window.xmax = GX_PIXELS_PER_NDC(dpi)*toPixels.viewport.xmax;
  toPixels.window.ymin = GX_PIXELS_PER_NDC(dpi)*toPixels.viewport.ymax;
  toPixels.window.ymax = 0.0;
  width = (int)toPixels.window.xmax;
  height = (int)toPixels.window.ymin;
  x = (width-topWidth)/2;
  if (landscape) y = (height-topHeight)/2;
  else y = (width-topHeight)/2;
  if (y<0) y = 0;
  if (x<0) x = 0;
  fxe = (gp_fx_engine_t *)gx_new_engine(s, name, &toPixels, -x,-y,heightButton+2,0,
                             sizeof(gp_fx_engine_t));

  fxe->xe.wtop = topWidth;
  fxe->xe.htop = topHeight;
  /* possibly want optional PL_RGBMODEL as well */
  hints = (gp_private_map?PL_PRIVMAP:0) | (gp_input_hint?0:PL_NOKEY) |
    (gp_rgb_hint?PL_RGBMODEL:0);
  fxe->xe.win = fxe->xe.w = gx_parent?
    pl_subwindow(s, topWidth, topHeight+heightButton+2,
                gx_parent, gx_xloc, gx_yloc, PL_BG, hints, fxe) :
    pl_window(s, topWidth, topHeight+heightButton+2, name, PL_BG, hints, fxe);
  gx_parent = 0;
  if (!fxe->xe.win) {
    gp_delete_engine(&fxe->xe.e);
    return 0;
  }

  /* set the extra information in the fxe structure */
  fxe->baseline = baseline;
  fxe->heightButton = heightButton;
  fxe->widthButton = widthButton;

  fxe->xmv = fxe->ymv = -1;
  fxe->pressed = 0;
  fxe->buttonState = 0;
  fxe->iSystem = -1;
  strcpy(fxe->msgText, "Press 1, 2, 3 to zoom in, pan, zoom out");
  fxe->msglen = 0;
  fxe->zoomState = fxe->zoomSystem = fxe->zoomAxis = 0;
  fxe->zoomX = fxe->zoomY = 0.0;

  /* set the fancy event handlers */
  gx_input((gp_engine_t *)fxe, &HandleExpose, &HandleClick, &HandleMotion,
          &HandleKey);

  return (gp_engine_t *)fxe;
}

/* ------------------------------------------------------------------------ */

static void
HandleExpose(gp_engine_t *engine, gd_drawing_t *drawing, int *xy)
{
  gp_fx_engine_t *fxe = (gp_fx_engine_t *)engine;
  redraw_seps(fxe);
  RedrawButton(fxe);
  RedrawMessage(fxe);
  if (!xy || xy[3]>=fxe->xe.topMargin)
    gx_expose(engine, drawing, xy);
}

static int xf_region(gp_fx_engine_t *fxe, int x, int y);

static void
HandleClick(gp_engine_t *e,int b,int md,int x,int y, unsigned long ms)
{
  gp_fx_engine_t *fxe = (gp_fx_engine_t *)e;
  if (x!=fxe->xmv || y!=fxe->ymv)
    HandleMotion(e, md, x, y);
  if (md & (1<<(b+2))) {  /* this is button release */
    if (fxe->pressed==1)
      ReleaseButton(fxe, fxe->xe.e.drawing, b, md, x, y, ms);
    else if (fxe->pressed==2)
      ReleaseZoom(fxe, fxe->xe.e.drawing, b, md, x, y, ms);

  } else {                /* this is button press */
    int region = xf_region(fxe, x, y);
    if (region == 3)
      PressZoom(fxe, fxe->xe.e.drawing, b, md, x, y, ms);
    else if (region == 1)
      PressButton(fxe, b, md, x, y, ms);
  }
}

static void
HandleMotion(gp_engine_t *e,int md,int x,int y)
{
  gp_fx_engine_t *fxe = (gp_fx_engine_t *)e;
  int region = xf_region(fxe, x, y);
  int old_region = xf_region(fxe, fxe->xmv, fxe->ymv);
  if (!fxe->pressed) {
    if (region != old_region) {
      if (old_region==3) pl_cursor(fxe->xe.win, PL_SELECT);
      else if (region==3) pl_cursor(fxe->xe.win, PL_CROSSHAIR);
    }
    if (region==1) {
      if (old_region!=1) EnterButton(fxe);
    } else {
      if (old_region==1) LeaveButton(fxe);
    }
  }
  if (fxe->pressed==1) {
    if (region!=1 && old_region==1)
      LeaveButton(fxe);
  } else if (region==3 || fxe->pressed==2) {
    MovePointer(fxe, fxe->xe.e.drawing, md, x, y);
  }
  fxe->xmv = x;
  fxe->ymv = y;
}

static int
xf_region(gp_fx_engine_t *fxe, int x, int y)
{
  if (y>=0 && x>=0 && x<fxe->xe.leftMargin+fxe->xe.wtop) {
    y -= fxe->xe.topMargin;
    if (y < 0)
      return (x < fxe->widthButton)? 1 : 2;
    else if (y < fxe->xe.htop)
      return 3;
  }
  return 0;
}

static void
HandleKey(gp_engine_t *e,int k,int md)
{
  gp_fx_engine_t *fxe = (gp_fx_engine_t *)e;
  if (!fxe->pressed && !fxe->buttonState) {
    if (!fxe->msglen) fxe->msgText[0] = '\0';
    if (k>=' ' && k<'\177') {
      /* append printing characters */
      if (fxe->msglen>=94) fxe->msglen = 0;  /* crude overflow handling */
      fxe->msgText[fxe->msglen++] = k;
      fxe->msgText[fxe->msglen] = '\0';
    } else if (k=='\177' || k=='\010') {
      /* delete or backspace kills char */
      if (fxe->msglen)
        fxe->msgText[--fxe->msglen] = '\0';
    } else if (k=='\027') {
      /* C-w kills word */
      int n = fxe->msglen;
      char c = n? fxe->msgText[n-1] : '\0';
      if (c<'0' || (c>'9' && c<'A') || (c>'Z' && c<'a' && c!='_') || c>'z') {
        if (fxe->msglen)
          fxe->msgText[--fxe->msglen] = '\0';
      } else {
        while (--n) {
          c = fxe->msgText[n-1];
          if (c<'0' || (c>'9' && c<'A') || (c>'Z' && c<'a' && c!='_') ||
              c>'z') break;
        }
        fxe->msgText[n] = '\0';
        fxe->msglen = n;
      }
    } else if (k=='\025') {
      /* C-u kills line */
      fxe->msgText[0] = '\0';
      fxe->msglen = 0;
    } else if (k=='\012' || k=='\015') {
      /* linefeed or carriage return sends line to interpreter */
      int n = fxe->msglen;
      fxe->msgText[n] = '\0';
      if (gp_on_keyline) gp_on_keyline(fxe->msgText);
      fxe->msgText[0] = '\0';
      fxe->msglen = 0;
    }
    RedrawMessage(fxe);
  }
}

/* ------------------------------------------------------------------------ */

static void
check_clipping(gp_fx_engine_t *fxe)
{
  pl_win_t *w = fxe->xe.win;
  if (w) {
    if (fxe->xe.clipping) {
      pl_clip(w, 0, 0, 0, 0);
      fxe->xe.clipping = 0;
    }
  }
}

static void
redraw_seps(gp_fx_engine_t *fxe)
{
  pl_win_t *w = fxe->xe.win;
  if (w) {
    int xpage = (int)fxe->xe.swapped.window.xmax;
    int ypage = (int)fxe->xe.swapped.window.ymin;
    int x[4], y[4];
    check_clipping(fxe);
    pl_color(w, PL_FG);
    if (fxe->xe.leftMargin+fxe->xe.wtop > xpage) {
      pl_pen(w, 4, PL_SOLID);
      x[0] = x[1] = xpage+2;
      y[0] = 0;  y[1] = ypage+2;
      pl_i_pnts(w, x, y, 2);
      pl_segments(w);
    }
    if (fxe->xe.topMargin+fxe->xe.htop > ypage) {
      pl_pen(w, 4, PL_SOLID);
      x[0] = 0;  x[1] = xpage+2;
      y[0] = y[1] = ypage+2;
      pl_i_pnts(w, x, y, 2);
      pl_segments(w);
    }
    pl_pen(w, 1, PL_SOLID);
    x[0] = 0;  x[1] = fxe->xe.wtop;
    y[0] = y[1] = fxe->xe.topMargin-1;
    x[2] = x[3] = fxe->widthButton;
    y[2] = 0;  y[3] = fxe->xe.topMargin-1;
    pl_i_pnts(w, x, y, 4);
    pl_segments(w);
  }
}

static void
RedrawButton(gp_fx_engine_t *fxe)
{
  pl_win_t *w = fxe->xe.win;
  if (w) {
    int fg = (fxe->buttonState==2)? PL_BG : PL_FG;
    int bg = (fxe->buttonState==2)? PL_FG : PL_BG;
    check_clipping(fxe);
    pl_color(w, bg);
    pl_rect(w, 0, 0, fxe->widthButton, fxe->xe.topMargin-1, 0);
    if (fxe->buttonState) HighlightButton(fxe);
    else pl_color(w, fg);
    pl_font(w, PL_GUI_FONT, 15, 0);
    pl_text(w, 3, fxe->baseline, "System", 6);
  }
}

static void
HighlightButton(gp_fx_engine_t *fxe)
{
  pl_win_t *w = fxe->xe.win;
  if (w && fxe->buttonState) {
    check_clipping(fxe);
    pl_color(w, (fxe->buttonState==2)? PL_BG : PL_FG);
    pl_pen(w, 3, PL_SOLID);
    pl_rect(w, 1, 1, fxe->widthButton-2, fxe->xe.topMargin-3, 1);
  }
}

static void
UnHighlightButton(gp_fx_engine_t *fxe)
{
  pl_win_t *w = fxe->xe.win;
  if (w) {
    check_clipping(fxe);
    pl_color(w, (fxe->buttonState==2)? PL_FG : PL_BG);
    pl_pen(w, 3, PL_SOLID);
    pl_rect(w, 1, 1, fxe->widthButton-2, fxe->xe.topMargin-3, 1);
  }
}

static void
EnterButton(gp_fx_engine_t *fxe)
{
  if (fxe->buttonState == 0) {
    fxe->buttonState = 1;
    HighlightButton(fxe);
  } else if (fxe->buttonState != 0) {
    fxe->buttonState = 0;
    RedrawButton(fxe);
  }
}

static void
LeaveButton(gp_fx_engine_t *fxe)
{
  int state = fxe->buttonState;
  fxe->buttonState = fxe->pressed = 0;
  if (state==1) UnHighlightButton(fxe);
  else if (state) RedrawButton(fxe);
}

static void
PressButton(gp_fx_engine_t *fxe,int b,int md,int x,int y, unsigned long ms)
{
  if (!(md&0370) && fxe->buttonState==1) {
    fxe->buttonState = 2;
    fxe->pressed = 1;
    RedrawButton(fxe);
  } else {
    LeaveButton(fxe);
  }
}

static void
ReleaseButton(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
              int b,int md,int x,int y, unsigned long ms)
{
  if (fxe->buttonState==2) {
    fxe->buttonState = 1;
    fxe->pressed = 0;
    RedrawButton(fxe);
    ButtonAction(fxe, drawing);
  } else {
    LeaveButton(fxe);
  }
}

/* ------------------------------------------------------------------------ */

static void ButtonAction(gp_fx_engine_t *fxe, gd_drawing_t *drawing)
{
  int nSystems= drawing? drawing->nSystems : 0;
  int iSystem= fxe->iSystem;
  iSystem++;
  if (iSystem<=nSystems) fxe->iSystem= iSystem;
  else fxe->iSystem= iSystem= -1;
  sprintf(fxe->msgText, "%s%2d", iSystem>=0?"=":":",
          iSystem>=0 ? iSystem : 0);
  RedrawMessage(fxe);
}

/* ------------------------------------------------------------------------ */

static void RedrawMessage(gp_fx_engine_t *fxe)
{
  pl_win_t *w = fxe->xe.win;
  if (w) {
    char *msg= fxe->msgText;
    int len= (int)strlen(msg);
    check_clipping(fxe);
    /* NOTE: may need to animate this if flickering is annoying */
    pl_color(w, PL_BG);
    pl_rect(w, fxe->widthButton+1, 0, fxe->xe.wtop, fxe->xe.topMargin-2, 0);
    pl_color(w, PL_FG);
    pl_font(w, PL_GUI_FONT, 15, 0);
    pl_text(w, fxe->widthButton+4, fxe->baseline, msg, len);
  }
}

static char stdFormat[] = "%7.4f";
static int rubberBanding = 0;
static int anchorX, anchorY, oldX, oldY;


/* Variables to store coordinate system and mouse coordinates after
   last mouse motion. */
int gx_current_sys = -1;
gp_real_t gx_current_x = 0, gx_current_y = 0;

static void
MovePointer(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
            int md,int x,int y)
{
  int iSystem = fxe->iSystem;
  int locked, logX = 0, logY = 0;
  char format[24];  /* e.g.- "%s%2d (%11.3e, %11.3e)" */
  char xFormat[16], yFormat[16], *f1, *f2;
  gp_real_t xWC, yWC;
  ge_system_t *system;

  if (!drawing || fxe->buttonState) return;

  /* find the system number and world coordinates */
  if (iSystem >= 0) {
    /* coordinate system is locked */
    Find1System(fxe, drawing, iSystem, x, y, &system, &xWC, &yWC);
    if (!system) iSystem = 0;
    locked = 1;
  } else {
    /* select coordinate system under pointer */
    iSystem = FindSystem(fxe, drawing, x, y, &system, &xWC, &yWC);
    locked = 0;
  }

  if (!fxe->msglen) {
    if (system) {
      logX = system->flags&GD_LIM_LOGX;
      logY = system->flags&GD_LIM_LOGY;
      xWC = GetFormat(xFormat, xWC, system->trans.window.xmin,
                      system->trans.window.xmax, logX);
      f1 = xFormat;
      yWC = GetFormat(yFormat, yWC, system->trans.window.ymin,
                      system->trans.window.ymax, logY);
      f2 = yFormat;
    } else {
      f1 = f2 = stdFormat;
    }
    sprintf(format, "%%s%%2d (%s, %s)", f1, f2);
    sprintf(fxe->msgText, format, locked? "=" : ":", iSystem, xWC, yWC);

    /* Save mouse coordinates. */
    gx_current_x = xWC;
    gx_current_y = yWC;
    gx_current_sys = iSystem;
    gx_current_engine = (gp_engine_t *)fxe;

    RedrawMessage(fxe);
  }
  if (rubberBanding) DrawRubber(fxe, x, y);
}

gp_real_t gx_zoom_factor= 1.5;

static int (*PtClCallBack)(gp_engine_t *engine, int system,
                           int release, gp_real_t x, gp_real_t y,
                           int butmod, gp_real_t xn, gp_real_t yn) = 0;
static int ptClStyle = 0, ptClSystem = 0, ptClCount = 0;

static unsigned int cShapes[3] = { PL_EW, PL_NS, PL_NSEW };

static void
PressZoom(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
          int b,int md,int x,int y, unsigned long ms)
{
  if (!drawing || !fxe->xe.win) return;
  if (fxe->zoomState == 0 && fxe->pressed == 0) {
    /* record button number as zoomState */
    if ((b==1)? (md&PL_SHIFT) : (b==2)) fxe->zoomState = 2;
    else if ((b==1)? (md&PL_META) : (b==3)) fxe->zoomState = 3;
    else if (b==1) fxe->zoomState = 1;
    if (fxe->zoomState) {
      if (PtClCallBack)
        fxe->zoomState |= 8;
      else if (md&PL_CONTROL)
        fxe->zoomState |= 4;
    }
    if (fxe->zoomState) {
      /* record system and axis, x and y, change cursor */
      ge_system_t *system;
      int iSystem, axis;
      if (!PtClCallBack || ptClSystem<0) {
        iSystem = FindSystem(fxe, drawing, x, y,
                             &system, &fxe->zoomX, &fxe->zoomY);
        axis = FindAxis(system, fxe->zoomX, fxe->zoomY);
      } else {
        iSystem = ptClSystem;
        Find1System(fxe, drawing, iSystem, x, y,
                    &system, &fxe->zoomX, &fxe->zoomY);
        if (!system) iSystem = ptClSystem = 0;
        axis = 3;
      }
      fxe->zoomSystem = iSystem;

      fxe->pressed = 2;

      if (PtClCallBack) {
        gp_xymap_t *map = &fxe->xe.e.map;   /* NDC->VDC (x,y) mapping */
        gp_real_t xNDC = ((gp_real_t)x - map->x.offset)/map->x.scale;
        gp_real_t yNDC = ((gp_real_t)y - map->y.offset)/map->y.scale;
        int button = fxe->zoomState&3;
        ptClCount--;
        if (PtClCallBack(&fxe->xe.e, iSystem, 0,
                         fxe->zoomX, fxe->zoomY,
                         button, xNDC, yNDC)) {
          /* callback routine signals abort */
          ResetZoom(fxe);
          return;
        }
      }

      if ((fxe->zoomAxis = axis)) {
        if (fxe->zoomState<4) {
          pl_cursor(fxe->xe.win, cShapes[axis-1]);
        } else if (!PtClCallBack || ptClStyle) {
          fxe->zoomAxis = 3;
          pl_cursor(fxe->xe.win, PL_SELECT);
          anchorX = oldX = x;
          anchorY = oldY = y;
          rubberBanding = PtClCallBack? ptClStyle : 1;
          DrawRubber(fxe, anchorX, anchorY);
        }
      } else if (!PtClCallBack) {
        /* no such thing as zoom or pan outside of all systems */
        fxe->zoomState = 0;
      }
    }

  } else {
    /* abort if 2nd button pressed */
    fxe->pressed = 0;
    ResetZoom(fxe);
  }
}

static void
ReleaseZoom(gp_fx_engine_t *fxe, gd_drawing_t *drawing,
            int b,int md,int ix,int iy, unsigned long ms)
{
  int zoomState = fxe->zoomState;
  if (zoomState) {
    /* perform the indicated zoom operation */
    int iSystem = fxe->zoomSystem;
    ge_system_t *system = GetSystemN(drawing, iSystem);
    gp_xymap_t *map = &fxe->xe.e.map;   /* NDC->VDC (x,y) mapping */
    gp_real_t x, xNDC = ((gp_real_t)ix - map->x.offset)/map->x.scale;
    gp_real_t y, yNDC = ((gp_real_t)iy - map->y.offset)/map->y.scale;

    /* get current pointer position (in world coordinates) */
    if (system &&
        /* be sure system has been scanned if limits extreme */
        (!(system->rescan || system->unscanned>=0) || !gd_scan(system))) {
      FindCoordinates(system, xNDC, yNDC, &x, &y);
    } else {
      x = xNDC;
      y = yNDC;
      iSystem = ptClSystem = 0;
    }

    if (!PtClCallBack) {
      int axis = fxe->zoomAxis;
      gp_real_t factor = 1.0;

      if (!iSystem) {
        fxe->pressed = 0;
        ResetZoom(fxe);
        return;
      }

      if (zoomState==1) factor = 1.0/gx_zoom_factor;
      else if (zoomState==3) factor = gx_zoom_factor;

      /* the redraw triggered here can mess up a rubber band box/line */
      if (rubberBanding) {
        DrawRubber(fxe, anchorX, anchorY);
        rubberBanding = 0;
      }

      /* switch to current drawing and engine temporarily, then save
         limits if this is first mouse-driven zoom */
      gd_set_drawing(drawing);
      gp_preempt((gp_engine_t *)fxe);
      if (!(system->flags&GD_LIM_ZOOMED)) gd_save_limits(0);

      if (zoomState<4) {
        if (axis&1) DoZoom(factor, fxe->zoomX, x,
                           &system->trans.window.xmin,
                           &system->trans.window.xmax);
        if (axis&2) DoZoom(factor, fxe->zoomY, y,
                           &system->trans.window.ymin,
                           &system->trans.window.ymax);
      } else {
        if (axis&1) AltZoom(fxe->zoomX, x, &system->trans.window.xmin,
                            &system->trans.window.xmax);
        if (axis&2) AltZoom(fxe->zoomY, y, &system->trans.window.ymin,
                            &system->trans.window.ymax);
      }
      system->flags&= ~(GD_LIM_XMIN | GD_LIM_XMAX | GD_LIM_YMIN | GD_LIM_YMAX);
      system->flags|= GD_LIM_ZOOMED;
      system->rescan= 1;

      /* The redraw must be done immediately -- nothing else triggers it.  */
      gd_draw(1);
      gp_preempt(0);     /* not correct if zoomed during a preempt... */
      gd_set_drawing(0);

    } else {
      int modifier = 0;
      if (md&PL_SHIFT) modifier|= 1;
      /* else if (state&LockMask) modifier|= 2; */
      else if (md&PL_CONTROL) modifier|= 4;
      else if (md&PL_META) modifier|= 8;
      else if (md&PL_ALT) modifier|= 16;
      else if (md&PL_COMPOSE) modifier|= 32;
      else if (md&PL_KEYPAD) modifier|= 64;
      /* else if (state&Mod5Mask) modifier|= 128; */
      ptClCount--;
      PtClCallBack(&fxe->xe.e, iSystem, 1, x, y, modifier, xNDC, yNDC);
    }

    /* free the zoom/pan cursor and reset zoomState */
    fxe->pressed = 0;
    ResetZoom(fxe);
  } else if (fxe->pressed==2) {
    fxe->pressed = 0;
  }
}

static void
ResetZoom(gp_fx_engine_t *fxe)
{
  int (*cback)(gp_engine_t *engine, int system,
               int release, gp_real_t x, gp_real_t y,
               int butmod, gp_real_t xn, gp_real_t yn) = PtClCallBack;
  /* free the zoom cursor and reset the zoom state */
  if (rubberBanding) {
    DrawRubber(fxe, anchorX, anchorY);
    rubberBanding = 0;
  }
  if (fxe->zoomState && fxe->xe.win)
    pl_cursor(fxe->xe.win, PL_CROSSHAIR);
  fxe->zoomState = 0;
  PtClCallBack = 0;
  if (cback) cback(0, -1, -1, 0., 0., -1, 0., 0.);
}

static void DoZoom(gp_real_t factor, gp_real_t w0, gp_real_t w1,
                   gp_real_t *wmin, gp_real_t *wmax)
{
  gp_real_t wn= *wmin;
  gp_real_t wx= *wmax;
  /* With origin at the release point, expand the scale, then put this origin
     at the press point.  This is a translation (pan) if factor==1.0.  */
  *wmin= w0-factor*(w1-wn);
  *wmax= w0+factor*(wx-w1);
}

static void AltZoom(gp_real_t w0, gp_real_t w1, gp_real_t *wmin, gp_real_t *wmax)
{
  gp_real_t wn= *wmin;
  gp_real_t wx= *wmax;
  /* Expand marked interval to full scale.  */
  if (wn<wx) {
    if (w0<w1) { *wmin= w0; *wmax= w1; }
    else if (w0>w1) { *wmin= w1; *wmax= w0; }
    else {
      wx= wx-wn;
      if (!wx) { if (wn) wx=0.0001*wn; else wx= 1.e-6; }
      else wx*= 0.01;
      *wmin= w0-wx; *wmax= w1+wx;
    }
  } else {
    if (w0<w1) { *wmin= w1; *wmax= w0; }
    else if (w0>w1) { *wmin= w0; *wmax= w1; }
    else {
      wx= wn-wx;
      if (!wx) { if (wn) wx=0.0001*wn; else wx= 1.e-6; }
      else wx*= 0.01;
      *wmin= w0+wx; *wmax= w1-wx;
    }
  }
}

static int FindSystem(gp_fx_engine_t *fxe, gd_drawing_t *drawing, int x, int y,
                      ge_system_t **system, gp_real_t *xr, gp_real_t *yr)
{
  ge_system_t *sys= drawing->systems, *thesys=sys;
  int nSystems= drawing->nSystems;
  gp_xymap_t *map= &fxe->xe.e.map;  /* NDC->VDC (x,y) mapping */
  gp_real_t xn, yn;
  gp_box_t *box;
  int i, iSystem=0;
  gp_real_t min=9., tmp; /* assume all viewports have area<9 */
  if (fxe->xe.w != fxe->xe.win) { /* adjust for animation mode margins */
    x -= fxe->xe.a_x;
    y -= fxe->xe.a_y;
  }
  xn = ((gp_real_t)x - map->x.offset)/map->x.scale;
  yn = ((gp_real_t)y - map->y.offset)/map->y.scale;
  for (i=nSystems ; i>0 ; i--) {
    sys= (ge_system_t *)sys->el.prev;
    if (!sys->elements ||
        /* be sure system has been scanned if limits extreme */
        ((sys->rescan || sys->unscanned>=0) &&
         gd_scan(sys))) continue;
    box= &sys->trans.viewport;
    if (xn>=box->xmin && xn<=box->xmax && yn>=box->ymin && yn<=box->ymax)
      {
        tmp= (box->xmax-box->xmin)*(box->ymax-box->ymin);
        if(tmp<0) tmp= -tmp;
        if(tmp<min) {
          min= tmp;
          iSystem= i;
          thesys= sys;
        }
      }
  }
  if (!iSystem) { /* look for nearest axis */
    min= 9.; /* now assume mouse is closer to an axis than 9 units */
    for (i=nSystems ; i>0 ; i--) {
      sys= (ge_system_t *)sys->el.prev;
      if (!sys->elements) continue;
      box= &sys->trans.viewport;
      if (yn>=box->ymin && yn<=box->ymax) {
        tmp= xn-box->xmax;
        if(tmp<min && tmp>0) { min= tmp; iSystem= i; thesys= sys; }
        tmp= box->xmin-xn;
        if(tmp<min && tmp>0) { min= tmp; iSystem= i; thesys= sys; }
      }
      if (xn>=box->xmin && xn<=box->xmax) {
        tmp= yn-box->ymax;
        if(tmp<min && tmp>0) { min= tmp; iSystem= i; thesys= sys; }
        tmp= box->ymin-yn;
        if(tmp<min && tmp>0) { min= tmp; iSystem= i; thesys= sys; }
      }
    }
  }
  if (iSystem) {
    sys= thesys;
    *system= sys;
    FindCoordinates(sys, xn, yn, xr, yr);
  } else {
    *system= 0;
    *xr= xn;  *yr= yn;
  }
  return iSystem;
}

static void Find1System(gp_fx_engine_t *fxe, gd_drawing_t *drawing, int iSystem,
                        int x, int y, ge_system_t **system,
                        gp_real_t *xr, gp_real_t *yr)
{
  gp_xymap_t *map= &fxe->xe.e.map; /* NDC->VDC (x,y) mapping */
  gp_real_t xn, yn;
  ge_system_t *sys= GetSystemN(drawing, iSystem);
  if (fxe->xe.w != fxe->xe.win) { /* adjust for animation mode margins */
    x -= fxe->xe.a_x;
    y -= fxe->xe.a_y;
  }
  xn = ((gp_real_t)x - map->x.offset)/map->x.scale;
  yn = ((gp_real_t)y - map->y.offset)/map->y.scale;
  if (sys && (!(sys->rescan || sys->unscanned>=0) ||
              !gd_scan(sys))) {
    FindCoordinates(sys, xn, yn, xr, yr);
    *system= sys;
  } else {
    *xr= xn;
    *yr= yn;
    *system= 0;
  }
}

static ge_system_t *GetSystemN(gd_drawing_t *drawing, int n)
{
  if (n<=0 || n>drawing->nSystems) return 0;
  else {
    ge_system_t *sys= drawing->systems;
    while (--n) sys= (ge_system_t *)sys->el.next;
    return (sys && sys->elements)? sys : 0;
  }
}

static int FindAxis(ge_system_t *system, gp_real_t x, gp_real_t y)
{
  if (system) {
    int xout= (x==system->trans.window.xmin || x==system->trans.window.xmax);
    int yout= (y==system->trans.window.ymin || y==system->trans.window.ymax);
    if (xout) return yout? 3 : 2;
    else return yout? 1 : 3;
  }
  return 0;
}

static void FindCoordinates(ge_system_t *system, gp_real_t xNDC, gp_real_t yNDC,
                            gp_real_t *xWC, gp_real_t *yWC)
{
  gp_real_t x, y;
  gp_xymap_t map;
  gp_set_map(&system->trans.viewport, &system->trans.window, &map);
  x= map.x.scale*xNDC + map.x.offset;
  y= map.y.scale*yNDC + map.y.offset;
  if (system->trans.window.xmin<system->trans.window.xmax) {
    if (x<system->trans.window.xmin) x= system->trans.window.xmin;
    else if (x>system->trans.window.xmax) x= system->trans.window.xmax;
  } else {
    if (x<system->trans.window.xmax) x= system->trans.window.xmax;
    else if (x>system->trans.window.xmin) x= system->trans.window.xmin;
  }
  if (system->trans.window.ymin<system->trans.window.ymax) {
    if (y<system->trans.window.ymin) y= system->trans.window.ymin;
    else if (y>system->trans.window.ymax) y= system->trans.window.ymax;
  } else {
    if (y<system->trans.window.ymax) y= system->trans.window.ymax;
    else if (y>system->trans.window.ymin) y= system->trans.window.ymin;
  }
  *xWC= x;
  *yWC= y;
}

static gp_real_t GetFormat(char *format, gp_real_t w,
                        gp_real_t wmin, gp_real_t wmax, int isLog)
{
  gp_real_t delta;

  if (isLog) {
    w= exp10(w);
    wmin= exp10(wmin);
    wmax= exp10(wmax);
  }
  delta= wmax-wmin;
  if (delta<0.0) delta= -delta;
  if (wmin<0.0) wmin= -wmin;
  if (wmax<0.0) wmax= -wmax;
  if (wmax<wmin) {
    wmax= wmin;
    wmin-= delta;
  }
  if (!isLog) wmin= wmax; /* only for deciding between e and f format */

  if (wmax>=10000.0 || wmin<0.001) {
    /* use e format */
    int n;
    if (delta>0.0) n= 3+(int)log10(wmax/delta + 0.50001);
    else n= 3;
    sprintf(format, "%%%d.%de", 8+n, n);
  } else {
    /* use f format */
    int pre, post;
    if (wmax>1000.0) pre= 4;
    else if (wmax>100.0) pre= 3;
    else if (wmax>10.0) pre= 2;
    else pre= 1;
    if (delta<0.1 && delta>0.0) post= 3-(int)log10(delta);
    else if (isLog && wmin<0.01) post= 5;
    else post= 4;
    sprintf(format, "%%%d.%df", pre+post+3, post);
  }

  return w;
}

/* ------------------------------------------------------------------------ */

static void
DrawRubber(gp_fx_engine_t *fxe, int x, int y)
{
  pl_win_t *w = fxe->xe.win;
  int iPass= 2;

  if (!w) return;

  pl_color(w, PL_XOR);
  pl_pen(w, 1, PL_SOLID);

  /* first undraw previous box or line, then draw new box or line */
  while (iPass--) {
    if (anchorX!=oldX || anchorY!=oldY) {
      if (rubberBanding==1) {
        /* this is a rubber box */
        pl_rect(w, anchorX, anchorY, oldX, oldY, 1);
      } else {
        /* this is a rubber line */
        int x[2], y[2];
        x[0] = anchorX;  x[1] = oldX;
        y[0] = anchorY;  y[1] = oldY;
        pl_i_pnts(w, x, y, 2);
        pl_lines(w);
      }
    }
    oldX = x;
    oldY = y;
  }
}

/* ------------------------------------------------------------------------ */

int gx_point_click(gp_engine_t *engine, int style, int system,
                 int (*CallBack)(gp_engine_t *engine, int system,
                                 int release, gp_real_t x, gp_real_t y,
                                 int butmod, gp_real_t xn, gp_real_t yn))
{
  gp_x_engine_t *xeng= gp_x_engine(engine);
  if (!xeng || !xeng->w) return 1;

  /* set up state variables for point-and-click sequence */
  if (!(PtClCallBack= CallBack)) return 1;
  if (style==1 || style==2) ptClStyle= style;
  else ptClStyle= 0;
  if (system<0) ptClSystem= -1;
  else ptClSystem= system;
  ptClCount= 2;

  return 0;
}

/* ------------------------------------------------------------------------ */
