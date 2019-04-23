/*
 * $Id: xbasic.c,v 1.4 2010-01-10 05:02:23 dhmunro Exp $
 * Implement the basic X windows engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist/xbasic.h"
#include "gist/text.h"
#include "play/std.h"

#include <string.h>

static void g_on_expose(void *c, int *xy);
static void g_on_destroy(void *c);
static void g_on_resize(void *c,int w,int h);
static void g_on_focus(void *c,int in);
static void g_on_key(void *c,int k,int md);
static void g_on_click(void *c,int b,int md,int x,int y, unsigned long ms);
static void g_on_motion(void *c,int md,int x,int y);
static void g_on_deselect(void *c);
static void g_on_panic(pl_scr_t *screen);

static gp_callbacks_t g_x_on = {
  "gist gp_x_engine_t", g_on_expose, g_on_destroy, g_on_resize, g_on_focus,
  g_on_key, g_on_click, g_on_motion, g_on_deselect };

static int ChangePalette(gp_engine_t *engine);
static gp_real_t TextWidth(const char *text, int nc, const gp_text_attribs_t *t);
static void Kill(gp_engine_t *engine);
static void ShutDown(gp_x_engine_t *xEngine);
static int Clear(gp_engine_t *engine, int always);
static int Flush(gp_engine_t *engine);
static void GetXRectangle(gp_xymap_t *map, gp_box_t *box,
                          int *x0, int *y0, int *x1, int *y1);
static void ClearArea(gp_engine_t *engine, gp_box_t *box);
static void SetXTransform(gp_transform_t *trans, int landscape, int dpi);
static void gx_translate(gp_box_t *trans_window, int x, int y);
static gp_box_t *DamageClip(gp_box_t *damage);
static void ChangeMap(gp_engine_t *engine);
static void chk_clipping(gp_x_engine_t *xeng);
static int SetupLine(gp_x_engine_t *xeng, gp_line_attribs_t *gistAl, int join);
static int DrawLines(gp_engine_t *engine, long n, const gp_real_t *px,
                     const gp_real_t *py, int closed, int smooth);
static int DrawMarkers(gp_engine_t *engine, long n, const gp_real_t *px,
                       const gp_real_t *py);
static int DrwText(gp_engine_t *engine, gp_real_t x0, gp_real_t y0, const char *text);
static int DrawFill(gp_engine_t *engine, long n, const gp_real_t *px,
                    const gp_real_t *py);
static int GetCells(gp_map_t *map, gp_real_t xmin, gp_real_t xmax,
                    gp_real_t px, gp_real_t qx, long width,
                    int *i0, int *di, int *ncols, int *x0, int *x1);
static int DrawCells(gp_engine_t *engine, gp_real_t px, gp_real_t py, gp_real_t qx,
                     gp_real_t qy, long width, long height, long nColumns,
                     const gp_color_t *colors);
static int DrawDisjoint(gp_engine_t *engine, long n, const gp_real_t *px,
                        const gp_real_t *py, const gp_real_t *qx, const gp_real_t *qy);
static void GetVisibleNDC(gp_x_engine_t *xeng,
                          gp_real_t *xn, gp_real_t *xx, gp_real_t *yn, gp_real_t *yx);

static int justify_text(gp_xymap_t *map, gp_real_t x0, gp_real_t y0,
                        const char *text, int *ix, int *iy,
                        int xbox[], int ybox[]);
static int justify_next(const char **text, int *ix, int *iy);

static int gxErrorFlag= 0;
static void GxErrorHandler(void);

/* gp_engine_t which currently has mouse focus (set to NULL when the
   "current" engine get destroyed or when mouse leaves the "current"
   engine window, set to engine address on mouse motion). */
gp_engine_t *gx_current_engine = NULL;

/* ------------------------------------------------------------------------ */

static int
ChangePalette(gp_engine_t *engine)
{
  gp_x_engine_t *xeng= (gp_x_engine_t *)engine;
  pl_scr_t *s = xeng->s;
  pl_win_t *win = xeng->win;
  pl_col_t *palette= engine->palette;  /* requested palette */
  int nColors= engine->nColors;
  int width, height, depth;

  if (!s) return 0;
  depth = pl_sshape(s, &width, &height);
  if (depth > 8) depth = 8;
  if (nColors > 256) nColors= 256;

  pl_palette(win, palette, nColors);

  xeng->e.colorChange= 0;

  return (1<<depth);
}

/* ------------------------------------------------------------------------ */

static int nChars, lineHeight, textHeight, prevWidth, maxWidth, alignH;
static int textAscent;
static int firstTextLine= 0;

static pl_scr_t *current_scr;
static pl_win_t *current_win;
static int current_state, current_fsym, current_fsize;
static int chunkWidth, nChunk, supersub, dy_super, dy_sub, x_chunks;

/* ARGSUSED */
static gp_real_t
TextWidth(const char *text, int nc, const gp_text_attribs_t *t)
{
  int width= 0;
  if (firstTextLine) nChars= nc;
  if (!gp_do_escapes) {
    width = pl_txwidth(current_scr, text, nc, ga_attributes.t.font, current_fsize);
    if (firstTextLine) {
      /* record width of first line */
      prevWidth= chunkWidth= width;
      nChunk= nc;
      firstTextLine= 0;
    }
  } else if (nc>0) {
    int firstChunk= firstTextLine;
    const char *txt= text;
    char c;
    for ( ; nc-- ; text++) {
      c= text[0];
      if ((nc && c=='!') || c=='^' || c=='_') {
        if (txt<text)
          width += pl_txwidth(current_scr, txt, (int)(text-txt),
                             ga_attributes.t.font, current_fsize);
        if (firstChunk) {
          nChunk= (int)(text-txt);
          chunkWidth= width;
          firstChunk= 0;
        }
        txt= text+1;
        if (c=='!') {
          /* process single character escapes immediately */
          text= txt;
          nc--;
          c= txt[0];
          if (c!='!' && c!='^' && c!='_') {
            if (c==']') c= '^';
            width += pl_txwidth(current_scr, &c, 1,
                               current_fsym, current_fsize);
            txt++;
          }
        } else if (c=='^') {
          supersub|= 1;
        } else {
          supersub|= 2;
        }
      }
    }
    if (txt<text)
      width += pl_txwidth(current_scr, txt, (int)(text-txt),
                         ga_attributes.t.font, current_fsize);
    if (firstChunk) {
      nChunk= (int)(text-txt);
      chunkWidth= width;
    }
    if (firstTextLine) {
      /* record width of first line */
      prevWidth= width;  /* this is whole line */
      firstTextLine= 0;
    }
  }
  return (gp_real_t)width;
}

int
justify_text(gp_xymap_t *map, gp_real_t x0, gp_real_t y0, const char *text,
              int *ix, int *iy, int xbox[], int ybox[])
{
  int nLines, alignV, dx, dy, xmin, xmax, ymin, ymax, ix0, iy0;
  gp_real_t rwidest;

  /* abort if string screen position is ridiculous */
  x0 = map->x.scale*x0 + map->x.offset;
  y0 = map->y.scale*y0 + map->y.offset;
  if (x0<-16000. || x0>16000. || y0<-16000. || y0>16000.) return -1;
  current_state = 0;

  /* call pl_font before any calls to TextWidth
   * - may improve efficiency of pl_txwidth, pl_txheight */
  pl_font(current_win, ga_attributes.t.font, current_fsize, ga_attributes.t.orient);

  /* Set nLines, maxWidth, nChars, prevWidth */
  firstTextLine = 1;
  nChars = prevWidth = chunkWidth = nChunk = supersub = 0;
  nLines = gp_get_text_shape(text, &ga_attributes.t, &TextWidth, &rwidest);
  maxWidth = (int)rwidest;

  /* state bits:
     1 - this chunk is superscript
     2 - this chunk is subscript (1 and 2 together meaningless)
     4 - this chunk is single symbol char
   */
  x_chunks= 0;

  /* Compute height of one line */
  textHeight = pl_txheight(current_scr, ga_attributes.t.font, current_fsize,
                          &textAscent);
  dy_super = (supersub&1)? textAscent/3 : 0;
  dy_sub = (supersub&2)? textAscent/3 : 0;
  lineHeight = textHeight+dy_sub+dy_super;

  /* Compute displacement and bounding box of entire string,
     relative to specified location */
  gp_get_text_alignment(&ga_attributes.t, &alignH, &alignV);
  /* Note: xmax= xmin+maxWidth  */
  if (alignH==GP_HORIZ_ALIGN_LEFT) {
    dx= xmin= 0;
    xmax= maxWidth;
  } else if (alignH==GP_HORIZ_ALIGN_CENTER) {
    xmax= maxWidth/2;
    xmin= -xmax;
    dx= -prevWidth/2;
  } else {
    xmax= 0;
    xmin= -maxWidth;
    dx= -prevWidth;
  }

  /* Note: ymax= ymin+nLines*lineHeight  and  ymin= dy-textAscent  */
  if (alignV<=GP_VERT_ALIGN_CAP) {
    dy= textAscent + dy_super;
    ymin= 0;
    ymax= nLines*lineHeight;
  } else if (alignV==GP_VERT_ALIGN_HALF) {
    ymin= textAscent/2;
    ymax= nLines*lineHeight;
    dy= ymin-(ymax-lineHeight)/2;
    ymin= dy-textAscent-dy_super;
    ymax+= ymin;
  } else if (alignV==GP_VERT_ALIGN_BASE) {
    dy= -(nLines-1)*lineHeight;
    ymin= dy-textAscent-dy_super;
    ymax= textHeight-textAscent + dy_sub;
  } else {
    ymin= dy_sub-nLines*lineHeight;
    dy= ymin+textAscent+dy_super;
    ymax= dy_sub;
  }

  /* handle orientation (path) of text */
  if (ga_attributes.t.orient==GP_ORIENT_LEFT) {         /* upside down */
    int tmp;
    dx= -dx;  dy= -dy;
    tmp= xmin;  xmin= -xmax;  xmax= -tmp;
    tmp= ymin;  ymin= -ymax;  ymax= -tmp;
  } else if (ga_attributes.t.orient==GP_ORIENT_UP) {    /* reading upwards */
    int tmp;
    tmp= dx;  dx= dy;  dy= -tmp;
    tmp= xmin;  xmin= ymin;  ymin= -xmax;
                xmax= ymax;  ymax= -tmp;
  } else if (ga_attributes.t.orient==GP_ORIENT_DOWN) {  /* reading downwards */
    int tmp;
    tmp= dx;  dx= -dy;  dy= tmp;
    tmp= xmin;  xmin= -ymax;  ymax= xmax;
                xmax= -ymin;  ymin= tmp;
  }

  /* get bounding box and adjusted reference point */
  ix0 = (int)x0;
  iy0 = (int)y0;

  xbox[0] = ix0+xmin;   ybox[0] = iy0+ymin;
  xbox[1] = ix0+xmax;   ybox[1] = iy0+ymax;

  *ix= dx + ix0;
  *iy= dy + iy0;

  if (nChunk) pl_font(current_win,
                     ga_attributes.t.font, current_fsize, ga_attributes.t.orient);
  return nChunk;
}

int
justify_next(const char **text, int *ix, int *iy)
{
  const char *txt= *text+nChunk;
  int xadj= 0, yadj= 0;
  char c;

  nChars-= nChunk;
  if (!nChars) {
    /* last chunk was the last one on a line */
    txt= gp_next_text_line(txt, &nChars, 0);
    if (!txt) return -1;

    *text= txt;

    /* scan for end of first chunk */
    if (gp_do_escapes) {
      for (nChunk=0 ; nChunk<nChars ; nChunk++) {
        c= txt[nChunk];
        if ((nChunk+1<nChars && c=='!') || c=='^' || c=='_') break;
      }
    } else {
      nChunk= nChars;
    }

    /* compute width of this chunk if necessary, compute width
       of whole line if necessary for justification */
    if (alignH!=GP_HORIZ_ALIGN_LEFT || ga_attributes.t.orient!=GP_ORIENT_RIGHT) {
      /* need to compute width of entire line */
      int width= prevWidth;
      firstTextLine= 1;
      prevWidth= (int)TextWidth(txt, nChars, &ga_attributes.t);
      if (alignH==GP_HORIZ_ALIGN_CENTER) xadj= (width-prevWidth)/2;
      else if (alignH==GP_HORIZ_ALIGN_RIGHT) xadj= width-prevWidth;
      /* TextWidth sets chunkWidth */
    } else if (nChunk<nChars) {
      /* just need width of this chunk */
      if (nChunk) chunkWidth = pl_txwidth(current_scr, txt, nChunk,
                                         ga_attributes.t.font, current_fsize);
      else chunkWidth= 0;  /* unused */
    }

    /* reset state, adjusting (possibly rotated) x and y as well */
    xadj-= x_chunks;
    yadj= lineHeight;
    if (current_state&1) yadj+= dy_super;
    else if (current_state&2) yadj-= dy_sub;
    if (nChunk && (current_state&4))
      pl_font(current_win, ga_attributes.t.font, current_fsize, ga_attributes.t.orient);
    current_state= 0;

  } else {
    /* previous chunk ended with an escape character, or was single
       escaped symbol character -- can't get here unles gp_do_escapes */
    char c1= '\0';
    xadj= chunkWidth;      /* width of previous chunk */
    x_chunks+= chunkWidth; /* accumulate all chunks except last */
    yadj= 0;
    if (!(current_state&4)) {
      c1= *txt++;
      nChars--;
    }
    *text= txt;

    if (c1=='!') {
      /* this chunk begins with escaped character */
      nChunk= 1;
      c= txt[0];
      if (c=='!' || c=='^' || c=='_') {
        /* chunk is just ordinary text */
        for ( ; nChunk<nChars ; nChunk++) {
          c= txt[nChunk];
          if ((nChunk+1<nChars && c=='!') || c=='^' || c=='_') break;
        }
        pl_font(current_win, ga_attributes.t.font, current_fsize, ga_attributes.t.orient);
        current_state&= 3;
      } else {
        /* chunk is single symbol char */
        pl_font(current_win, current_fsym, current_fsize, ga_attributes.t.orient);
        current_state|= 4;
      }

    } else {
      for (nChunk=0 ; nChunk<nChars ; nChunk++) {
        c= txt[nChunk];
        if ((nChunk+1<nChars && c=='!') || c=='^' || c=='_') break;
      }
      if (nChunk)
        pl_font(current_win, ga_attributes.t.font, current_fsize, ga_attributes.t.orient);
      if (c1=='^') {
        if (current_state&1) {
          yadj+= dy_super;  /* return from super to normal */
          current_state= 0;
        } else {
          if (current_state&2) yadj-= dy_sub;
          yadj-= dy_super;  /* move to superscript */
          current_state= 1;
        }
      } else if (c1=='_') {
        if (current_state&2) {
          yadj-= dy_sub;  /* return from sub to normal */
          current_state= 0;
        } else {
          if (current_state&1) yadj+= dy_super;
          yadj+= dy_sub;  /* move to subscript */
          current_state= 2;
        }
      } else {
        /* just finished a symbol char */
        current_state&= 3;
      }
    }

    if (nChunk &&
        (nChunk<nChars || alignH!=GP_HORIZ_ALIGN_LEFT || ga_attributes.t.orient!=GP_ORIENT_RIGHT)) {
      char caret= '^';
      if (nChunk==1 && (current_state&4) && txt[0]==']') txt= &caret;
      chunkWidth = pl_txwidth(current_scr, txt, nChunk,
                             (current_state&4)? current_fsym : ga_attributes.t.font,
                             current_fsize);
    } else {
      chunkWidth= 0;  /* unused */
    }
  }

  if (ga_attributes.t.orient==GP_ORIENT_RIGHT) {
    *iy+= yadj;
    *ix+= xadj;
  } else if (ga_attributes.t.orient==GP_ORIENT_LEFT) {
    *iy-= yadj;
    *ix-= xadj;
  } else if (ga_attributes.t.orient==GP_ORIENT_UP) {
    *ix+= yadj;
    *iy-= xadj;
  } else {
    *ix-= yadj;
    *iy+= xadj;
  }

  return nChunk;
}

/* ------------------------------------------------------------------------ */

/* notes for Kill() and g_on_destroy
 * (1) Kill() is program-driven (e.g.- winkill)
 *     g_on_destroy() is event-driven (e.g.- mouse click)
 * (2) however, the pl_destroy() function MIGHT or MIGHT NOT
 *     result in a call to g_on_destroy()
 *     under Windows, g_on_destroy() is naturally called
 *       by the call pl_destroy() must make to close the window
 *     under UNIX/X, the play event handler calls pl_destroy()
 *       after g_on_destroy() returns
 * (3) therefore g_on_destroy() must not call pl_destroy(), since
 *       this would cause infinite recursion on Windows, and a
 *       double call on X.
 * (4) worse, if this is the final window on an X display, gist
 *       should disconnect from the display, but that can only
 *       be done after the final pl_destroy(), which is after
 *       the g_on_destroy() in the event-driven case
 *       -- thus, the pl_disconnect must take place on the next
 *          event, which is during the gh_before_wait hlevel.c function
 */
/* hack to disconnect if last engine destroyed (see gh_before_wait) */
static void g_do_disconnect(void);
extern void (*g_pending_task)(void);
void (*g_pending_task)(void) = 0;

static void
Kill(gp_engine_t *engine)
{
  gp_x_engine_t *xeng= (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->win;
  ShutDown(xeng);
  if (w) pl_destroy(w);
  /* for program-driven Kill(), can take care of pl_disconnect immediately */
  g_do_disconnect();
}

static int
Clear(gp_engine_t *engine, int always)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  if (!xeng->w) return 1;
  if ((always || xeng->e.marked) && xeng->w==xeng->win) {
    int tm = xeng->topMargin;
    int lm = xeng->leftMargin;
    if (tm || lm) {
      int xmax = (int)xeng->swapped.window.xmax;
      int ymax = (int)xeng->swapped.window.ymin;
      if (xmax > lm+xeng->wtop) xmax = lm+xeng->wtop;
      if (ymax > tm+xeng->htop) ymax = tm+xeng->htop;
      if (xeng->clipping) {
        pl_clip(xeng->w, 0,0,0,0);
        xeng->clipping = 0;
      }
      pl_color(xeng->w, PL_BG);
      pl_rect(xeng->w, lm, tm, xmax, ymax, 0);
    } else {
      pl_clear(xeng->w);
    }
  }
  if (xeng->e.colorChange) ChangePalette(engine);
  xeng->e.marked = 0;
  return 0;
}

static int
Flush(gp_engine_t *engine)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  if (!xeng->w) return 1;
  pl_flush(xeng->w);
  /* test whether an X error has been reported */
  if (gxErrorFlag) GxErrorHandler();
  return 0;
}

static void
GetXRectangle(gp_xymap_t *map, gp_box_t *box,
              int *x0, int *y0, int *x1, int *y1)
{
  /* get corners of clip rectangle in pixel coordinates */
  int wmin= (int)(map->x.scale*box->xmin+map->x.offset);
  int wmax= (int)(map->x.scale*box->xmax+map->x.offset);
  if (wmax>=wmin) {
    *x0 = wmin;
    *x1 = wmax+1;
  } else {
    *x0 = wmax;
    *x1 = wmin+1;
  }
  wmin= (int)(map->y.scale*box->ymin+map->y.offset);
  wmax= (int)(map->y.scale*box->ymax+map->y.offset);
  if (wmax>=wmin) {
    *y0 = wmin;
    *y1 = wmax+1;
  } else {
    *y0 = wmax;
    *y1 = wmin+1;
  }
}

static void
ClearArea(gp_engine_t *engine, gp_box_t *box)
{
  gp_x_engine_t *xeng= (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  int x0, y0, x1, y1;
  if (!w) return;
  /* if this is animation mode, do not try to clear window */
  if (w==xeng->win) {
    int lm = xeng->leftMargin;
    int tm = xeng->topMargin;
    GetXRectangle(&engine->devMap, box, &x0, &y0, &x1, &y1);
    if (x0 < lm) x0 = lm;
    if (x1 > lm+xeng->wtop) x1 = lm+xeng->wtop;
    if (y0 < tm) y0 = tm;
    if (y1 > tm+xeng->htop) y1 = tm+xeng->htop;
    pl_color(w, PL_BG);
    pl_rect(w, x0, y0, x1, y1, 0);
  }
}

static void
SetXTransform(gp_transform_t *trans, int landscape, int dpi)
{
  trans->viewport= landscape? gp_landscape_box : gp_portrait_box;
  trans->window.xmin= 0.0;
  trans->window.xmax= GX_PIXELS_PER_NDC(dpi)*trans->viewport.xmax;
  trans->window.ymin= GX_PIXELS_PER_NDC(dpi)*trans->viewport.ymax;
  trans->window.ymax= 0.0;
}

static void
gx_translate(gp_box_t *trans_window, int x, int y)
{
  trans_window->xmax += x - trans_window->xmin;
  trans_window->xmin = x;
  trans_window->ymin += y - trans_window->ymax;
  trans_window->ymax = y;
}

static gp_box_t cPort;

static gp_box_t *
DamageClip(gp_box_t *damage)
{
  cPort= gp_transform.viewport;
  if (cPort.xmin>cPort.xmax)
    { gp_real_t tmp= cPort.xmin; cPort.xmin= cPort.xmax; cPort.xmax= tmp; }
  if (cPort.ymin>cPort.ymax)
    { gp_real_t tmp= cPort.ymin; cPort.ymin= cPort.ymax; cPort.ymax= tmp; }
  /* (assume damage box is properly ordered) */
  if (damage->xmin>cPort.xmin) cPort.xmin= damage->xmin;
  if (damage->xmax<cPort.xmax) cPort.xmax= damage->xmax;
  if (damage->ymin>cPort.ymin) cPort.ymin= damage->ymin;
  if (damage->ymax<cPort.ymax) cPort.ymax= damage->ymax;
  if (cPort.xmin>cPort.xmax || cPort.ymin>cPort.ymax) return 0;
  else return &cPort;
}

static void
ChangeMap(gp_engine_t *engine)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  int landscape = xeng->width > xeng->height;
  int x0, y0, x1, y1;
  gp_box_t *clipport;
  if (!w) return;

  /* check to be sure that landscape/portrait mode hasn't changed */
  if (landscape!=xeng->e.landscape) {
    /* this is probably insane if in animation mode... */
    SetXTransform(&xeng->e.transform, xeng->e.landscape, xeng->dpi);
    xeng->width = (int)xeng->e.transform.window.xmax;
    xeng->height = (int)xeng->e.transform.window.ymin;
    xeng->swapped = xeng->e.transform;
    /* make adjustments to allow for SetXTransform, then recenter */
    if (xeng->w != xeng->win) {
      xeng->a_x += xeng->x+1;
      xeng->a_y += xeng->y+1;
    }
    xeng->x = xeng->y = -1;
    gx_recenter(xeng, xeng->wtop+xeng->leftMargin, xeng->htop+xeng->topMargin);
  }

  /* do generic change map */
  gp_compose_map(engine);

  /* get current clip window */
  if (xeng->e.damaged) clipport = DamageClip(&xeng->e.damage);
  else clipport = &gp_transform.viewport;
  if (clipport) {
    /* set clipping rectangle for this gp_x_engine_t */
    GetXRectangle(&engine->devMap, clipport, &x0, &y0, &x1, &y1);
    if (xeng->w == xeng->win) {
      /* additional restriction for vignetting by window borders */
      int lm = xeng->leftMargin;
      int tm = xeng->topMargin;
      if (x0 < lm) x0 = lm;
      if (x1 > lm+xeng->wtop) x1 = lm+xeng->wtop;
      if (y0 < tm) y0 = tm;
      if (y1 > tm+xeng->htop) y1 = tm+xeng->htop;
      xeng->clipping = 1;
    } else {
      if (x0 < 0) x0 = 0;
      if (x1 > xeng->a_width) x1 = xeng->a_width;
      if (y0 < 0) y0 = 0;
      if (y1 > xeng->a_height) y1 = xeng->a_height;
      if (x0==0 && x1==xeng->a_width && y0==0 && y1==xeng->a_height)
        x0 = x1 = y0 = y1 = 0;
    }
    if (x0 || x1 || y0 || y1) {
      if (x1<=x0) x1 = x0+1;
      if (y1<=y0) y1 = y0+1;
    }
    pl_clip(xeng->w, x0, y0, x1, y1);
  }
}

static void
chk_clipping(gp_x_engine_t *xeng)
{
  pl_win_t *w = xeng->win;
  if (!xeng->clipping) {
    int x0, y0, x1, y1;
    int lm = xeng->leftMargin;
    int tm = xeng->topMargin;
    if (xeng->e.damaged) {
      gp_box_t *box = DamageClip(&xeng->e.damage);
      gp_xymap_t map;
      if (xeng->w != w)
        gp_set_map(&xeng->swapped.viewport, &xeng->swapped.window, &map);
      else
        map = xeng->e.devMap;
      GetXRectangle(&map, box, &x0, &y0, &x1, &y1);
      /* additional restriction for vignetting by window borders */
      if (x0 < lm) x0 = lm;
      if (x1 > lm+xeng->wtop) x1 = lm+xeng->wtop;
      if (y0 < tm) y0 = tm;
      if (y1 > tm+xeng->htop) y1 = tm+xeng->htop;
    } else {
      x0 = lm;
      x1 = lm+xeng->wtop;
      y0 = tm;
      y1 = tm+xeng->htop;
    }
    xeng->clipping = 1;
    if (x1<=x0) x1 = x0+1;
    if (y1<=y0) y1 = y0+1;
    pl_clip(w, x0, y0, x1, y1);
  }
}

/* ------------------------------------------------------------------------ */

static int
SetupLine(gp_x_engine_t *xeng, gp_line_attribs_t *gistAl, int join)
{
  gp_xymap_t *map= &xeng->e.map;
  double xt[2], yt[2];
  xt[0] = map->x.scale;
  xt[1] = map->x.offset;
  yt[0] = map->y.scale;
  yt[1] = map->y.offset;
  pl_d_map(xeng->w, xt, yt, 1);
  chk_clipping(xeng);
  if (gistAl->type != GP_LINE_NONE) {
    int type = gistAl->type-1;
    int width = (unsigned int)(GP_DEFAULT_LINE_INCHES*xeng->dpi*gistAl->width);
    if (join) type |= PL_SQUARE;
    pl_pen(xeng->w, width, type);
    pl_color(xeng->w, gistAl->color);
    return 0;
  } else {
    return 1;
  }
}

static int
DrawLines(gp_engine_t *engine, long n, const gp_real_t *px,
          const gp_real_t *py, int closed, int smooth)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  long i, imax;
  int npts;

  if (!w) return 1;
  if (n<=0 || SetupLine(xeng, &ga_attributes.l, 0)) return 0;

  closed = (closed && n>1 && (px[0]!=px[n-1] || py[0]!=py[n-1]));
  for (i=0 ; i<n ; i=imax) {
    imax = i+2047;
    npts = (imax<=n)? 2047 : (int)(n-i);
    pl_d_pnts(w, px+i, py+i, npts);
    if (closed && imax>=n)
      pl_d_pnts(w, px, py, -1);
    pl_lines(w);
  }

  xeng->e.marked = 1;
  return 0;
}

/* ------------------------------------------------------------------------ */

/* play has no polymarker primitive, use gp_draw_pseudo_mark-- unless
   user requested tiny points, in which case we use pl_dots */
static int
DrawMarkers(gp_engine_t *engine, long n, const gp_real_t *px, const gp_real_t *py)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  if (!w || !xeng->mapped) return 1;

  xeng->e.marked = 1;
  if (ga_attributes.m.type!=GP_MARKER_POINT || ga_attributes.m.size>1.5) {
    return gp_draw_pseudo_mark(engine, n, px, py);

  } else {
    long i, imax;
    int npts;
    gp_xymap_t *map= &xeng->e.map;
    double xt[2], yt[2];
    xt[0] = map->x.scale;
    xt[1] = map->x.offset;
    yt[0] = map->y.scale;
    yt[1] = map->y.offset;
    pl_d_map(w, xt, yt, 1);
    chk_clipping(xeng);
    pl_color(w, ga_attributes.m.color);

    for (i=0 ; i<n ; i=imax) {
      imax = i+2048;
      npts = (imax<=n)? 2048 : (int)(n-i);
      pl_d_pnts(w, px+i, py+i, npts);
      pl_dots(w);
    }

    return 0;
  }
}

/* ------------------------------------------------------------------------ */

static int
DrwText(gp_engine_t *engine, gp_real_t x0, gp_real_t y0, const char *text)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  gp_xymap_t *map = &xeng->e.map;
  int ix, iy, len, xbox[2], ybox[2], xn, xx, yn, yx;
  const char *txt;
  char *caret= "^";

  if (!w || !xeng->mapped) return 1;
  chk_clipping(xeng);

  current_fsize = (int)((xeng->dpi/GP_ONE_INCH)*ga_attributes.t.height);
  if (current_fsize<4) current_fsize = 4;     /* totally illegible */
  if (current_fsize>180) current_fsize = 180; /* way too big */
  current_fsym = GP_FONT_SYMBOL | (ga_attributes.t.font&3);
  current_scr = xeng->s;
  current_win = w;

  /* get current window */
  xn = (int)(gp_transform.window.xmin*map->x.scale + map->x.offset);
  xx = (int)(gp_transform.window.xmax*map->x.scale + map->x.offset);
  yn = (int)(gp_transform.window.ymin*map->y.scale + map->y.offset);
  yx = (int)(gp_transform.window.ymax*map->y.scale + map->y.offset);
  if (yn > yx) { int tmp=yn ; yn=yx; yx=tmp ; }

  /* handle multi-line strings */
  len = justify_text(map, x0, y0, text, &ix, &iy, xbox, ybox);
  if (len < 0) return 0;

  /* consider whether string is completely clipped */
  if (ybox[0]>yx || ybox[1]<yn || xbox[0]>xx || xbox[1]<xn) return 0;

  /* erase background if string is opaque */
  if (ga_attributes.t.opaque) {
    pl_color(w, PL_BG);
    pl_rect(w, xbox[0], ybox[0], xbox[1], ybox[1], 0);
  }
  pl_color(w, ga_attributes.t.color);

  do {
    if (len>0) {
      if (len==1 && (current_state&4) && text[0]==']') txt = caret;
      else txt = text;
      pl_text(w, ix, iy, txt, len);
    }
    len = justify_next(&text, &ix, &iy);
  } while (len>=0);

  xeng->e.marked = 1;
  return 0;
}

/* ------------------------------------------------------------------------ */

static int
DrawFill(gp_engine_t *engine, long n, const gp_real_t *px,
         const gp_real_t *py)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  long i, imax;
  int npts, has_edge;

  if (!w || !xeng->mapped) return 1;
  has_edge = !SetupLine(xeng, &ga_attributes.e, 0); /* but shouldn't set color */
  pl_color(w, ga_attributes.f.color);

  /* This gives incorrect results if more than one pass through the loop,
   * but there is no way to give the correct result, so may as well...  */
  for (i=0 ; i<n ; i=imax) {
    imax = i+2048;
    npts = (imax<=n)? 2048 : (int)(n-i);
    pl_d_pnts(w, px+i, py+i, npts);
    /* Can Nonconvex or Convex be detected? */
    pl_fill(w, 0);
  }

  xeng->e.marked = 1;
  if (has_edge) {
    pl_color(w, ga_attributes.e.color);
    for (i=0 ; i<n ; i=imax) {
      imax = i+2047;
      npts = (imax<=n)? 2047 : (int)(n-i);
      pl_d_pnts(w, px+i, py+i, npts);
      if (imax>=n)
        pl_d_pnts(w, px, py, -1);
      pl_lines(w);
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------ */

static int
GetCells(gp_map_t *map, gp_real_t xmin, gp_real_t xmax,
         gp_real_t px, gp_real_t qx, long width,
         int *i0, int *di, int *ncols, int *x0, int *x1)
{
  gp_real_t scale = map->scale;
  gp_real_t offset = map->offset;
  gp_real_t x, dx = (qx-px)/width;
  int imin, imax;

  if (xmin>xmax) x=xmin, xmin=xmax, xmax=x;

  if (dx>=0.0) {
    if (qx<xmin || px>xmax) return 0;
    if (dx > 0.0) {
      x = (xmin - px)/dx;
      if (x < 1.) imin = 0;
      else imin=(int)x, px+=imin*dx;
      x = (qx - xmax)/dx;
      if (x < 1.) imax = 0;
      else imax=(int)x, qx-=imax*dx;
      width -= imin + imax;
      if (width < 3) { /* see comment below */
        if (qx <= xmax) {
          if (imin) px = xmin;
        } else if (px >= xmin) {
          if (imax) qx = xmax;
        } else if (width < 2) {
          px = xmin;
          qx = xmax;
        } else if (qx-xmax <= xmin-px) {
          px += qx - xmax;
          qx = xmax;
        } else {
          qx += qx - xmin;
          px = xmin;
        }
      }
    } else {
      imin = width/2;
      imax = width - imin - 1;
      width = 1;
    }
  } else {
    if (px<xmin || qx>xmax) return 0;
    dx = -dx;
    x = (px - xmax)/dx;
    if (x < 1.) imin = 0;
    else imin=(int)x, px-=imin*dx;
    x = (xmin - qx)/dx;
    if (x < 1.) imax = 0;
    else imax=(int)x, qx+=imax*dx;
    width -= imin + imax;
    if (width < 3) { /* see comment below */
      if (px <= xmax) {
        if (imax) qx = xmin;
      } else if (qx >= xmin) {
        if (imin) px = xmax;
      } else if (width < 2) {
        qx = xmin;
        px = xmax;
      } else if (px-xmax <= xmin-qx) {
        qx += px - xmax;
        px = xmax;
      } else {
        px += qx - xmin;
        qx = xmin;
      }
    }
  }
  /* width<3 logic above guarantees these will not overflow as int */
  px = px*scale + offset;
  qx = qx*scale + offset;
  if (qx >= px) {
    *i0 = imin;
    *di = 1;
    *ncols = width;
    *x0 = (int)px;
    *x1 = (int)qx;
  } else {
    *i0 = imin + width - 1;
    *di = -1;
    *ncols = width;
    *x0 = (int)qx;
    *x1 = (int)px;
  }
  /* cell array always at least 1 pixel */
  if (*x1 == *x0) *x1 += 1;

  return 1;
}

static int
DrawCells(gp_engine_t *engine, gp_real_t px, gp_real_t py, gp_real_t qx,
          gp_real_t qy, long width, long height, long nColumns,
          const gp_color_t *colors)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  gp_xymap_t *map= &xeng->e.map;
  int i0, j0, di, dj, ncols, nrows, x0, y0, x1, y1;

  if (!w || !xeng->mapped) return 1;
  chk_clipping(xeng);

  if (GetCells(&map->x, gp_transform.window.xmin, gp_transform.window.xmax,
               px, qx, width, &i0, &di, &ncols, &x0, &x1) &&
      GetCells(&map->y, gp_transform.window.ymin, gp_transform.window.ymax,
               py, qy, height, &j0, &dj, &nrows, &y0, &y1)) {
    unsigned char *ndxs = (unsigned char *)colors;
    if (di<0 || dj<0 || ncols!=width || nrows!=height || nColumns!=width) {
      int r, c, nr, nc, i, j;
      ndxs = pl_malloc(ga_attributes.rgb?3*ncols*nrows:ncols*nrows);
      j0 *= nColumns;
      dj *= nColumns;
      if (ga_attributes.rgb) {
        for (j=j0,c=0,nr=nrows ; nr-- ; j+=dj,c+=ncols) {
          nc = ncols;
          for (i=i0,r=0 ; nc-- ; i+=di,r++) {
            ndxs[3*(c+r)] = colors[3*(j+i)];
            ndxs[3*(c+r)+1] = colors[3*(j+i)+1];
            ndxs[3*(c+r)+2] = colors[3*(j+i)+2];
          }
        }
      } else {
        for (j=j0,c=0,nr=nrows ; nr-- ; j+=dj,c+=ncols) {
          nc = ncols;
          for (i=i0,r=0 ; nc-- ; i+=di,r++)
            ndxs[c+r] = colors[j+i];
        }
      }
    }
    if (ncols && nrows) {
      if (ga_attributes.rgb)
        pl_rgb_cell(w, ndxs, ncols, nrows, x0, y0, x1, y1);
      else
        pl_ndx_cell(w, ndxs, ncols, nrows, x0, y0, x1, y1);
    }
    if (ndxs!=colors) pl_free(ndxs);
  }

  xeng->e.marked = 1;
  return 0;
}

/* ------------------------------------------------------------------------ */

static int
DrawDisjoint(gp_engine_t *engine, long n, const gp_real_t *px,
             const gp_real_t *py, const gp_real_t *qx, const gp_real_t *qy)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  pl_win_t *w = xeng->w;
  long i, imax;
  int nseg;

  if (!w || !xeng->mapped) return 1;
  if (SetupLine(xeng, &ga_attributes.l, 1)) return 0;

  pl_d_pnts(w, px, py, 0);
  for (i=0 ; i<n ;) {
    imax = i+1024;
    nseg = (imax<=n)? 1024 : (int)(n-i);
    while (nseg--) {
      pl_d_pnts(w, px+i, py+i, -1);
      pl_d_pnts(w, qx+i, qy+i, -1);
      i++;
    }
    pl_segments(w);
  }

  xeng->e.marked = 1;
  return 0;
}

/* ------------------------------------------------------------------------ */

static gp_engine_t *waiting_for = 0;
static void (*wait_callback)(void) = 0;

int
gp_expose_wait(gp_engine_t *eng, void (*e_callback)(void))
{
  if (waiting_for) {
    waiting_for = 0;
    wait_callback = 0;
    return 1;
  } else {
    gp_x_engine_t *xeng = gp_x_engine(eng);
    if (!xeng || !xeng->w) return 1;
    if (xeng->mapped) return 2;
    waiting_for = eng;
    wait_callback = e_callback;
    return 0;
  }
}

static void
g_on_expose(void *c, int *xy)
{
  gp_x_engine_t *xeng = c;
  if (xeng->e.on==&g_x_on) {
    if (c && c == waiting_for) {
      waiting_for = 0;
      if (wait_callback) wait_callback();
      wait_callback = 0;
    }
    if (!xeng->w) return;
    xeng->mapped = 1;
    if (xeng->HandleExpose)
      /* the alternate handler should probably call gx_expose */
      xeng->HandleExpose(&xeng->e, xeng->e.drawing, xy);
    else
      gx_expose(&xeng->e, xeng->e.drawing, xy);
  } else if (xeng->e.on && xeng->e.on->expose) {
    xeng->e.on->expose(c, xy);
  }
}

static void
g_on_click(void *c,int b,int md,int x,int y, unsigned long ms)
{
  gp_x_engine_t *xeng = c;
  if (xeng->e.on==&g_x_on) {
    if (!xeng->w) return;
    if (xeng->HandleClick)
      xeng->HandleClick(&xeng->e, b, md, x, y, ms);
  } else if (xeng->e.on && xeng->e.on->click) {
    xeng->e.on->click(c, b, md, x, y, ms);
  }
}

static void
g_on_motion(void *c,int md,int x,int y)
{
  gp_x_engine_t *xeng = c;
  gx_current_engine = (gp_engine_t *)c;
  if (xeng->e.on==&g_x_on) {
    if (!xeng->w) return;
    if (xeng->HandleMotion)
      xeng->HandleMotion(&xeng->e, md, x, y);
  } else if (xeng->e.on && xeng->e.on->motion) {
    xeng->e.on->motion(c, md, x, y);
  }
}

static void
g_on_destroy(void *c)
{
  gp_x_engine_t *xeng = c;
  if (xeng->e.on==&g_x_on) {
    /* if xeng->win==0, assume ShutDown already called */
    if (xeng->win) ShutDown(xeng);
  } else if (xeng->e.on && xeng->e.on->destroy) {
    xeng->e.on->destroy(c);
  }
}

static void
g_on_resize(void *c,int w,int h)
{
  gp_x_engine_t *xeng = c;
  if (xeng->e.on==&g_x_on) {
    if (xeng->w) gx_recenter(xeng, w, h);
  } else if (xeng->e.on && xeng->e.on->resize) {
    xeng->e.on->resize(c, w, h);
  }
}

static void
g_on_focus(void *c,int in)
{
  gp_x_engine_t *xeng = c;
  if (in == 2) gx_current_engine = NULL; /* current window has lost mouse focus */
  if (xeng->e.on==&g_x_on) {
    if (xeng->w && xeng->HandleMotion && in==2)
      xeng->HandleMotion(&xeng->e, 0, -1, -1);
  } else if (xeng->e.on && xeng->e.on->focus) {
    xeng->e.on->focus(c, in);
  }
}

static void
g_on_key(void *c,int k,int md)
{
  gp_x_engine_t *xeng = c;
  if (xeng->e.on==&g_x_on) {
    if (xeng->w && xeng->HandleKey)
      xeng->HandleKey(&xeng->e, k, md);
  } else if (xeng->e.on && xeng->e.on->key) {
    xeng->e.on->key(c, k , md);
  }
}

static void g_on_deselect(void *c)
{
  gp_x_engine_t *xeng = c;
  if (xeng->e.on==&g_x_on) {
  } else if (xeng->e.on && xeng->e.on->deselect) {
    xeng->e.on->deselect(c);
  }
}

void
gx_expose(gp_engine_t *engine, gd_drawing_t *drawing, int *xy)
{
  gp_x_engine_t *xeng = (gp_x_engine_t *)engine;
  gp_box_t damage;
  if (!drawing || !xeng->w) return;
  /* xy=0 to redraw all, otherwise x0,y0,x1,y1 */
  if (xy) {
    gp_xymap_t *map = &engine->devMap;
    damage.xmin= (xy[0]-map->x.offset)/map->x.scale;
    damage.xmax= (xy[2]-map->x.offset)/map->x.scale;
    damage.ymax= (xy[1]-map->y.offset)/map->y.scale;
    damage.ymin= (xy[3]-map->y.offset)/map->y.scale;
  } else {
    damage.xmin = xeng->swapped.viewport.xmin;
    damage.xmax = xeng->swapped.viewport.xmax;
    damage.ymin = xeng->swapped.viewport.ymin;
    damage.ymax = xeng->swapped.viewport.ymax;
  }
  if (engine->damaged) {
    gp_swallow(&engine->damage, &damage);
  } else {
    engine->damage = damage;
    engine->damaged = 1;
  }
  gd_set_drawing(drawing);
  gp_preempt(engine);
  gd_draw(1);
  gp_preempt(0);        /* not correct if damaged during a preempt... */
  gd_set_drawing(0);
}

void
gx_recenter(gp_x_engine_t *xeng, int width, int height)
{
  int x, y;
  int eWidth = xeng->width;
  int eHeight = xeng->height;
  width -= xeng->leftMargin;
  height -= xeng->topMargin;
  xeng->wtop = width;
  xeng->htop = height;
  x = (eWidth-width)/2;
  /* put center of page at center of landscape window */
  if (eWidth>eHeight) y = (eHeight-height)/2;
  /* put center of upper square of page at center of portrait window */
  else y = (eWidth-height)/2;
  /* once either dimension is big enough for whole picture, stop moving it */
  if (y<0) y = 0;
  if (x<0) x = 0;
  if (x!=xeng->x || y!=xeng->y) {
    int tmargin = xeng->topMargin;
    int lmargin = xeng->leftMargin;
    gx_translate(&xeng->swapped.window, -x+lmargin, -y+tmargin);
    if (xeng->w == xeng->win) {
      gx_translate(&xeng->e.transform.window, -x+lmargin, -y+tmargin);
      gp_set_device_map(&xeng->e);
    } else {
      xeng->a_x -= x - xeng->x;
      xeng->a_y -= y - xeng->y;
      lmargin = tmargin = 0;
    }
    xeng->x = x;
    xeng->y = y;
    if (xeng->wtop>0) x = xeng->wtop+lmargin;
    else x = lmargin+1;
    if (xeng->htop>0) y = xeng->htop+tmargin;
    else y = tmargin+1;
    xeng->clipping = 1;
    pl_clip(xeng->win, lmargin, tmargin, x, y);
  }
}

/* ------------------------------------------------------------------------ */

typedef struct g_scr g_scr;
struct g_scr {
  char *name;
  int number;
  pl_scr_t *s;
};
static g_scr *g_screens = 0;
static int n_screens = 0;

/* ARGSUSED */
void
gp_initializer(int *pargc, char *argv[])
{
  extern char *g_argv0;
  g_argv0 = argv? argv[0] : 0;
  pl_gui(&g_on_expose, &g_on_destroy, &g_on_resize, &g_on_focus,
        &g_on_key, &g_on_click, &g_on_motion, &g_on_deselect,
        &g_on_panic);
}

pl_scr_t *
gx_connect(char *displayName)
{
  pl_scr_t *s = 0;
  int i, j, i0=-1, len=0, number=0;

  /* split display into base name and screen number (separated by dot) */
  if (displayName) while (displayName[len]) len++;
  if (len) {
    for (i=len-1 ; i>=0 ; i--) if (displayName[i]=='.') break;
    if (i>=0) {
      int i0 = i;
      for (i++ ; i<len && displayName[i]<='9' && displayName[i]>='0' ; i++)
        number = 10*number + (displayName[i]-'0');
      if (i == len) len = i0;
      else number = 0;
    }
  }
  if (!len) displayName = 0;
  if (g_screens) {
    for (i=0 ; i<n_screens ; i++) {
      j = 0;
      if (g_screens[i].name) {
        for ( ; j<len ; j++)
          if (g_screens[i].s && g_screens[i].name[j]!=displayName[j]) break;
      }
      if (j==len && (len? (!g_screens[i].name[j]) : !g_screens[i].name)) {
        if (number == g_screens[i].number) break;
        else if (i0<0) i0 = i;
      }
    }
    if (i<n_screens) s = g_screens[i].s;
  }
  if (!s) {
    if (i0<0) s = pl_connect(displayName);
    else s = pl_multihead(g_screens[i0].s, number);
    if (!s) return s;
    for (i=0 ; i<n_screens ; i++) if (!g_screens[i].s) break;
    if (i==n_screens && !(i & (i-1))) {
      int n = i? 2*i : 1;
      g_screens = pl_realloc(g_screens, sizeof(g_scr)*n);
    }
    g_screens[i].number = number;
    g_screens[i].name = displayName? pl_strncat(0, displayName, len) : 0;
    g_screens[i].s = s;
    if (i==n_screens) n_screens++;
  }

  return s;
}

void
gx_disconnect(pl_scr_t *s)
{
  if (s) {
    int i;
    char *name;
    for (i=0 ; i<n_screens ; i++) {
      if (g_screens[i].s == s) {
        name = g_screens[i].name;
        g_screens[i].name = 0;
        g_screens[i].s = 0;
        pl_free(name);
      }
    }
    pl_disconnect(s);
  } else {
    g_do_disconnect();
  }
}

gp_x_engine_t *
gx_new_engine(pl_scr_t *s, char *name, gp_transform_t *toPixels,
         int x, int y, int topMargin, int leftMargin, long engineSize)
{
  gp_x_engine_t *xEngine;
  unsigned int width, height;
  gp_real_t pixels_per_page;
  int dpi;

  if (!s) return 0;

  /* Graphics window will have dimensions of toPixels transform window */
  if (toPixels->window.xmin<toPixels->window.xmax)
    width = (unsigned int)(toPixels->window.xmax - toPixels->window.xmin);
  else
    width = (unsigned int)(toPixels->window.xmin - toPixels->window.xmax);
  if (toPixels->window.ymin<toPixels->window.ymax)
    height = (unsigned int)(toPixels->window.ymax - toPixels->window.ymin);
  else
    height = (unsigned int)(toPixels->window.ymin - toPixels->window.ymax);

  /* Reconstruct dpi (dots per inch) from toPixels transform */
  pixels_per_page = toPixels->window.ymin;
  if (pixels_per_page < toPixels->window.xmax)
    pixels_per_page = toPixels->window.xmax;
  dpi = (int)(0.01 + pixels_per_page*GP_ONE_INCH/gp_portrait_box.ymax);

  /* adjust VDC window so gp_set_device_map in gp_new_engine sets proper
   * transform, which will have xmin=x<0, ymax=y<0 */
  gx_translate(&toPixels->window, x+leftMargin, y+topMargin);

  xEngine =
    (gp_x_engine_t *)gp_new_engine(engineSize, name, &g_x_on, toPixels, width>height,
                           &Kill, &Clear, &Flush, &ChangeMap,
                           &ChangePalette, &DrawLines, &DrawMarkers,
                           &DrwText, &DrawFill, &DrawCells,
                           &DrawDisjoint);
  if (!xEngine) {
    strcpy(gp_error, "memory manager failed in gx_new_engine");
    return 0;
  }

  /* XEngines can repair damage */
  xEngine->e.ClearArea = &ClearArea;

  /* Fill in gp_engine_t properties specific to gp_x_engine_t */
  xEngine->s = s;
  xEngine->win = 0;
  xEngine->width = width;
  xEngine->height = height;
  xEngine->topMargin = topMargin;
  xEngine->leftMargin = leftMargin;
  xEngine->x = -x;
  xEngine->y = -y;
  xEngine->mapped = xEngine->clipping = 0;
  xEngine->dpi = dpi;

  xEngine->e.colorMode = 0;

  xEngine->w = 0;
  xEngine->a_width = xEngine->a_height= 0;
  xEngine->a_x = xEngine->a_y= 0;
  xEngine->swapped = xEngine->e.transform;

  xEngine->HandleExpose = 0;
  xEngine->HandleClick = 0;
  xEngine->HandleMotion = 0;
  xEngine->HandleKey = 0;

  return xEngine;
}

/* default top window represents 6 inch square */
int gx_75dpi_width = 450;
int gx_100dpi_width = 600;
int gx75height = 450;
int gx100height = 600;

unsigned long gx_parent = 0;
int gx_xloc=0, gx_yloc=0;

int gp_private_map = 0;
int gp_input_hint = 0;
int gp_rgb_hint = 0;

gp_engine_t *
gp_new_bx_engine(char *name, int landscape, int dpi, char *displayName)
{
  pl_scr_t *s = gx_connect(displayName);
  int topWidth = GX_DEFAULT_TOP_WIDTH(dpi);
  int topHeight = GX_DEFAULT_TOP_HEIGHT(dpi);
  gp_transform_t toPixels;
  int x, y, width, height, hints;
  gp_x_engine_t *xeng;

  if (!s) return 0;

  SetXTransform(&toPixels, landscape, dpi);
  width = (int)toPixels.window.xmax;
  height = (int)toPixels.window.ymin;
  x = (width-topWidth)/2;
  if (landscape) y = (height-topHeight)/2;
  else y = (width-topHeight)/2;
  if (y<0) y = 0;
  if (x<0) x = 0;
  xeng = gx_new_engine(s, name, &toPixels, -x,-y,0,0, sizeof(gp_x_engine_t));

  xeng->wtop = topWidth;
  xeng->htop = topHeight;
  /* possibly want optional PL_RGBMODEL as well */
  hints = (gp_private_map?PL_PRIVMAP:0) | (gp_input_hint?0:PL_NOKEY) |
    (gp_rgb_hint?PL_RGBMODEL:0);
  xeng->win = xeng->w = gx_parent?
    pl_subwindow(s, topWidth, topHeight,
                gx_parent, gx_xloc, gx_yloc, PL_BG, hints, xeng) :
    pl_window(s, topWidth, topHeight, name, PL_BG, hints, xeng);
  gx_parent = 0;
  if (!xeng->win) {
    gp_delete_engine(&xeng->e);
    return 0;
  }

  return &xeng->e;
}

int
gx_input(gp_engine_t *engine,
        void (*HandleExpose)(gp_engine_t *, gd_drawing_t *, int *),
        void (*HandleClick)(gp_engine_t *,int,int,int,int,unsigned long),
        void (*HandleMotion)(gp_engine_t *,int,int,int),
        void (*HandleKey)(gp_engine_t *,int,int))
{
  gp_x_engine_t *xeng = gp_x_engine(engine);
  if (!xeng) return 1;
  xeng->HandleExpose = HandleExpose;
  xeng->HandleClick = HandleClick;
  xeng->HandleMotion = HandleMotion;
  xeng->HandleKey = HandleKey;
  return 0;
}

gp_x_engine_t *
gp_x_engine(gp_engine_t *engine)
{
  return (engine && engine->on==&g_x_on)? (gp_x_engine_t *)engine : 0;
}

/* ------------------------------------------------------------------------ */

int
gx_animate(gp_engine_t *engine, gp_box_t *viewport)
{
  gp_x_engine_t *xeng = gp_x_engine(engine);
  int x, y, x1, y1;
  gp_box_t *v, *w;
  gp_real_t xmin, xmax, ymin, ymax;
  gp_real_t scalx, offx, scaly, offy;

  if (!xeng || !xeng->w) return 1;
  if (xeng->w!=xeng->win) gx_direct(engine);

  v = &xeng->e.transform.viewport;  /* NDC */
  w = &xeng->e.transform.window;    /* pixels */

  /* get NDC-->pixel mapping coefficients */
  scalx = xeng->e.devMap.x.scale;
  offx = xeng->e.devMap.x.offset;
  scaly = xeng->e.devMap.y.scale;
  offy = xeng->e.devMap.y.offset;

  /* clip given viewport to portion of NDC space which is actually
   * visible now -- note that v is either gp_landscape_box or gp_portrait_box,
   * so that min<max for v; must also be true for input viewport */
  GetVisibleNDC(xeng, &xmin, &xmax, &ymin, &ymax);
  if (viewport->xmin>xmin) xmin = viewport->xmin;
  if (viewport->xmax<xmax) xmax = viewport->xmax;
  if (viewport->ymin>ymin) ymin = viewport->ymin;
  if (viewport->ymax<ymax) ymax = viewport->ymax;

  /* install NDC-->pixel transform for animation pixmap */
  v->xmin = xmin;
  v->xmax = xmax;
  v->ymin = ymin;
  v->ymax = ymax;

  /* set the engine transform to map the specified viewport into
   * offscreen pixels, and get (x,y) offset from full window pixels
   * to offscreen pixels */
  w->xmin = scalx*xmin+offx;
  w->xmax = scalx*xmax+offx;
  if (w->xmax > w->xmin) {
    x = (int)w->xmin;
    w->xmax -= w->xmin;
    w->xmin = 0.0;
  } else {
    x = (int)w->xmax;
    w->xmin -= w->xmax;
    w->xmax = 0.0;
  }
  w->ymin = scaly*ymin+offy;
  w->ymax = scaly*ymax+offy;
  if (w->ymax > w->ymin) {
    y = (int)w->ymin;
    w->ymax -= w->ymin;
    w->ymin = 0.0;
  } else {
    y = (int)w->ymax;
    w->ymin -= w->ymax;
    w->ymax = 0.0;
  }
  gp_set_device_map((gp_engine_t *)xeng);
  /* GetXRectangle(&xeng->e.devMap, v, &x0, &y0, &x1, &y1);
     x1 -= x0;
     y1 -= y0;
  */
  x1 = xeng->wtop;
  y1 = xeng->htop;

  /* create the offscreen pixmap */
  xeng->w = pl_offscreen(xeng->win, x1, y1);
  if (!xeng->w) {
    xeng->w = xeng->win;
    xeng->e.transform = xeng->swapped;
    gp_set_device_map((gp_engine_t *)xeng);
    return 2;
  }
  xeng->a_width = x1;
  xeng->a_height = y1;
  xeng->a_x = x;
  xeng->a_y = y;

  /* set coordinate mapping for offscreen */
  ChangeMap((gp_engine_t *)xeng);

  /* reset mapping clip to whole visible window */
  if (xeng->wtop>0) x1 = xeng->wtop+xeng->leftMargin;
  else x1 = xeng->leftMargin+1;
  if (xeng->htop>0) y1 = xeng->htop+xeng->topMargin;
  else y1 = xeng->topMargin+1;
  xeng->clipping = 1;
  pl_clip(xeng->win, xeng->leftMargin, xeng->topMargin, x1, y1);

  pl_clear(xeng->w);
  return 0;
}

static void
GetVisibleNDC(gp_x_engine_t *xeng,
              gp_real_t *xn, gp_real_t *xx, gp_real_t *yn, gp_real_t *yx)
{
  gp_real_t scalx = xeng->e.devMap.x.scale;
  gp_real_t offx = xeng->e.devMap.x.offset;
  gp_real_t scaly = xeng->e.devMap.y.scale;
  gp_real_t offy = xeng->e.devMap.y.offset;
  int xmin, xmax, ymin, ymax;

  xmin = xeng->leftMargin;
  xmax = xmin+xeng->wtop;
  ymax = xeng->topMargin;
  ymin = ymax+xeng->htop;

  /* Convert pixels to NDC coordinates */
  *xn = (xmin-offx)/scalx;
  *xx = (xmax-offx)/scalx;
  *yn = (ymin-offy)/scaly;
  *yx = (ymax-offy)/scaly;
}

int
gx_strobe(gp_engine_t *engine, int clear)
{
  gp_x_engine_t *xeng = gp_x_engine(engine);

  if (!xeng || !xeng->w || xeng->w==xeng->win) return 1;

  pl_bitblt(xeng->win, xeng->a_x, xeng->a_y, xeng->w,
           0, 0, xeng->a_width, xeng->a_height);
  if (clear) pl_clear(xeng->w);

  return 0;
}

int
gx_direct(gp_engine_t *engine)
{
  gp_x_engine_t *xeng = gp_x_engine(engine);

  if (!xeng || !xeng->w || xeng->w==xeng->win) return 1;

  pl_destroy(xeng->w);
  xeng->w = xeng->win;

  /* set coordinate map and clipping to values for graphics window */
  xeng->e.transform = xeng->swapped;
  gp_set_device_map((gp_engine_t *)xeng);
  ChangeMap((gp_engine_t *)xeng);

  return 0;
}

/* ------------------------------------------------------------------------ */

void (*_gh_hook)(gp_engine_t *engine)= 0;

static void
g_do_disconnect(void)
{
  if (g_screens) {
    pl_scr_t *s;
    int i;
    for (i=n_screens-1 ; i>=0 ; i--) {
      s = g_screens[i].s;
      if (s && !pl_wincount(s)) gx_disconnect(s);
    }
  }
  g_pending_task = 0;
}

static void
ShutDown(gp_x_engine_t *xeng)
{
  pl_scr_t *s = xeng->s;
  pl_win_t *w = xeng->w;
  pl_win_t *win = xeng->win;
  if ((gp_engine_t *)xeng == gx_current_engine) gx_current_engine = NULL;
  xeng->mapped= 0;
  /* turn off all event callbacks (probably unnecessary) */
  xeng->e.on = 0;
  /* destroy any hlevel references without further ado */
  if (_gh_hook != NULL) _gh_hook((gp_engine_t*)xeng);
  xeng->w = xeng->win = 0;
  xeng->s = 0;
  /* note that pl_destroy and gp_delete_engine call on_destroy in Windows */
  if (w && w!=win) pl_destroy(w);
  gp_delete_engine(&xeng->e);
  if (s) {
    if (!pl_wincount(s))
      g_pending_task = g_do_disconnect;
  }
}

static void (*XErrHandler)(char *errMsg)= 0;

static void
g_on_panic(pl_scr_t *screen)
{
  gp_engine_t *eng = 0;
  gp_x_engine_t *xeng = 0;
  do {
    for (eng=gp_next_engine(eng) ; eng ; eng=gp_next_engine(eng)) {
      xeng= gp_x_engine(eng);
      if (xeng && xeng->s==screen) break;
    }
    if (eng) {
      xeng->s = 0;  /* be sure not to call pl_disconnect */
      Kill(eng);
    }
  } while (eng);
  XErrHandler("play on_panic called (screen graphics engines killed)");
}

/* this routine actually calls the XErrHandler, which may not return
   and/or which may trigger additional X protocol requests */
static void GxErrorHandler(void)
{
  char msg[80];
  gxErrorFlag= 0;
  XErrHandler(msg);
}

int gp_set_xhandler(void (*ErrHandler)(char *errMsg))
{
  /* install X error handlers which don't call exit */
  XErrHandler= ErrHandler;
  return 0;
}

/* ------------------------------------------------------------------------ */

int
gh_rgb_read(gp_engine_t *eng, gp_color_t *rgb, long *nx, long *ny)
{
  gp_x_engine_t *xeng = gp_x_engine(eng);
  if (!xeng || !xeng->w || !xeng->win) return 1;
  gp_preempt(eng);
  gd_draw(1);       /* make sure screen updated */
  gp_preempt(0);
  if (xeng->w == xeng->win) {
    /* not in animate mode */
    if (!rgb) {
      *nx = xeng->wtop;
      *ny = xeng->htop;
    } else {
      pl_rgb_read(xeng->win, rgb, xeng->leftMargin, xeng->topMargin,
                 xeng->leftMargin+xeng->wtop, xeng->topMargin+xeng->htop);
    }
  } else {
    /* in animate mode, read offscreen pixmap */
    if (!rgb) {
      *nx = xeng->a_width;
      *ny = xeng->a_height;
    } else {
      pl_rgb_read(xeng->w, rgb, 0, 0, xeng->a_width, xeng->a_height);
    }
  }
  return 0;
}

/* ------------------------------------------------------------------------ */
