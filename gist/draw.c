/*
 * $Id: draw.c,v 1.1 2005-09-18 22:04:28 dhmunro Exp $
 * Implement display list portion of GIST C interface
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist/draw.h"
#include "gist/text.h"
#include "play/std.h"

/* Generating default contour labels requires sprintf function */
#include <stdio.h>

#include <string.h>

extern double log10(double);
#define SAFELOG0 (-999.)
#define SAFELOG(x) ((x)>0? log10(x) : ((x)<0? log10(-(x)) : -999.))

/* In tick.c */
extern gp_real_t GpNiceUnit(gp_real_t finest, int *base, int *power);

extern double floor(double);
extern double ceil(double);
#ifndef NO_EXP10
  extern double exp10(double);
#else
# define exp10(x) pow(10.,x)
  extern double pow(double,double);
#endif
#define LOG2 0.301029996

/* ------------------------------------------------------------------------ */

static gd_drawing_t *currentDr= 0;  /* gd_drawing_t from gd_new_drawing or GdGetDrawing */
static ge_system_t *currentSy;    /* System from gd_new_system or gd_set_system */
static gd_element_t *currentEl;   /* Element extracted with gd_set_system, or
                                  GdGetElement */
static int currentCn;          /* level selected by GdGetContour */

/* Saved state information used by gd_set_drawing */
static gd_drawing_t *saveDr= 0;
static ge_system_t *saveSy;
static gd_element_t *saveEl;
static int saveCn;

gd_properties_t gd_properties= {
  0, 0,
  {
  {7.5, 50., 1.2, 1.2, 4, 1, GA_TICK_L|GA_TICK_U|GA_TICK_OUT|GA_LABEL_L,
   0.0, 14.0*GP_ONE_POINT,
   {12.*GP_ONE_POINT, 8.*GP_ONE_POINT, 5.*GP_ONE_POINT, 3.*GP_ONE_POINT, 2.*GP_ONE_POINT},
     {PL_FG, GP_LINE_SOLID, 1.0},
     {PL_FG, GP_LINE_DOT, 1.0},
     {PL_FG, GP_FONT_HELVETICA, 14.*GP_ONE_POINT,
        GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_NORMAL, GP_VERT_ALIGN_NORMAL, 1},
     .425, .5-52.*GP_ONE_POINT},
  {7.5, 50., 1.2, 1.2, 3, 1, GA_TICK_L|GA_TICK_U|GA_TICK_OUT|GA_LABEL_L,
   0.0, 14.0*GP_ONE_POINT,
   {12.*GP_ONE_POINT, 8.*GP_ONE_POINT, 5.*GP_ONE_POINT, 3.*GP_ONE_POINT, 2.*GP_ONE_POINT},
     {PL_FG, GP_LINE_SOLID, 1.0},
     {PL_FG, GP_LINE_DOT, 1.0},
     {PL_FG, GP_FONT_HELVETICA, 14.*GP_ONE_POINT,
        GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_NORMAL, GP_VERT_ALIGN_NORMAL, 1},
     .25, .5-52.*GP_ONE_POINT},
  0, {PL_FG, GP_LINE_SOLID, 1.0}
  },
  {{ 0.0, 1.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }},
  GD_LIM_XMIN | GD_LIM_XMAX | GD_LIM_YMIN | GD_LIM_YMAX,    /* flags */
  { 0.0, 1.0, 0.0, 1.0 },               /* limits */
  0, 0, 0, 0, 0,
  0.0, 0.0, 0,
  0.0, 0.0, 0.0, 0.0, 0, 0,             /* gd_add_cells */
  0, 0,
  0, { 0, 0, 0, 0, 0, 0 }, 0,           /* noCopy, mesh, region */
  0, 0,
  0, 0, 0.0,
  0, 0, 0,  0 };

gd_drawing_t *gd_draw_list= 0;

static void ClearDrawing(gd_drawing_t *drawing);
static void Damage(ge_system_t *sys, gd_element_t *el);
static void SquareAdjust(gp_real_t *umin, gp_real_t *umax,
                         gp_real_t dv, int doMin, int doMax);
static void NiceAdjust(gp_real_t *umin, gp_real_t *umax, int isLog, int isMin);
static void EqAdjust(gp_real_t *umin, gp_real_t *umax);
static void EmptyAdjust(gp_real_t *umin, gp_real_t *umax, int doMin, int doMax);
static void EqualAdjust(gp_real_t *umin, gp_real_t *umax, int doMin, int doMax);
extern int Gd_DrawRing(void *elv, int xIsLog, int yIsLog,
                       ge_system_t *sys, int t);
static void InitLegends(int contours, ge_system_t *systems, gd_element_t *elements,
                        int size);
static void NextContours(void);
static int NextRing(void);
static int NextLegend(void);
static int BuildLegends(int more, int contours, ge_system_t *systems,
                        gd_element_t *elements, ge_legend_box_t *lbox);
static int MemoryError(void);
static void *Copy1(const void *orig, long size);
static void *Copy2(void *x1, const void *orig1, const void *orig2, long size);
extern void Gd_ScanZ(long n, const gp_real_t *z, gp_real_t *zmin, gp_real_t *zmax);
static void ScanXY(long n, const gp_real_t *x, const gp_real_t *y, gp_box_t *extrema);

extern void Gd_NextMeshBlock(long *ii, long *jj, long len, long iMax,
                             int *reg, int region);
extern void Gd_MeshXYGet(void *vMeshEl);
static int AutoMarker(ga_line_attribs_t *dl, int number);
extern int Gd_MakeContours(ge_contours_t *con);
static void GuessBox(gp_box_t *box, gp_box_t *viewport, ga_tick_style_t *ticks);
static gd_element_t *NextConCurve(gd_element_t *el);
static int GeFindIndex(int id, ge_system_t *sys);

extern void Gd_KillRing(void *elv);
extern void Gd_KillMeshXY(void *vMeshEl);

static void (*DisjointKill)(void *el);
static void (*FilledKill)(void *el);
static void (*VectorsKill)(void *el);
static void (*ContoursKill)(void *el);
static void (*SystemKill)(void *el);
static int (*LinesGet)(void *el);
static int (*ContoursGet)(void *el);
extern void Gd_LinesSubSet(void *el);
static int (*SystemDraw)(void *el, int xIsLog, int yIsLog);

/* ------------------------------------------------------------------------ */
/* Set virtual function tables */

extern gd_operations_t *GetDrawingOpTables(void);  /* in draw0.c */
static gd_operations_t *opTables= 0;

/* ------------------------------------------------------------------------ */
/* Constructor and destructor for gd_drawing_t declared in gist2.h */

gd_drawing_t *gd_new_drawing(char *gsFile)
{
  gd_drawing_t *drawing= pl_malloc(sizeof(gd_drawing_t));
  if (!drawing) return 0;
  if (!opTables) {
    opTables= GetDrawingOpTables();
    DisjointKill= opTables[GD_ELEM_DISJOINT].Kill;
    FilledKill= opTables[GD_ELEM_FILLED].Kill;
    VectorsKill= opTables[GD_ELEM_VECTORS].Kill;
    ContoursKill= opTables[GD_ELEM_CONTOURS].Kill;
    SystemKill= opTables[GD_ELEM_SYSTEM].Kill;
    LinesGet= opTables[GD_ELEM_LINES].GetProps;
    ContoursGet= opTables[GD_ELEM_CONTOURS].GetProps;
    SystemDraw= opTables[GD_ELEM_SYSTEM].Draw;
  }

  drawing->next= gd_draw_list;
  gd_draw_list= drawing;
  drawing->cleared=
    drawing->nSystems= drawing->nElements= 0;
  drawing->systems= 0;
  drawing->elements= 0;
  drawing->damaged= 0;
  drawing->damage.xmin= drawing->damage.xmax=
    drawing->damage.ymin= drawing->damage.ymax= 0.0;
  drawing->landscape= 0;
  drawing->legends[0].nlines= drawing->legends[1].nlines= 0;

  gd_set_drawing(drawing);

  if (gd_read_style(drawing, gsFile)) {
    gd_set_drawing(0);
    gd_kill_drawing(drawing);
    return 0;
  }

  return drawing;
}

int gd_set_landscape(int landscape)
{
  if (!currentDr) return 1;
  if (landscape) landscape= 1;
  if (currentDr->landscape!=landscape) {
    currentDr->landscape= landscape;
    gd_detach(currentDr, 0);
  }
  return 0;
}

void gd_kill_drawing(gd_drawing_t *drawing)
{
  if (!drawing) {
    drawing= currentDr;
    if (!drawing) return;
  }

  ClearDrawing(drawing);
  Gd_KillRing(drawing->systems);

  if (drawing==gd_draw_list) gd_draw_list= drawing->next;
  else {
    gd_drawing_t *draw= gd_draw_list;
    while (draw->next!=drawing) draw= draw->next;
    draw->next= drawing->next;
  }

  if (drawing==currentDr) currentDr= 0;

  pl_free(drawing);
}

extern void GdKillSystems(void);

void GdKillSystems(void)
{
  if (!currentDr) return;
  ClearDrawing(currentDr);
  Gd_KillRing(currentDr->systems);
  currentDr->systems= 0;
  currentDr->nSystems= 0;
}

int gd_set_drawing(gd_drawing_t *drawing)
{
  int nMax, sysIndex, i;
  ge_system_t *sys;

  if (!drawing) {  /* swap current and saved state info */
    gd_drawing_t *tmpDr= currentDr;
    ge_system_t *tmpSy= currentSy;
    gd_element_t *tmpEl= currentEl;
    int tmpCn= currentCn;
    currentDr= saveDr;   saveDr= tmpDr;
    currentSy= saveSy;   saveSy= tmpSy;
    currentEl= saveEl;   saveEl= tmpEl;
    currentCn= saveCn;   saveCn= tmpCn;
    return 0;
  }

  saveDr= currentDr;
  saveSy= currentSy;
  saveEl= currentEl;
  saveCn= currentCn;

  currentDr= drawing;

  /* Make a reasonable guess at current system and element */
  nMax= drawing->elements? drawing->elements->prev->number : -1;
  sysIndex= drawing->nSystems? 1 : 0;
  i= 0;
  if ((sys= drawing->systems)) do {
    i++;
    if (sys->el.number>nMax) { nMax= sys->el.number;  sysIndex= i; }
    sys= (ge_system_t *)sys->el.next;
  } while (sys!=drawing->systems);
  gd_set_system(sysIndex);
  if (nMax<0) {
    if (sysIndex<1) currentSy= 0;
    currentEl= 0;
  } else {
    gd_element_t *el= currentSy? currentSy->elements : drawing->elements;
    if (el) {
      currentEl= el->prev;
      currentEl->ops->GetProps(currentEl);
    } else {
      currentEl= 0;
    }
  }
  currentCn= -1;

  return 0;
}

int gd_clear(gd_drawing_t *drawing)
{
  if (!drawing) drawing= currentDr;
  if (!drawing) return 1;
  drawing->cleared= 1;
  return 0;
}

static void ClearDrawing(gd_drawing_t *drawing)
{
  ge_system_t *sys, *sys0= drawing->systems;
  int nSystems= 0;
  if ((sys= sys0)) do {
    Gd_KillRing(sys->elements);
    sys->elements= 0;
    sys->rescan= 0;
    sys->unscanned= -1;
    sys->el.number= -1;
    sys= (ge_system_t *)sys->el.next;
    nSystems++;
  } while (sys!=sys0);
  Gd_KillRing(drawing->elements);
  drawing->elements= 0;
  drawing->nElements= 0;
  drawing->nSystems= nSystems;
  drawing->cleared= 2;

  if (drawing==currentDr) {
    currentSy= drawing->systems; /* as after gd_set_drawing */
    currentEl= 0;
    currentCn= -1;
  }

  /* Must detatch drawing from all engines, since even inactive ones
     need to know that the drawing has been erased.  */
  gd_detach(drawing, (gp_engine_t *)0);
}

/* ------------------------------------------------------------------------ */

/* The GIST display list is called a "drawing".  Any number of drawings
   may exist, but only one may be active at a time.

   The entire drawing is rendered on all active engines by calling
   gd_draw(0).  The drawing sequence is preceded by gp_clear(0, GP_CONDITIONALLY).

   gd_draw(-1) is like gd_draw(0) except the drawing is damaged to force
   all data to be rescanned as well.

   gd_draw(1) draws only the changes since the previous gd_draw.  Changes can
   be either destructive or non-destructive:

     If the engine->damage box is set, then some operation has
     damaged that part of the drawing since the prior gd_draw
     (such as changing a line style or plot limits).  The damaged
     section is cleared, then the display list is traversed, redrawing
     all elements which may intersect the damaged box, clipped to
     the damaged box.

     Then, any elements which were added since the prior gd_draw
     (and which caused no damage like a change in limits) are
     drawn as non-destructive updates.

 */

static void Damage(ge_system_t *sys, gd_element_t *el)
{
  gp_box_t *box, adjustBox;
  if (!currentDr) return;
  if (!el) {
    if (!sys) return;
    /* If no element, damage the entire coordinate system, ticks and all */
    box= &sys->el.box;
  } else if (sys) {
    /* If element is in a coordinate system, damage the whole viewport */
    box= &sys->trans.viewport;
  } else {
    /* Elements not in a coordinate system already have NDC box--
       but may need to adjust it to allow for projecting junk.  */
    el->ops->Margin(el, &adjustBox);
    adjustBox.xmin+= el->box.xmin;
    adjustBox.xmax+= el->box.xmax;
    adjustBox.ymin+= el->box.ymin;
    adjustBox.ymax+= el->box.ymax;
    box= &adjustBox;
  }
  if (currentDr->damaged) {
    gp_swallow(&currentDr->damage, box);
  } else {
    currentDr->damage= *box;
    currentDr->damaged= 1;
  }
}

static void SquareAdjust(gp_real_t *umin, gp_real_t *umax,
                         gp_real_t dv, int doMin, int doMax)
{
  if (doMin) {
    if (doMax) umax[0]= 0.5*(umin[0]+umax[0]+dv);
    umin[0]= umax[0]-dv;
  } else if (doMax) {
    umax[0]= umin[0]+dv;
  }
}

static void NiceAdjust(gp_real_t *umin, gp_real_t *umax, int isLog, int isMin)
{
  gp_real_t un= *umin,   ux= *umax;
  gp_real_t unit,   du= ux-un;
  int base, power, reverted= 0;
  if (isLog) {
    if (du <= LOG2) {
      /* revert to linear scale */
      un= exp10(un);
      ux= exp10(ux);
      du= ux-un;
      reverted= 1;
    } else if (du>6.0 && isMin) {
      un= ux-6.0;
      du= 6.0;
    }
  }
  unit= GpNiceUnit(du/3.0, &base, &power);
  if (!isLog || reverted || unit>0.75) {
    un= unit*floor(un/unit);
    ux= unit*ceil(ux/unit);
    if (reverted) {
      un= log10(un);
      ux= log10(ux);
    }
  } else {
    /* subdecade log scale (sigh), use 2, 5, or 10 */
    gp_real_t dn= floor(un+0.0001),   dx= ceil(ux-0.0001);
    if (un<dn+(LOG2-0.0001)) un= dn;
    else if (un<dn+(0.9999-LOG2)) un= dn+LOG2;
    else un= dn+(1.0-LOG2);
    if (ux>dx-(LOG2+0.0001)) ux= dx;
    else if (ux>dx-(1.0001-LOG2)) ux= dx-LOG2;
    else ux= ux-(1.0-LOG2);
  }
  *umin= un;
  *umax= ux;
}

static void EqAdjust(gp_real_t *umin, gp_real_t *umax)
{
  gp_real_t nudge= *umin>0.0? 0.001*(*umin) : -0.001*(*umin);
  if (nudge==0.0) nudge= 1.0e-6;
  *umin-= nudge;
  *umax+= nudge;
}

static void EmptyAdjust(gp_real_t *umin, gp_real_t *umax, int doMin, int doMax)
{
  if (doMin) {
    if (doMax) { *umin= -1.0e-6; *umax= +1.0e-6; }
    else if (*umax>0.0) *umin= 0.999*(*umax);
    else if (*umax<0.0) *umin= 1.001*(*umax);
    else *umin= -1.0e-6;
  } else if (doMax) {
    if (*umin>0.0) *umax= 1.001*(*umin);
    else if (*umin<0.0) *umax= 0.999*(*umin);
    else *umax= +1.0e-6;
  } else if ((*umin)==(*umax)) {
    EqAdjust(umin, umax);
  }
}

static void EqualAdjust(gp_real_t *umin, gp_real_t *umax, int doMin, int doMax)
{
  if (doMin && doMax) EqAdjust(umin, umax);
  else EmptyAdjust(umin, umax, doMin, doMax);
}

int gd_scan(ge_system_t *sys)
{
  int flags= sys->flags;
  gp_box_t limits, tmp, *w= &sys->trans.window;
  gp_real_t xmin= w->xmin, xmax= w->xmax, ymin= w->ymin, ymax= w->ymax;
  int swapx, swapy;
  gd_element_t *el, *el0= sys->elements;
  int begin, damaged, first;

  /* Handle case of no curves (if, e.g., all elements removed) */
  if (!el0) {
    EmptyAdjust(&w->xmin, &w->xmax, flags&GD_LIM_XMIN, flags&GD_LIM_XMAX);
    EmptyAdjust(&w->ymin, &w->ymax, flags&GD_LIM_YMIN, flags&GD_LIM_YMAX);
    return 0;
  }

  /* Assure that limits are ordered even if window is not */
  swapx= (xmin > xmax) && !(flags&(GD_LIM_XMIN|GD_LIM_XMAX));
  swapy= (ymin > ymax) && !(flags&(GD_LIM_YMIN|GD_LIM_YMAX));
  if (!swapx) { limits.xmin= xmin;  limits.xmax= xmax; }
  else { limits.xmin= xmax;  limits.xmax= xmin; }
  if (!swapy) { limits.ymin= ymin;  limits.ymax= ymax; }
  else { limits.ymin= ymax;  limits.ymax= ymin; }
  tmp= limits;

  el= el0;
  begin= sys->rescan? -1 : sys->unscanned;

  /* Scan limits for each element */
  damaged= 0;
  first= 1;
  do {
    if (!el->hidden) {
      if (el->number>=begin) {
        /* Scan ensures log values present, sets box, scans xy values */
        if (el->ops->Scan(el, flags, &tmp)) return 1; /* mem failure */
        if (first) {
          /* first non-hidden element gives first cut at limits */
          limits= tmp;
          damaged= 1;
        } else {
          /* subsequent elements may cause limits to be adjusted */
          if (tmp.xmin<=tmp.xmax) {
            if (tmp.xmin<limits.xmin) limits.xmin= tmp.xmin;
            if (tmp.xmax>limits.xmax) limits.xmax= tmp.xmax;
          }
          if (tmp.ymin<=tmp.ymax) {
            if (tmp.ymin<limits.ymin) limits.ymin= tmp.ymin;
            if (tmp.ymax>limits.ymax) limits.ymax= tmp.ymax;
          }
        }
      }
      first= 0;
    }
    el= el->next;
  } while (el!=el0);

  /* #1- adjust if min==max */
  if (limits.xmin==limits.xmax)
    EqualAdjust(&limits.xmin, &limits.xmax, flags&GD_LIM_XMIN, flags&GD_LIM_XMAX);
  if (limits.ymin==limits.ymax)
    EqualAdjust(&limits.ymin, &limits.ymax, flags&GD_LIM_XMIN, flags&GD_LIM_XMAX);

  /* #2- adjust if log axis and minimum was SAFELOG(0) */
  if ((flags & GD_LIM_LOGX) && (flags & GD_LIM_XMIN) && limits.xmin==SAFELOG0
      && limits.xmax>SAFELOG0+10.0) limits.xmin= limits.xmax-10.0;
  if ((flags & GD_LIM_LOGY) && (flags & GD_LIM_YMIN) && limits.ymin==SAFELOG0
      && limits.ymax>SAFELOG0+10.0) limits.ymin= limits.ymax-10.0;

  /* #3- adjust if square limits specified and not semi-logarithmic */
  if ((flags & GD_LIM_SQUARE) &&
      !(((flags&GD_LIM_LOGX)!=0) ^ ((flags&GD_LIM_LOGY)!=0))) {
    /* (Square axes don't make sense for semi-log scales) */
    gp_real_t dx= limits.xmax-limits.xmin;
    gp_real_t dy= limits.ymax-limits.ymin;
    gp_real_t dydx= (sys->trans.viewport.ymax-sys->trans.viewport.ymin)/
                 (sys->trans.viewport.xmax-sys->trans.viewport.xmin);
    /* Adjust y if either (1) dx>dy, or (2) x limits are both fixed
       (NB- SquareAdjust is a noop if both limits fixed) */
    if ((dx*dydx>dy && (flags&(GD_LIM_YMIN|GD_LIM_YMAX))) ||
        !(flags&(GD_LIM_XMIN|GD_LIM_XMAX)))
      SquareAdjust(&limits.ymin, &limits.ymax, dx*dydx,
                   flags&GD_LIM_YMIN, flags&GD_LIM_YMAX);
    else /* adjust x */
      SquareAdjust(&limits.xmin, &limits.xmax, dy/dydx,
                   flags&GD_LIM_XMIN, flags&GD_LIM_XMAX);
  }

  /* #4- adjust if nice limits specified */
  if (flags & GD_LIM_NICE) {
    NiceAdjust(&limits.xmin, &limits.xmax, flags&GD_LIM_LOGX, flags&GD_LIM_XMIN);
    NiceAdjust(&limits.ymin, &limits.ymax, flags&GD_LIM_LOGY, flags&GD_LIM_YMIN);
  }

  if (swapx) {
    gp_real_t tmp= limits.xmin;  limits.xmin= limits.xmax;  limits.xmax= tmp;
  }
  if (swapy) {
    gp_real_t tmp= limits.ymin;  limits.ymin= limits.ymax;  limits.ymax= tmp;
  }
  if (damaged || limits.xmin!=xmin || limits.xmax!=xmax ||
      limits.ymin!=ymin || limits.ymax!=ymax)
    Damage(sys, (gd_element_t *)0);
  w->xmin= limits.xmin;
  w->xmax= limits.xmax;
  w->ymin= limits.ymin;
  w->ymax= limits.ymax;

  sys->rescan= 0;
  sys->unscanned= -1;

  return 0;
}

int Gd_DrawRing(void *elv, int xIsLog, int yIsLog, ge_system_t *sys, int t)
{
  gd_element_t *el0, *el= elv;
  gp_box_t adjustBox, *box;
  int value= 0, drawIt= t;
  if ((el0= el)) {
    do {
      if (!t) {
        if (!sys) {
          el->ops->Margin(el, &adjustBox);
          adjustBox.xmin+= el->box.xmin;
          adjustBox.xmax+= el->box.xmax;
          adjustBox.ymin+= el->box.ymin;
          adjustBox.ymax+= el->box.ymax;
          box= &adjustBox;
        } else {
          box= &sys->trans.viewport;
        }
        drawIt= _gd_begin_el(box, el->number);
      }
      if (drawIt) value|= el->ops->Draw(el, xIsLog, yIsLog);
      el= el->next;
    } while (el!=el0);
  }
  return value;
}

static gp_transform_t unitTrans= { {0., 2., 0., 2.}, {0., 2., 0., 2.} };

int gd_draw(int changesOnly)
{
  int value= 0;
  gp_box_t *damage;
  int systemCounter;
  int rescan= 0;

  if (!currentDr) return 1;

  if (changesOnly==-1) {
    rescan= 1;
    changesOnly= 0;
  }

  /* Take care of conditional clear */
  if (currentDr->cleared==1) {
    if (changesOnly) return 0;
    else ClearDrawing(currentDr);
  }
  if (!changesOnly || currentDr->cleared) {
    gp_clear(0, GP_CONDITIONALLY);
    currentDr->cleared= 0;
  }

  /* Check if any coordinate systems need to be rescanned */
  if (currentDr->systems) {
    int changed;
    ge_system_t *sys, *sys0;
    sys= sys0= currentDr->systems;
    do {
      if (rescan) sys->rescan= 1;
      changed= (sys->rescan || sys->unscanned>=0);
      if (changed) changesOnly= 0;
      if (changed && gd_scan(sys)) return 1;  /* memory manager failure */
      sys= (ge_system_t *)sys->el.next;
    } while (sys!=sys0);
  }

  /* Give engines a chance to prepare for a drawing */
  if (currentDr->damaged) {
    damage= &currentDr->damage;
    currentDr->damaged= 0;
  } else {
    damage= 0;
  }
  /* _gd_begin_dr returns 1 if any active engine has been cleared or
     partially cleared.  */
  if (!_gd_begin_dr(currentDr, damage, currentDr->landscape) && changesOnly)
    return 0;

  /* Do coordinate systems */
  if (currentDr->systems) {
    ge_system_t *sys, *sys0;
    sys= sys0= currentDr->systems;
    systemCounter= 0;
    do {
      value|= SystemDraw(sys, systemCounter, 0);
      systemCounter++;
      sys= (ge_system_t *)sys->el.next;
    } while (sys!=sys0);
  }

  /* Do elements outside of coordinate systems */
  gp_set_transform(&unitTrans);
  gp_clipping= 0;
  value|= Gd_DrawRing(currentDr->elements, 0, 0, (ge_system_t *)0, 0);

  /* Give engines a chance to clean up after a drawing */
  _gd_end_dr();

  return value;
}

/* ------------------------------------------------------------------------ */
/* Legend routines */

int gd_set_legend_box(int which, gp_real_t x, gp_real_t y, gp_real_t dx, gp_real_t dy,
                const gp_text_attribs_t *t, int nchars, int nlines, int nwrap)
{
  ge_legend_box_t *lbox;
  if (!currentDr || nchars<0) return 1;
  lbox= currentDr->legends;
  if (which) lbox++;
  lbox->x= x;     lbox->y= y;
  lbox->dx= dx;   lbox->dy= dy;
  lbox->textStyle= *t;
  lbox->nchars= nchars;
  lbox->nlines= nlines;
  lbox->nwrap= nwrap;
  return 0;
}

static char *legendText= 0;
static long lenLegends, maxLegends= 0;

static int nRemaining, curWrap;
static char *curLegend;
static int curMarker= 0;

static int doingContours, levelCurve, nLevels;
static gd_element_t *curElement, *cur0Element, *drElements, *curCon, *cur0Con;
static ge_system_t *curSystem, *cur0System;
static gp_real_t *levelValue;
static ge_lines_t **levelGroup;
static char levelLegend[32];

static void InitLegends(int contours, ge_system_t *systems, gd_element_t *elements,
                        int size)
{
  doingContours= levelCurve= contours;
  if (doingContours) curCon= 0;
  curElement= 0;
  curSystem= cur0System= systems;
  drElements= elements;
  curLegend= 0;
  curMarker= 0;
  nRemaining= 0;

  if (size>maxLegends) {
    if (legendText) pl_free(legendText);
    legendText= pl_malloc((long)size);
  }
}

static void NextContours(void)
{
  if (!levelCurve) {
    /* Set up for the ring of level curves */
    ge_contours_t *con= (ge_contours_t *)curCon;
    nLevels= con->nLevels;
    levelValue= con->levels;
    levelGroup= con->groups;
    levelCurve= 1;
    if (levelGroup) {
      while (nLevels && !levelGroup[0]) {
        levelValue++;
        levelGroup++;
        nLevels--;
      }
    } else {
      nLevels= 0;
    }
    if (nLevels>0) curElement= (gd_element_t *)levelGroup[0];
    else curElement= 0;
    return;
  }

  levelCurve= 0;
  curElement= 0;
  if (curCon) {
    curCon= curCon->next;
    if (curCon==cur0Con) curCon= 0;
  }
  for (;;) {
    if (curCon) {
      do {
        if (curCon->ops->type==GD_ELEM_CONTOURS && !curCon->hidden) {
          /* Set up for contour element itself-- terminates immediately */
          curElement= curCon;
          cur0Element= curElement->next;
          return;
        }
        curCon= curCon->next;
      } while (curCon!=cur0Con);
    }

    if (curSystem) {
      curCon= cur0Con= curSystem->elements;
      curSystem= (ge_system_t *)curSystem->el.next;
      if (curSystem==cur0System) curSystem= 0;
    } else if (drElements) {
      curCon= cur0Con= drElements;
      drElements= 0;
    } else {
      break;
    }
  }
}

static int NextRing(void)
{
  if (doingContours) {
    NextContours();
    if (!curElement) return 0;
  } else if (curSystem) {
    curElement= cur0Element= curSystem->elements;
    curSystem= (ge_system_t *)curSystem->el.next;
    if (curSystem==cur0System) curSystem= 0;
  } else if (drElements) {
    curElement= cur0Element= drElements;
    drElements= 0;
  } else {
    return 0;
  }
  return 1;
}

static int specialMarks[5]= { '.', '+', '*', 'o', 'x' };

static int NextLegend(void)
{
  curLegend= 0;
  curMarker= 0;
  do {
    while (curElement) {
      if (!curElement->hidden) {
        int type= curElement->ops->type;
        if (curElement->legend) curLegend= curElement->legend;
        else if (levelCurve) {
          /* automatically generate level curve legend if not supplied */
          curLegend= levelLegend;
          sprintf(curLegend, "\001: %.4g", *levelValue);
        }
        if (curLegend) {
          nRemaining= strlen(curLegend);
          curWrap= 0;
          if ((type==GD_ELEM_LINES || type==GD_ELEM_CONTOURS) && curLegend[0]=='\001') {
            /* insert marker into GD_ELEM_LINES legend if so directed */
            curMarker= type==GD_ELEM_LINES? ((ge_lines_t *)curElement)->m.type :
            ((ge_contours_t *)curElement)->m.type;
            if (curMarker>=1 && curMarker<=5)
              curMarker= specialMarks[curMarker-1];
            else if (curMarker<' ' || curMarker>='\177')
              curMarker= ' ';
          }
        }
      }
      if (levelCurve) {
        do {
          levelValue++;
          levelGroup++;
          nLevels--;
        } while (nLevels && !levelGroup[0]);
        if (nLevels>0) curElement= (gd_element_t *)levelGroup[0];
        else curElement= 0;
      } else {
        curElement= curElement->next;
        if (curElement==cur0Element) curElement= 0;
      }
      if (curLegend) return 1;
    }
  } while (NextRing());
  return 0;
}

static int BuildLegends(int more, int contours, ge_system_t *systems,
                        gd_element_t *elements, ge_legend_box_t *lbox)
{
  int firstLine= 1;
  int nlines= lbox->nlines;
  int nchars= lbox->nchars;
  int nwrap= lbox->nwrap;
  int nc;

  lenLegends= 0;
  if (!more) {
    if (nlines<=0 || nchars<=0) return 0;
    InitLegends(contours, systems, elements, (nchars+1)*nlines);
    if (!legendText) return 0;
  }

  for ( ; ; nlines--) {
    if (!curLegend && !NextLegend()) { more= 0;   break; }
    if (nlines<=0) { more= !more;   break; }
    if (firstLine) firstLine= 0;
    else legendText[lenLegends++]= '\n';
    nc= nRemaining>nchars? nchars : nRemaining;
    strncpy(legendText+lenLegends, curLegend, nc);
    if (curMarker) {
      legendText[lenLegends]= (char)curMarker;
      curMarker= 0;
    }
    lenLegends+= nc;
    nRemaining-= nc;
    if (nRemaining>0 && curWrap++<nwrap) curLegend+= nc;
    else { curLegend= 0; curMarker= 0; }
  }

  legendText[lenLegends]= '\0';
  return more;
}

int gd_draw_legends(gp_engine_t *engine)
{
  gp_real_t x, y;
  int type, more;
  ge_legend_box_t *lbox;
  if (!currentDr) return 1;

  if (engine) gp_preempt(engine);

  for (type=0 ; type<2 ; type++) {
    lbox= &currentDr->legends[type];
    x= lbox->x;
    y= lbox->y;
    ga_attributes.t= lbox->textStyle;
    gp_set_transform(&unitTrans);
    gp_clipping= 0;
    if (lbox->nlines <= 0) continue;
    for (more=0 ; ; ) {
      more= BuildLegends(more, type, currentDr->systems, currentDr->elements,
                         lbox);
      if (!legendText) {
        /* memory error */
        if (engine) gp_preempt(0);
        return 1;
      }
      if (lenLegends>0) gp_draw_text(x, y, legendText);
      if (!more || (lbox->dx==0.0 && lbox->dy==0.0)) break;
      x+= lbox->dx;
      y+= lbox->dy;
    }
  }

  if (engine) gp_preempt(0);
  return 0;
}

/* ------------------------------------------------------------------------ */
/* Utility routines */

static int MemoryError(void)
{
  if (currentDr)
    strcpy(gp_error, "memory manager failed in Gd function");
  else
    strcpy(gp_error, "currentDr not set in Gd function");
  return -1;
}

static void *Copy1(const void *orig, long size)
{
  void *px;
  if (size<=0) return 0;
  px= pl_malloc(size);
  if (!px) MemoryError();
  else if (orig) memcpy(px, orig, size);
  return px;
}

static void *Copy2(void *x1, const void *orig1, const void *orig2, long size)
{
  void *x2, **x1p= (void **)x1;
  *x1p= Copy1(orig1, size);
  if (!*x1p) return 0;
  x2= Copy1(orig2, size);
  if (!x2) { pl_free(*x1p);  *x1p= 0; }
  return x2;
}

void Gd_ScanZ(long n, const gp_real_t *z, gp_real_t *zmin, gp_real_t *zmax)
{
  long i;
  gp_real_t zn, zx;
  zn= zx= z[0];
  for (i=1 ; i<n ; i++) {
    if (z[i]<zn) zn= z[i];
    else if (z[i]>zx) zx= z[i];
  }
  *zmin= zn;
  *zmax= zx;
}

static void ScanXY(long n, const gp_real_t *x, const gp_real_t *y, gp_box_t *extrema)
{
  Gd_ScanZ(n, x, &extrema->xmin, &extrema->xmax);
  Gd_ScanZ(n, y, &extrema->ymin, &extrema->ymax);
}

void ge_add_element(int type, gd_element_t *element)
{
  gd_element_t *old;
  gd_drawing_t *drawing= currentDr;
  ge_system_t *sys;

  if (drawing->cleared==1) ClearDrawing(drawing);
  sys= currentSy;

  old= sys? sys->elements : drawing->elements;
  if (!old) {  /* this is first element */
    if (sys) sys->elements= element;
    else drawing->elements= element;
    element->prev= element->next= element;
  } else {     /* insert element at end of ring */
    element->prev= old->prev;
    element->next= old;
    old->prev= element->prev->next= element;
  }
  element->ops= opTables + type;
  element->hidden= gd_properties.hidden;
  if (gd_properties.legend) {
    element->legend= Copy1(gd_properties.legend, strlen(gd_properties.legend)+1);
    /* sigh. ignore memory error here */
  } else {
    element->legend= 0;
  }
  element->number= drawing->nElements++;
  /* System nust always have number of its largest element for
     _gd_begin_sy to work properly */
  if (sys) sys->el.number= element->number;
  else Damage((ge_system_t *)0, element);
}

void Gd_NextMeshBlock(long *ii, long *jj, long len, long iMax,
                      int *reg, int region)
{   /* Find next contiguous run of mesh points in given region */
  long i= *ii;
  long j= *jj;
  if (region==0) {
    for (j=i ; j<len ; j++)
      if (reg[j] || reg[j+1] || reg[j+iMax] || reg[j+iMax+1]) break;
    i= j;
    for (j=i+1 ; j<len ; j++)
      if (!reg[j] && !reg[j+1] && !reg[j+iMax] && !reg[j+iMax+1]) break;
  } else {
    for (j=i ; j<len ; j++)
      if (reg[j]==region || reg[j+1]==region ||
          reg[j+iMax]==region || reg[j+iMax+1]==region) break;
    i= j;
    for (j=i+1 ; j<len ; j++)
      if (reg[j]!=region && reg[j+1]!=region &&
          reg[j+iMax]!=region && reg[j+iMax+1]!=region) break;
  }
  *ii= i;
  *jj= j;
}

long ge_get_mesh(int noCopy, ga_mesh_t *meshin, int region, void *vMeshEl)
{
  ge_mesh_t *meshEl= vMeshEl;
  ga_mesh_t *mesh= &meshEl->mesh;
  gp_box_t *linBox= &meshEl->linBox;
  long iMax, jMax, i, j, len;
  int *reg;

  if (currentDr->cleared==1) ClearDrawing(currentDr);

  /* retrieve mesh shape from meshin */
  mesh->iMax= iMax= meshin->iMax;
  mesh->jMax= jMax= meshin->jMax;
  len= iMax*jMax;

  mesh->reg= 0;       /* set up for possible error return */
  mesh->triangle= 0;  /* only needed by gd_add_contours, so handled there */

  meshEl->xlog= meshEl->ylog= 0;
  meshEl->region= region;

  meshEl->noCopy= noCopy & (GD_NOCOPY_MESH|GD_NOCOPY_COLORS|GD_NOCOPY_UV|GD_NOCOPY_Z);
  if (noCopy&GD_NOCOPY_MESH) {
    /* Just copy pointers to mesh arrays -- NOCOPY also means not
       to free the pointer later */
    mesh->x= meshin->x;
    mesh->y= meshin->y;
  } else {
    /* Copy the mesh arrays themselves */
    mesh->y= Copy2(&mesh->x, meshin->x, meshin->y, sizeof(gp_real_t)*len);
    if (!mesh->y) { pl_free(vMeshEl);  return 0; }
  }

  if ((noCopy&GD_NOCOPY_MESH) && meshin->reg) {
    mesh->reg= reg= meshin->reg;
    meshEl->noCopy|= GD_NOCOPY_REG;
  } else {
    mesh->reg= reg= Copy1(meshin->reg, sizeof(int)*(len+iMax+1));
    if (!reg) { Gd_KillMeshXY(vMeshEl);  pl_free(vMeshEl);  return 0; }
  }

  /* Be sure region array is legal */
  for (i=0 ; i<iMax ; i++) reg[i]= 0;
  if (!meshin->reg) for (i=iMax ; i<len ; i++) reg[i]= 1;
  for (i=len ; i<len+iMax+1 ; i++) reg[i]= 0;
  for (i=0 ; i<len ; i+=iMax) reg[i]= 0;

  /* Scan mesh for extreme values */
  if (meshin->reg) {
    gp_box_t box;
    int first= 1;

    /* Use ScanXY on the longest contiguous run(s) of points */
    for (i=0 ; i<len ; ) {
      Gd_NextMeshBlock(&i, &j, len, iMax, reg, region);
      if (i>=len) break;
      ScanXY(j-i, mesh->x+i, mesh->y+i, &box);
      if (first) { *linBox= box;  first= 0; }
      else gp_swallow(linBox, &box);
      i= j+1;
    }
    if (first)
      linBox->xmin= linBox->xmax= linBox->ymin= linBox->ymax= 0.0;

  } else {
    ScanXY(len, mesh->x, mesh->y, linBox);
  }
  if (!currentSy) meshEl->el.box= *linBox;  /* for ge_add_element */

  /* copy mesh properties to gd_properties */
  Gd_MeshXYGet(vMeshEl);

  return len;
}

void ge_mark_for_scan(gd_element_t *el, gp_box_t *linBox)
{
  if (currentSy) {
    if (currentSy->unscanned<0) currentSy->unscanned= el->number;
  } else {
    el->box= *linBox;
  }
}

static int AutoMarker(ga_line_attribs_t *dl, int number)
{
  int p;
  p= (number+3)%4;
  if (number>=26) number%= 26;
  dl->mPhase= 0.25*(0.5+p)*dl->mSpace;
  dl->rPhase= 0.25*(0.5+p)*dl->rSpace;
  return 'A'+number;
}

/* ------------------------------------------------------------------------ */
/* Constructors for drawing elements are public routines declared in gist2.h */

int gd_add_lines(long n, const gp_real_t *px, const gp_real_t *py)
{
  ge_lines_t *el;
  if (n<=0) return -1;
  el= currentDr? pl_malloc(sizeof(ge_lines_t)) : 0;
  if (!el) return MemoryError();
  el->xlog= el->ylog= 0;

  /* make private copies of x and y arrays */
  el->y= Copy2(&el->x, px, py, sizeof(gp_real_t)*n);
  if (!el->y) { pl_free(el);  return -1; }
  el->n= n;

  /* scan for min and max of x and y arrays */
  ScanXY(n, px, py, &el->linBox);
  if (!currentSy) el->el.box= el->linBox;  /* for ge_add_element */

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->l= ga_attributes.l;
  el->dl= ga_attributes.dl;
  el->m= ga_attributes.m;

  /* set base class members */
  ge_add_element(GD_ELEM_LINES, &el->el);
  if (ga_attributes.m.type==0) el->m.type= AutoMarker(&el->dl, el->el.number);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.n= n;
  gd_properties.x= el->x;
  gd_properties.y= el->y;

  return el->el.number;
}

int gd_add_disjoint(long n, const gp_real_t *px, const gp_real_t *py,
               const gp_real_t *qx, const gp_real_t *qy)
{
  ge_disjoint_t *el;
  gp_box_t box;
  if (n<=0) return -1;
  el= currentDr? pl_malloc(sizeof(ge_disjoint_t)) : 0;
  if (!el) return MemoryError();
  el->el.next= el->el.prev= 0;
  el->xlog= el->ylog= el->xqlog= el->yqlog= 0;

  /* make private copies of x, y, xq, yq arrays */
  el->y= Copy2(&el->x, px, py, sizeof(gp_real_t)*n);
  if (!el->y) { pl_free(el);  return -1; }
  el->yq= Copy2(&el->xq, qx, qy, sizeof(gp_real_t)*n);
  if (!el->yq) { DisjointKill(el);  return -1; }
  el->n= n;

  /* scan for min and max of x and y arrays */
  ScanXY(n, px, py, &box);
  ScanXY(n, qx, qy, &el->linBox);
  gp_swallow(&el->linBox, &box);
  if (!currentSy) el->el.box= el->linBox;  /* for ge_add_element */

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->l= ga_attributes.l;

  /* set base class members */
  ge_add_element(GD_ELEM_DISJOINT, &el->el);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.n= n;
  gd_properties.x= el->x;
  gd_properties.y= el->y;
  gd_properties.xq= el->xq;
  gd_properties.yq= el->yq;

  return el->el.number;
}

int gd_add_text(gp_real_t x0, gp_real_t y0, const char *text, int toSys)
{
  ge_text_t *el= currentDr? pl_malloc(sizeof(ge_text_t)) : 0;
  ge_system_t *sys= currentSy;
  if (!el) return MemoryError();

  /* make private copy of text string */
  el->text= Copy1(text, strlen(text)+1);
  if (!el->text) { pl_free(el);  return -1; }
  el->x0= x0;
  el->y0= y0;

  /* Without some sort of common font metric, there is no way to
     know the box associated with text.  Even with such a metric,
     the box would have to change with the coordinate transform,
     unlike any other element.  For now, punt.  */
  el->el.box.xmin= el->el.box.xmax= x0;
  el->el.box.ymin= el->el.box.ymax= y0;

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->t= ga_attributes.t;

  /* set base class members */
  if (currentDr->cleared==1) ClearDrawing(currentDr); /* else currentSy */
  if (!toSys) currentSy= 0;                      /* can be clobbered... */
  ge_add_element(GD_ELEM_TEXT, &el->el);
  if (currentSy && currentSy->unscanned<0)
    currentSy->unscanned= el->el.number;
  if (!toSys) currentSy= sys;

  /* copy properties to gd_properties */
  gd_properties.x0= el->x0;
  gd_properties.y0= el->y0;
  gd_properties.text= el->text;

  return el->el.number;
}

int gd_add_cells(gp_real_t px, gp_real_t py, gp_real_t qx, gp_real_t qy,
            long width, long height, long nColumns, const gp_color_t *colors)
{
  gp_real_t x[2], y[2];
  gp_box_t linBox;
  long ncells= width*height;
  long len= sizeof(gp_color_t)*ncells;
  ge_cells_t *el= currentDr? pl_malloc(sizeof(ge_cells_t)) : 0;
  if (!el) return MemoryError();

  /* make private copy of colors array */
  el->rgb = ga_attributes.rgb;
  if (ga_attributes.rgb) len *= 3;
  ga_attributes.rgb = 0;
  el->colors= pl_malloc(len);
  if (!el->colors) { pl_free(el); return MemoryError(); }
  el->px= x[0]= px;
  el->py= y[0]= py;
  el->qx= x[1]= qx;
  el->qy= y[1]= qy;
  el->width= width;
  el->height= height;
  if (nColumns==width) {
    memcpy(el->colors, colors, len);
  } else {
    gp_color_t *newcols= el->colors;
    long i, rowSize= sizeof(gp_color_t)*width;
    for (i=0 ; i<height ; i++) {
      memcpy(newcols, colors, rowSize);
      newcols+= width;
      colors+= nColumns;
    }
  }
  ScanXY(2L, x, y, &linBox);
  if (!currentSy) el->el.box= linBox;  /* for ge_add_element */

  /* set base class members */
  ge_add_element(GD_ELEM_CELLS, &el->el);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &linBox);

  /* copy properties to gd_properties */
  gd_properties.px= el->px;
  gd_properties.py= el->py;
  gd_properties.qx= el->qx;
  gd_properties.qy= el->qy;
  gd_properties.width= el->width;
  gd_properties.height= el->height;
  gd_properties.colors= el->colors;

  return el->el.number;
}

int gd_add_fill(long n, const gp_color_t *colors, const gp_real_t *px,
           const gp_real_t *py, const long *pn)
{
  ge_polys_t *el;
  long i, ntot;
  if (n<=0) return -1;
  el= currentDr? pl_malloc(sizeof(ge_polys_t)) : 0;
  if (!el) return MemoryError();
  el->xlog= el->ylog= 0;

  /* make private copy of colors array */
  if (colors) {
    long ncol = (ga_attributes.rgb? 3*n : n);
    el->rgb = ga_attributes.rgb;
    el->colors= pl_malloc(ncol);
    if (!el->colors) { pl_free(el); return MemoryError(); }
    memcpy(el->colors, colors, ncol);
  } else {
    el->rgb = 0;
    el->colors= 0;
  }
  ga_attributes.rgb = 0;

  /* make private copy of lengths array */
  el->pn= pl_malloc(sizeof(long)*n);
  if (!el->pn) { pl_free(el->colors); pl_free(el); return MemoryError(); }
  for (ntot=i=0 ; i<n ; i++) {
    el->pn[i]= pn[i];
    ntot+= pn[i];
  }

  /* make private copies of x and y arrays */
  el->y= Copy2(&el->x, px, py, sizeof(gp_real_t)*ntot);
  if (!el->y) { pl_free(el->pn); pl_free(el->colors); pl_free(el);  return -1; }
  el->n= n;

  /* scan for min and max of x and y arrays */
  if (n<2 || pn[1]>1) ScanXY(ntot, px, py, &el->linBox);
  else ScanXY(ntot-pn[0], px+pn[0], py+pn[0], &el->linBox);
  if (!currentSy) el->el.box= el->linBox;  /* for ge_add_element */

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->e= ga_attributes.e;  /* for edges */

  /* set base class members */
  ge_add_element(GD_ELEM_POLYS, &el->el);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.n= n;
  gd_properties.x= el->x;
  gd_properties.y= el->y;
  gd_properties.pn= el->pn;
  gd_properties.colors= el->colors;

  return el->el.number;
}

int gd_add_mesh(int noCopy, ga_mesh_t *mesh, int region, int boundary,
           int inhibit)
{
  long len;
  ge_mesh_t *el= currentDr? pl_malloc(sizeof(ge_mesh_t)) : 0;
  if (!el) return MemoryError();
  el->el.next= el->el.prev= 0;

  /* get mesh */
  len= ge_get_mesh(noCopy, mesh, region, el);
  if (!len) return -1;
  el->boundary= boundary;
  el->inhibit= inhibit;

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->l= ga_attributes.l;

  /* set base class members */
  ge_add_element(GD_ELEM_MESH, &el->el);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.boundary= el->boundary;
  gd_properties.inhibit= el->inhibit;

  return el->el.number;
}

int gd_add_fillmesh(int noCopy, ga_mesh_t *mesh, int region,
               gp_color_t *colors, long nColumns)
{
  long len;
  ge_fill_t *el= currentDr? pl_malloc(sizeof(ge_fill_t)) : 0;
  if (!el) return MemoryError();
  el->el.next= el->el.prev= 0;

  /* get mesh */
  len= ge_get_mesh(noCopy, mesh, region, el);
  if (!len) return -1;

  /* make private copy of colors array */
  el->rgb = ga_attributes.rgb;
  if (noCopy&GD_NOCOPY_COLORS || !colors) {
    el->colors= colors;
  } else {
    long iMax1= mesh->iMax-1;
    long len1= len - mesh->jMax - iMax1;
    int rgb = ga_attributes.rgb;
    if (rgb) len1 *= 3;
    el->colors= Copy1(nColumns==iMax1?colors:0, sizeof(gp_color_t)*len1);
    if (!el->colors) { FilledKill(el);  return -1; }
    if (nColumns!=iMax1) {
      long i, j=0, k=0;
      for (i=0 ; i<len1 ; i++) {
        if (rgb) {
          el->colors[i++]= colors[3*(j+k)];
          el->colors[i++]= colors[3*(j+k)+1];
          el->colors[i]= colors[3*(j+k)+2];
        } else {
          el->colors[i]= colors[j+k];
        }
        j++;
        if (j==iMax1) { k+= nColumns; j= 0; }
      }
      nColumns= iMax1;
    }
  }
  ga_attributes.rgb = 0;
  el->nColumns= nColumns;

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->e= ga_attributes.e;  /* for edges */

  /* set base class members */
  ge_add_element(GD_ELEM_FILLED, &el->el);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.nColumns= nColumns;
  gd_properties.colors= el->colors;

  return el->el.number;
}

int gd_add_vectors(int noCopy, ga_mesh_t *mesh, int region,
              gp_real_t *u, gp_real_t *v, gp_real_t scale)
{
  long len;
  ge_vectors_t *el= currentDr? pl_malloc(sizeof(ge_vectors_t)) : 0;
  if (!el) return MemoryError();
  el->el.next= el->el.prev= 0;

  /* get mesh */
  len= ge_get_mesh(noCopy, mesh, region, el);
  if (!len) return -1;

  /* make private copy of (u,v) arrays */
  if (noCopy&GD_NOCOPY_UV) {
    el->u= u;
    el->v= v;
  } else {
    el->v= Copy2(&el->u, u, v, sizeof(gp_real_t)*len);
    if (!el->v) { VectorsKill(el);  return -1; }
  }
  el->scale= scale;

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->l= ga_attributes.l;
  el->f= ga_attributes.f;
  el->vect= ga_attributes.vect;

  /* set base class members */
  ge_add_element(GD_ELEM_VECTORS, &el->el);

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.u= el->u;
  gd_properties.v= el->v;
  gd_properties.scale= el->scale;

  return el->el.number;
}

int Gd_MakeContours(ge_contours_t *con)
{
  long n;
  gp_real_t dphase, *px, *py;
  int i;
  ge_lines_t *group;
  ge_lines_t *el, *prev;
  int marker;

  /* Generic properties copied to all contours (m.type reset below)  */
  ga_attributes.l= con->l;
  ga_attributes.dl= con->dl;
  ga_attributes.m= con->m;
  marker= ga_attributes.m.type>32? ga_attributes.m.type : 'A';
  dphase= 0.25*con->dl.mSpace;

  for (i=0 ; i<con->nLevels ; i++) con->groups[i]= 0;

  for (i=0 ; i<con->nLevels ; i++) {
    ga_attributes.m.type= marker++;
    if (marker=='Z'+1 || marker=='z'+1) marker= 'A';
    group= prev= 0;
    if (ga_init_contour(&con->mesh, con->region, con->z, con->levels[i])) {
      while (ga_draw_contour(&n, &px, &py, &ga_attributes.dl.closed)) {
        el= currentDr? pl_malloc(sizeof(ge_lines_t)) : 0;
        if (!el) return MemoryError();

        /* make private copies of x and y arrays */
        el->y= Copy2(&el->x, px, py, sizeof(gp_real_t)*n);
        if (!el->y) { pl_free(el);  return -1; }
        el->n= n;
        el->xlog= el->ylog= 0;

        /* scan for min and max of x and y arrays */
        ScanXY(n, px, py, &el->linBox);
        if (!currentSy) el->el.box= el->linBox;  /* for ge_add_element */

        el->el.ops= opTables + GD_ELEM_LINES;
        el->el.hidden= 0;
        el->el.legend= 0;
        el->el.box= el->linBox;

        /* GeContour number always matches number of largest element
           for _gd_begin_sy to work properly */
        el->el.number= con->el.number= currentDr->nElements++;

        if (prev) {
          prev->el.next= group->el.prev= &el->el;
          el->el.prev= &prev->el;
          el->el.next= &group->el;
        } else {
          con->groups[i]= group= el;
          el->el.next= el->el.prev= &el->el;
        }
        prev= el;

        el->l= ga_attributes.l;
        el->dl= ga_attributes.dl;
        el->m= ga_attributes.m;

        ga_attributes.dl.mPhase+= dphase;
        if (ga_attributes.dl.mPhase>ga_attributes.dl.mSpace)
          ga_attributes.dl.mPhase-= ga_attributes.dl.mSpace;
      }
    }
  }

  return 0;
}

int gd_add_contours(int noCopy, ga_mesh_t *mesh, int region,
               gp_real_t *z, const gp_real_t *levels, int nLevels)
{
  long len;
  ge_contours_t *el= currentDr? pl_malloc(sizeof(ge_contours_t)) : 0;
  if (!el) return MemoryError();
  el->el.next= el->el.prev= 0;
  el->z= el->levels= 0;
  el->groups= 0;

  /* get mesh */
  len= ge_get_mesh(noCopy, mesh, region, el);
  if (!len) return -1;

  /* make private copy of z and levels arrays */
  if (noCopy&GD_NOCOPY_Z) {
    el->z= z;
  } else {
    el->z= Copy1(z, sizeof(gp_real_t)*len);
    if (!el->z) { ContoursKill(el);  return -1; }
  }

  /* create triangle array now if necessary */
  if (noCopy&GD_NOCOPY_MESH && mesh->triangle) {
    el->mesh.triangle= mesh->triangle;
    el->noCopy|= GD_NOCOPY_TRI;
    gd_properties.noCopy|= GD_NOCOPY_TRI;
  } else {
    el->mesh.triangle= Copy1(mesh->triangle, sizeof(short)*len);
    if (!el->mesh.triangle) { ContoursKill(el);  return -1; }
  }

  /* copy relevant attributes from ga_attributes
     -- This must be done BEFORE ge_add_element, since the damage
        calculation depends on the Margins! */
  el->l= ga_attributes.l;
  el->dl= ga_attributes.dl;
  el->m= ga_attributes.m;

  /* set base class members */
  ge_add_element(GD_ELEM_CONTOURS, &el->el);
  if (ga_attributes.m.type==0) el->m.type= AutoMarker(&el->dl, el->el.number);

  el->nLevels= nLevels;
  if (nLevels>0) {
    el->levels= Copy1(levels, sizeof(gp_real_t)*nLevels);
    if (!el->levels) { ContoursKill(el);  return -1; }
    el->groups= (ge_lines_t **)pl_malloc(sizeof(ge_lines_t *)*nLevels);
    if (!el->groups || Gd_MakeContours(el)) { ContoursKill(el);  return -1; }
  } else {
    nLevels= 0;
    el->levels= 0;
    el->groups= 0;
  }

  /* current box not set, mark as unscanned if in system */
  ge_mark_for_scan(&el->el, &el->linBox);

  /* copy properties to gd_properties */
  gd_properties.z= el->z;
  gd_properties.nLevels= el->nLevels;
  gd_properties.levels= el->levels;

  return el->el.number;
}

/* ------------------------------------------------------------------------ */

static void GuessBox(gp_box_t *box, gp_box_t *viewport, ga_tick_style_t *ticks)
{
  gp_real_t dxmin= 0.0, xmin= viewport->xmin;
  gp_real_t dxmax= 0.0, xmax= viewport->xmax;
  gp_real_t dymin= 0.0, ymin= viewport->ymin;
  gp_real_t dymax= 0.0, ymax= viewport->ymax;
  int vf= ticks->vert.flags;
  int hf= ticks->horiz.flags;
  gp_real_t vlen= ((((vf&GA_TICK_IN)&&(vf&GA_TICK_OUT))||(vf&GA_TICK_C))? 0.5 : 1.0) *
    ticks->vert.tickLen[0];
  gp_real_t hlen= ((((vf&GA_TICK_IN)&&(vf&GA_TICK_OUT))||(vf&GA_TICK_C))? 0.5 : 1.0) *
    ticks->horiz.tickLen[0];
  gp_real_t cy= ticks->horiz.textStyle.height;
  gp_real_t cx= ticks->vert.textStyle.height*0.6;  /* guess at char width */
  gp_real_t hx= cy*0.6;                            /* guess at char width */
  gp_box_t overflow;

  /* Note-- extra 0.4 for nudged log decades (see DrawXLabels, tick.c) */
  cx*= ticks->vert.nDigits+2.4;         /* largest width of y label */
  hx*= 0.5*(ticks->horiz.nDigits+2.4);  /* maximum distance x label
                                           can project past xmin, xmax */

  if (((vf&GA_TICK_L)&&(vf&GA_TICK_OUT)) || (vf&GA_TICK_C))
    dxmin= ticks->vert.tickOff+vlen;
  if (((vf&GA_TICK_U)&&(vf&GA_TICK_OUT)) || (vf&GA_TICK_C))
    dxmax= ticks->vert.tickOff+vlen;
  if (((hf&GA_TICK_L)&&(hf&GA_TICK_OUT)) || (hf&GA_TICK_C))
    dymin= ticks->horiz.tickOff+hlen;
  if (((hf&GA_TICK_U)&&(hf&GA_TICK_OUT)) || (hf&GA_TICK_C))
    dymax= ticks->horiz.tickOff+hlen;

  if (vf & GA_LABEL_L) xmin-= ticks->vert.labelOff+cx;
  else if ((hf&(GA_LABEL_L|GA_LABEL_U)) && hx>dxmin) xmin-= hx;
  else xmin-= dxmin;
  if (vf & GA_LABEL_U) xmax+= ticks->vert.labelOff+cx;
  else if ((hf&(GA_LABEL_L|GA_LABEL_U)) && hx>dxmax) xmax+= hx;
  else xmax+= dxmax;

  if (hf & GA_LABEL_L) ymin-= ticks->horiz.labelOff+2.0*cy;
  else if ((vf&(GA_LABEL_L|GA_LABEL_U)) && 0.5*cy>dymin) ymin-= 0.5*cy;
  else ymin-= dymin;
  if (hf & GA_LABEL_U) xmax+= ticks->horiz.labelOff+2.0*cy;
  else if ((vf&(GA_LABEL_L|GA_LABEL_U)) && 0.5*cy>dymax) ymax+= 0.5*cy;
  else ymax+= dymax;

  if (vf & (GA_TICK_L|GA_TICK_U)) {
    xmin-= 0.5*ticks->vert.tickStyle.width*GP_DEFAULT_LINE_WIDTH;
    xmax+= 0.5*ticks->vert.tickStyle.width*GP_DEFAULT_LINE_WIDTH;
  }
  if (hf & (GA_TICK_L|GA_TICK_U)) {
    ymin-= 0.5*ticks->horiz.tickStyle.width*GP_DEFAULT_LINE_WIDTH;
    ymax+= 0.5*ticks->horiz.tickStyle.width*GP_DEFAULT_LINE_WIDTH;
  }

  box->xmin= xmin;
  box->xmax= xmax;
  box->ymin= ymin;
  box->ymax= ymax;

  /* Finally, swallow overflow boxes, assuming 22 characters max */
  overflow.xmin= ticks->horiz.xOver;
  overflow.ymin= ticks->horiz.yOver-ticks->horiz.textStyle.height*0.2;
  overflow.xmax= overflow.xmin+ticks->horiz.textStyle.height*(0.6*22.0);
  overflow.ymax= overflow.ymin+ticks->horiz.textStyle.height;
  gp_swallow(box, &overflow);
  overflow.xmin= ticks->vert.xOver;
  overflow.ymin= ticks->vert.yOver-ticks->vert.textStyle.height*0.2;
  overflow.xmax= overflow.xmin+ticks->vert.textStyle.height*(0.6*22.0);
  overflow.ymax= overflow.ymin+ticks->vert.textStyle.height;
  gp_swallow(box, &overflow);
}

int gd_new_system(gp_box_t *viewport, ga_tick_style_t *ticks)
{
  ge_system_t *sys;
  int sysIndex;

  if (!currentDr) return -1;

  /* Adding a new system clears the drawing */
  if (currentDr->cleared!=2) ClearDrawing(currentDr);
  sysIndex= currentDr->nSystems+1;

  sys= pl_malloc(sizeof(ge_system_t));
  if (!sys) return -1;
  sys->el.ops= opTables + GD_ELEM_SYSTEM;
  if (gd_properties.legend) {
    sys->el.legend= Copy1(gd_properties.legend, strlen(gd_properties.legend)+1);
    if (!sys->el.legend) { pl_free(sys);  return -1; }
  } else sys->el.legend= 0;
  sys->el.hidden= gd_properties.hidden;

  if (sysIndex>1) {
    gd_element_t *prev= currentDr->systems->el.prev;
    prev->next= &sys->el;
    sys->el.prev= prev;
    sys->el.next= &currentDr->systems->el;
    currentDr->systems->el.prev= &sys->el;
  } else {
    sys->el.prev= sys->el.next= &sys->el;
    currentDr->systems= sys;
  }
  sys->el.number= -1;
  currentDr->nSystems++;
  sys->rescan= 0;
  sys->unscanned= -1;

  GuessBox(&sys->el.box, viewport, ticks);

  if (viewport->xmin<viewport->xmax) {
    sys->trans.viewport.xmin= viewport->xmin;
    sys->trans.viewport.xmax= viewport->xmax;
  } else {
    sys->trans.viewport.xmin= viewport->xmax;
    sys->trans.viewport.xmax= viewport->xmin;
  }
  if (viewport->ymin<viewport->ymax) {
    sys->trans.viewport.ymin= viewport->ymin;
    sys->trans.viewport.ymax= viewport->ymax;
  } else {
    sys->trans.viewport.ymin= viewport->ymax;
    sys->trans.viewport.ymax= viewport->ymin;
  }
  sys->trans.window.xmin= sys->trans.window.ymin= 0.0;
  sys->trans.window.xmax= sys->trans.window.ymax= 1.0;
  sys->ticks= *ticks;
  sys->flags= GD_LIM_XMIN | GD_LIM_XMAX | GD_LIM_YMIN | GD_LIM_YMAX;
  sys->elements= 0;
  sys->savedWindow.xmin= sys->savedWindow.ymin= 0.0;
  sys->savedWindow.xmax= sys->savedWindow.ymax= 1.0;
  sys->savedFlags= GD_LIM_XMIN | GD_LIM_XMAX | GD_LIM_YMIN | GD_LIM_YMAX;

  sys->xtick= sys->ytick= 0;
  sys->xlabel= sys->ylabel= 0;

  gd_set_system(sysIndex);
  return sysIndex;
}

int gd_get_system(void)
{
  gd_element_t *sys, *sys0;
  int sysIndex= 0;
  if (!currentDr) return -1;
  if (!currentDr->systems || !currentSy) return 0;

  /* ClearDrawing sets currentSy-- must not be pending */
  if (currentDr->cleared==1) ClearDrawing(currentDr);

  sys= sys0= (gd_element_t *)currentDr->systems;
  for (sysIndex=1 ; sys!=&currentSy->el ; sysIndex++) {
    sys= sys->next;
    if (sys==sys0) return -2;
  }

  return sysIndex;
}

int gd_set_system(int sysIndex)
{
  ge_system_t *sys;
  gd_element_t *sys0;
  if (!currentDr || !currentDr->systems) return GD_ELEM_NONE;

  /* ClearDrawing sets currentSy-- must not be pending */
  if (currentDr->cleared==1) ClearDrawing(currentDr);

  currentEl= 0;
  currentCn= -1;
  if (sysIndex<1) {  /* Set current system to none */
    currentSy= 0;
    gd_properties.trans.viewport.xmin= gd_properties.trans.viewport.xmax=
      gd_properties.trans.viewport.ymin= gd_properties.trans.viewport.ymax= 0.0;
    gd_properties.flags= 0;
    return GD_ELEM_NONE;
  }

  sys= currentDr->systems;
  sys0= &sys->el;
  while (--sysIndex && sys->el.next!=sys0)
    sys= (ge_system_t *)sys->el.next;
  if (sysIndex>0) return GD_ELEM_NONE;

  currentSy= sys;
  gd_properties.hidden= sys->el.hidden;
  gd_properties.legend= sys->el.legend;
  gd_properties.ticks= sys->ticks;
  gd_properties.trans.viewport= sys->trans.viewport;
  if (gd_get_limits()) {
    SystemKill(sys);
    return GD_ELEM_NONE;
  }
  return GD_ELEM_SYSTEM;
}

int gd_set_element(int elIndex)
{
  gd_element_t *el, *el0;
  if (!currentDr) return GD_ELEM_NONE;

  el= currentSy? currentSy->elements : currentDr->elements;

  if (elIndex<0 || !el) {   /* set current element to none */
    currentEl= 0;
    currentCn= -1;
    return GD_ELEM_NONE;
  }

  el0= el;
  while (elIndex-- && el->next!=el0) el= el->next;
  if (elIndex>=0) return GD_ELEM_NONE;

  currentEl= el;
  currentCn= -1;
  return el->ops->GetProps(el);
}

static gd_element_t *NextConCurve(gd_element_t *el)
{
  ge_contours_t *con= (ge_contours_t *)currentEl;
  gd_element_t *el0= &con->groups[currentCn]->el;
  if (!el) el= el0;
  else if (el->next==el0) el= 0;
  else el= el->next;
  return el;
}

int gd_set_contour(int levIndex)
{
  ge_contours_t *con;
  gd_element_t *el;
  if (!currentDr || !currentEl || currentEl->ops->type!=GD_ELEM_CONTOURS)
    return GD_ELEM_NONE;
  con= (ge_contours_t *)currentEl;
  if (levIndex>=con->nLevels) return GD_ELEM_NONE;
  if (levIndex<0) {
    currentCn= -1;
    return GD_ELEM_NONE;
  }
  currentCn= levIndex;
  el= NextConCurve((gd_element_t *)0);
  if (el) LinesGet(el);
  else ContoursGet(con);
  return GD_ELEM_LINES;
}

static int GeFindIndex(int id, ge_system_t *sys)
{
  int elIndex;
  gd_element_t *el, *el0;
  if (!currentDr) return -1;

  el= sys? sys->elements : currentDr->elements;
  if (!el) return -1;

  el0= el;
  elIndex= 0;
  while (el->number != id) {
    if (el->next==el0) return -1;
    el= el->next;
    elIndex++;
  }

  return elIndex;
}

int gd_find_index(int id)
{
  return GeFindIndex(id, currentSy);
}

int gd_find_system(int id)
{
  int sysIndex;
  ge_system_t *sys;
  gd_element_t *sys0;
  if (!currentDr) return -1;

  if (GeFindIndex(id, 0)>=0) return 0;
  sys= currentDr->systems;
  if (!sys) return -1;
  sys0= &sys->el;
  sysIndex= 1;
  while (GeFindIndex(id, sys)<0) {
    if (sys->el.next==sys0) return -1;
    sys= (ge_system_t *)sys->el.next;
    sysIndex++;
  }
  return sysIndex;
}

int gd_set_alt_ticker(ga_ticker_t *xtick, ga_labeler_t *xlabel,
              ga_ticker_t *ytick, ga_labeler_t *ylabel)
{
  if (!currentDr || !currentSy) return 1;
  if (xtick) currentSy->xtick= xtick;
  if (ytick) currentSy->ytick= ytick;
  if (xlabel) currentSy->xlabel= xlabel;
  if (ylabel) currentSy->ylabel= ylabel;
  return 0;
}

/* ------------------------------------------------------------------------ */

int gd_edit(int xyzChanged)
{
  gd_element_t *el= currentEl;
  if (!currentDr || !el) return 1;

  /* Changing linestyles or most other simple changes may incur damage
     in a way that is very difficult to anticipate, hence, must call
     Damage here.  On the other hand, the elements need to be rescanned
     only if GD_CHANGE_XY or GD_CHANGE_Z has been set, so only set rescan
     in this case.  If only linestyles and such have been changed, gd_scan
     will not call Damage again, although if they have it will-- hence, if
     you are changing both coordinates and linestyles, there is no way to
     avoid "double damage".  */
  Damage(currentSy, el);
  if (currentSy && xyzChanged) currentSy->rescan= 1;

  if (currentCn>=0) {
    el= NextConCurve((gd_element_t *)0);
    if (el) {
      /* legend only changes on first piece of contour */
      el->legend= gd_properties.legend;
      Gd_LinesSubSet(el);
      /* other line properties propagate to all contour pieces
         -- but NEVER attempt to change the (x,y) values */
      while ((el= NextConCurve(el))) Gd_LinesSubSet(el);
    }
    return 0;
  }
  return el->ops->SetProps(el, xyzChanged);
}

int gd_remove(void)
{
  gd_element_t *el= currentEl;
  if (!currentDr || !el || currentCn>=0) return 1;

  /* Damage alert must take place here-- unfortunately, if this remove
     changes extreme values, a second call to Damage will be made in
     gd_scan.  Hopefully, gd_remove is a rare enough operation that this
     inefficiency is negligible.  */
  Damage(currentSy, el);

  if (currentSy) {
    gd_element_t *prev= el->prev;
    if (el==prev) {
      currentSy->unscanned= -1;
      currentSy->rescan= 0;
      currentSy->el.number= -1;
    } else {
      if (el->number==currentSy->unscanned) {
        if (el->next != currentSy->elements)
          currentSy->unscanned= el->next->number;
        else currentSy->unscanned= -1;
      }
      if (el->number<currentSy->unscanned && !el->hidden)
        currentSy->rescan= 1;
      if (el->number==currentSy->el.number)
        currentSy->el.number= prev->number;
    }
  }

  if (currentSy && el==currentSy->elements) {
    if (el->next==el) currentSy->elements= 0;
    else currentSy->elements= el->next;
  } else if (el==currentDr->elements) {
    if (el->next==el) currentDr->elements= 0;
    else currentDr->elements= el->next;
  }

  el->ops->Kill(el);
  currentEl= 0;
  return 0;
}

int gd_get_limits(void)
{
  if (!currentDr || !currentSy) return 1;
  if ((currentSy->rescan || currentSy->unscanned>=0)
      && gd_scan(currentSy)) return 1;  /* memory manager failure */
  gd_properties.trans.window= currentSy->trans.window;
  gd_properties.flags= currentSy->flags;

  if (gd_properties.flags & GD_LIM_LOGX) {
    gd_properties.limits.xmin= exp10(gd_properties.trans.window.xmin);
    gd_properties.limits.xmax= exp10(gd_properties.trans.window.xmax);
  } else {
    gd_properties.limits.xmin= gd_properties.trans.window.xmin;
    gd_properties.limits.xmax= gd_properties.trans.window.xmax;
  }
  if (gd_properties.flags & GD_LIM_LOGY) {
    gd_properties.limits.ymin= exp10(gd_properties.trans.window.ymin);
    gd_properties.limits.ymax= exp10(gd_properties.trans.window.ymax);
  } else {
    gd_properties.limits.ymin= gd_properties.trans.window.ymin;
    gd_properties.limits.ymax= gd_properties.trans.window.ymax;
  }
  return 0;
}

int gd_set_limits(void)
{
  int flags, rescan;
  if (!currentDr || !currentSy) return 1;

  if (gd_properties.flags & GD_LIM_LOGX) {
    gd_properties.trans.window.xmin= SAFELOG(gd_properties.limits.xmin);
    gd_properties.trans.window.xmax= SAFELOG(gd_properties.limits.xmax);
  } else {
    gd_properties.trans.window.xmin= gd_properties.limits.xmin;
    gd_properties.trans.window.xmax= gd_properties.limits.xmax;
  }
  if (gd_properties.flags & GD_LIM_LOGY) {
    gd_properties.trans.window.ymin= SAFELOG(gd_properties.limits.ymin);
    gd_properties.trans.window.ymax= SAFELOG(gd_properties.limits.ymax);
  } else {
    gd_properties.trans.window.ymin= gd_properties.limits.ymin;
    gd_properties.trans.window.ymax= gd_properties.limits.ymax;
  }

  flags= currentSy->flags;
  currentSy->flags= gd_properties.flags;

  /* Normally, setting the limits damages the entire system.
     However, would like to allow a special case for fixing limits to
     their existing extreme values.  */
  rescan= 1;
  if ( ! ( (flags^gd_properties.flags) & (~(GD_LIM_XMIN|GD_LIM_XMAX|GD_LIM_YMIN|GD_LIM_YMAX)) ) ) {
    if (((flags&GD_LIM_XMIN)==(gd_properties.flags&GD_LIM_XMIN) || (gd_properties.flags&GD_LIM_XMIN)==0) &&
        ((flags&GD_LIM_XMAX)==(gd_properties.flags&GD_LIM_XMAX) || (gd_properties.flags&GD_LIM_XMAX)==0) &&
        ((flags&GD_LIM_YMIN)==(gd_properties.flags&GD_LIM_YMIN) || (gd_properties.flags&GD_LIM_YMIN)==0) &&
        ((flags&GD_LIM_YMAX)==(gd_properties.flags&GD_LIM_YMAX) || (gd_properties.flags&GD_LIM_YMAX)==0)) {
      gp_box_t *w= &currentSy->trans.window;
      if (w->xmin==gd_properties.trans.window.xmin &&
          w->xmax==gd_properties.trans.window.xmax &&
          w->ymin==gd_properties.trans.window.ymin &&
          w->ymax==gd_properties.trans.window.ymax)
        rescan= 0;
    }
  }
  currentSy->trans.window= gd_properties.trans.window;
  currentSy->rescan|= rescan;

  /* damage alert takes place in gd_scan just before rendering */
  return 0;
}

int gd_save_limits(int resetZoomed)
{
  if (!currentDr || !currentSy) return 1;
  currentSy->savedWindow= currentSy->trans.window;
  currentSy->savedFlags= currentSy->flags;
  if (resetZoomed) currentSy->savedFlags&= ~GD_LIM_ZOOMED;
  return 0;
}

int gd_revert_limits(int ifZoomed)
{
  if (!currentDr || !currentSy ||
      (ifZoomed && !(currentSy->flags&GD_LIM_ZOOMED))) return 1;
  if (currentSy->savedFlags!=currentSy->flags ||
      currentSy->savedWindow.xmin!=currentSy->trans.window.xmin ||
      currentSy->savedWindow.xmax!=currentSy->trans.window.xmax ||
      currentSy->savedWindow.ymin!=currentSy->trans.window.ymin ||
      currentSy->savedWindow.ymax!=currentSy->trans.window.ymax) {
    currentSy->trans.window= currentSy->savedWindow;
    currentSy->flags= currentSy->savedFlags;
    currentSy->rescan= 1;
  }
  return 0;
}

int gd_set_viewport(void)
{
  gp_box_t *v, oldBox;
  if (!currentDr || !currentSy) return 1;

  currentSy->el.hidden= gd_properties.hidden;
  /*
  if (gd_properties.legend) {
    currentSy->el.legend= Copy1(gd_properties.legend, strlen(gd_properties.legend)+1);
  }
  */

  /* First, damage current coordinate system box.  */
  Damage(currentSy, (gd_element_t *)0);

  /* Save old box, set new ticks, viewport, and correponding box */
  oldBox= currentSy->el.box;
  v= &currentSy->trans.viewport;
  currentSy->ticks= gd_properties.ticks;
  *v= gd_properties.trans.viewport;
  GuessBox(&currentSy->el.box, &gd_properties.trans.viewport, &gd_properties.ticks);

  /* Since stacking order hasn't changed, new box must be damaged
     if it is not contained in the old box.  */
  v= &currentSy->el.box;
  if (v->xmin<oldBox.xmin || v->xmax>oldBox.xmax ||
      v->ymin<oldBox.ymin || v->ymax>oldBox.ymax)
    Damage(currentSy, (gd_element_t *)0);

  return 0;
}

gp_box_t *gd_clear_system(void)
{
  gp_box_t *dBox;
  int n, nel;
  ge_system_t *sys, *sys0;
  gd_element_t *el, *el0;
  /* Intended for use with animation... */
  if (!currentDr || !currentSy) return 0;

  Gd_KillRing(currentSy->elements);
  currentSy->elements= 0;
  currentSy->el.number= currentSy->unscanned= -1;
  currentSy->rescan= 0;

  sys0= currentDr->systems;
  el0= currentDr->elements;
  nel= -1;
  if ((sys= sys0)) do {
    if (sys==currentSy) continue;
    n= currentSy->el.number;
    if (n>nel) nel= n;
    sys= (ge_system_t *)sys->el.next;
  } while (sys!=sys0);
  if ((el= el0)) do {
    n= el->number;
    if (n>nel) nel= n;
    el= el->next;
  } while (el!=el0);
  currentDr->nElements= nel+1;

  if (currentSy->flags & (GD_LIM_XMIN|GD_LIM_XMAX|GD_LIM_YMIN|GD_LIM_YMAX)) {
    /* Some extreme value set, damage whole box */
    dBox= &currentSy->el.box;
    Damage(currentSy, (gd_element_t *)0);
  } else {
    /* All limits fixed, damage only viewport */
    dBox= &currentSy->trans.viewport;
    Damage(currentSy, &currentSy->el);
  }

  return dBox;
}

/* ------------------------------------------------------------------------ */
