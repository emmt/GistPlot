/*
 * $Id: engine.c,v 1.1 2005-09-18 22:04:19 dhmunro Exp $
 * Implement common properties of all GIST engines
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist2.h"
#include "gist/engine.h"
#include "gist/draw.h"
#include "play/std.h"

gp_engine_t *gp_engines= 0;
gp_engine_t *gp_active_engines= 0;
gp_engine_t *gp_preempted_engine= 0;

#include <string.h>

static void DefaultClearArea(gp_engine_t *engine, gp_box_t *box);
static void MoreScratch(long np, long ns);

/* ------------------------------------------------------------------------ */

/* ARGSUSED */
static void DefaultClearArea(gp_engine_t *engine, gp_box_t *box)
{
  /* Default ClearArea triggers complete redraw */
  engine->Clear(engine, GP_CONDITIONALLY);
  engine->lastDrawn= -1;
  engine->systemsSeen[0]= engine->systemsSeen[1]= 0;
  engine->damaged= engine->inhibit= 0;
}

gp_engine_t *gp_new_engine(long size, char *name, gp_callbacks_t *on,
                    gp_transform_t *transform, int landscape,
  void (*Kill)(gp_engine_t*), int (*Clear)(gp_engine_t*,int), int (*Flush)(gp_engine_t*),
  void (*ChangeMap)(gp_engine_t*), int (*ChangePalette)(gp_engine_t*),
  int (*DrawLines)(gp_engine_t*,long,const gp_real_t*,const gp_real_t*,int,int),
  int (*DrawMarkers)(gp_engine_t*,long,const gp_real_t*,const gp_real_t *),
  int (*DrwText)(gp_engine_t*e,gp_real_t,gp_real_t,const char*),
  int (*DrawFill)(gp_engine_t*,long,const gp_real_t*,const gp_real_t*),
  int (*DrawCells)(gp_engine_t*,gp_real_t,gp_real_t,gp_real_t,gp_real_t,
                   long,long,long,const gp_color_t*),
  int (*DrawDisjoint)(gp_engine_t*,long,const gp_real_t*,const gp_real_t*,
                      const gp_real_t*,const gp_real_t*))
{
  long lname= name? strlen(name) : 0;
  gp_engine_t *engine;
  /* For Electric Fence package and maybe others, it is nice to ensure
     that size of block allocated for gp_engine_t is a multiple of the size
     of the most restrictively aligned object which can be in any
     gp_engine_t; assume this is a double.  */
  lname= (lname/sizeof(double) + 1)*sizeof(double);  /* >= lname+1 */
  engine= (gp_engine_t *)pl_malloc(size+lname);
  if (!engine) return 0;

  /* Fill in gp_engine_t properties, link into gp_engines list */
  engine->next= gp_engines;
  gp_engines= engine;
  engine->nextActive= 0;
  engine->name= (char *)engine + size;
  strcpy(name? engine->name : "", name);
  engine->on = on;
  engine->active= 0;
  engine->marked= 0;

  engine->transform= *transform;
  engine->landscape= landscape? 1 : 0;
  gp_set_device_map(engine);
  /* (a proper map will be installed when the engine is activated) */
  engine->map.x.scale= engine->map.y.scale= 1.0;
  engine->map.x.offset= engine->map.y.offset= 0.0;

  /* No pseudocolor map initially */
  engine->colorChange= 0;
  engine->colorMode= 0;
  engine->nColors= 0;
  engine->palette= 0;

  /* No associated drawing initially */
  engine->drawing= 0;
  engine->lastDrawn= -1;
  engine->systemsSeen[0]= engine->systemsSeen[1]= 0;
  engine->inhibit= 0;
  engine->damaged= 0;  /* causes Clear if no ClearArea virtual function */
  engine->damage.xmin= engine->damage.xmax=
    engine->damage.ymin= engine->damage.ymax= 0.0;

  /* Fill in virtual function table */
  engine->Kill= Kill;
  engine->Clear= Clear;
  engine->Flush= Flush;
  engine->ChangeMap= ChangeMap;
  engine->ChangePalette= ChangePalette;
  engine->DrawLines= DrawLines;
  engine->DrawMarkers= DrawMarkers;
  engine->DrwText= DrwText;
  engine->DrawFill= DrawFill;
  engine->DrawCells= DrawCells;
  engine->DrawDisjoint= DrawDisjoint;
  engine->ClearArea= &DefaultClearArea;   /* damage causes complete redraw */

  return engine;
}

void gp_delete_engine(gp_engine_t *engine)
{
  gp_engine_t *eng= gp_engines;
  if (!engine) return;

  /* Unlink from gp_engines list */
  if (engine->active) gp_deactivate(engine);
  if (eng==engine) gp_engines= engine->next;
  else {
    /* Because of recursive deletes necessary to deal with X window
       deletions (see xbasic.c:ShutDown, hlevel.c:ShutDownDev), if
       the engine has already been removed from the list, it means that
       this routine is being called for the second time for this engine,
       and pl_free must NOT be called.  Fix this someday.  */
    while (eng && eng->next!=engine) eng= eng->next;
    if (!eng) return;
    eng->next= engine->next;
  }

  pl_free(engine);
}

/* ------------------------------------------------------------------------ */

void gp_kill_engine(gp_engine_t *engine)
{
  if (engine) engine->Kill(engine);
}

int gp_activate(gp_engine_t *engine)
{
  if (!engine) return 1;
  if (!engine->active) {
    engine->active= 1;
    engine->nextActive= gp_active_engines;
    gp_active_engines= engine;
    engine->ChangeMap(engine);
  }
  return 0;
}

int gp_deactivate(gp_engine_t *engine)
{
  if (!engine) return 1;
  if (engine->active) {
    gp_engine_t *active= gp_active_engines;
    engine->active= 0;
    if (active==engine) gp_active_engines= engine->nextActive;
    else {
      while (active->nextActive!=engine) active= active->nextActive;
      active->nextActive= engine->nextActive;
    }
  }
  return 0;
}

int gp_preempt(gp_engine_t *engine)
{
  gp_preempted_engine= engine;
  if (engine && !engine->active) engine->ChangeMap(engine);
  return 0;
}

int gp_is_active(gp_engine_t *engine)
{
  if (!engine) return 0;
  return engine==gp_preempted_engine? 1 : engine->active;
}

int gp_clear(gp_engine_t *engine, int flag)
{
  int value= 0;
  if (!engine) {
    for (engine=gp_next_active_engine(0) ; engine ; engine=gp_next_active_engine(engine)) {
      engine->damaged= engine->inhibit= 0;
      engine->lastDrawn= -1;
      engine->systemsSeen[0]= engine->systemsSeen[1]= 0;
      value|= engine->Clear(engine, flag);
    }
  } else {
    engine->damaged= engine->inhibit= 0;
    engine->lastDrawn= -1;
    engine->systemsSeen[0]= engine->systemsSeen[1]= 0;
    value= engine->Clear(engine, flag);
  }
  return value;
}

int gp_flush(gp_engine_t *engine)
{
  if (!engine) {
    int value= 0;
    for (engine=gp_next_active_engine(0) ; engine ; engine=gp_next_active_engine(engine))
      value|= engine->Flush(engine);
    return value;
  }
  return engine->Flush(engine);
}

gp_engine_t *gp_next_engine(gp_engine_t *engine)
{
  return engine? engine->next : gp_engines;
}

gp_engine_t *gp_next_active_engine(gp_engine_t *engine)
{
  if (gp_preempted_engine) return engine? 0 : gp_preempted_engine;
  else return engine? engine->nextActive : gp_active_engines;
}

/* ------------------------------------------------------------------------ */

int gp_set_transform(const gp_transform_t *trans)
{
  gp_engine_t *engine;

  if (trans!=&gp_transform) gp_transform= *trans;

  for (engine=gp_next_active_engine(0) ; engine ; engine=gp_next_active_engine(engine))
    engine->ChangeMap(engine);

  return 0;
}

int gp_set_landscape(gp_engine_t *engine, int landscape)
{
  if (!engine) {
    for (engine=gp_next_active_engine(0) ; engine ; engine=gp_next_active_engine(engine))
      engine->landscape= landscape;
  } else {
    engine->landscape= landscape;
  }
  return 0;
}

void gp_set_map(const gp_box_t *src, const gp_box_t *dst, gp_xymap_t *map)
{
  map->x.scale= (dst->xmax-dst->xmin)/(src->xmax-src->xmin);
  map->x.offset=  dst->xmin - map->x.scale*src->xmin;
  map->y.scale= (dst->ymax-dst->ymin)/(src->ymax-src->ymin);
  map->y.offset=  dst->ymin - map->y.scale*src->ymin;
}

void gp_set_device_map(gp_engine_t *engine)
{
  gp_set_map(&engine->transform.viewport, &engine->transform.window,
           &engine->devMap);
}

void gp_compose_map(gp_engine_t *engine)
{
  gp_map_t *devx= &engine->devMap.x;
  gp_map_t *devy= &engine->devMap.y;
  gp_map_t *mapx= &engine->map.x;
  gp_map_t *mapy= &engine->map.y;
  mapx->scale=
    devx->scale*(gp_transform.viewport.xmax-gp_transform.viewport.xmin)/
                (gp_transform.window.xmax-gp_transform.window.xmin);
  mapx->offset= devx->offset + devx->scale*gp_transform.viewport.xmin -
                mapx->scale*gp_transform.window.xmin;
  mapy->scale=
    devy->scale*(gp_transform.viewport.ymax-gp_transform.viewport.ymin)/
                (gp_transform.window.ymax-gp_transform.window.ymin);
  mapy->offset= devy->offset + devy->scale*gp_transform.viewport.ymin -
                mapy->scale*gp_transform.window.ymin;
}

/* ------------------------------------------------------------------------ */

/* Scratch space used by gp_int_points and gp_int_segments */
static void *scratch= 0;
static long scratchPoints= 0, scratchSegs= 0;

static void MoreScratch(long np, long ns)
{
  if (scratch) pl_free(scratch);
  if (np) {
    np+= 64;
    scratch= (void *)pl_malloc(sizeof(gp_point_t)*np);
    scratchPoints= np;
    scratchSegs= (sizeof(gp_point_t)*np)/sizeof(gp_segment_t);
  } else {
    ns+= 32;
    scratch= (void *)pl_malloc(sizeof(gp_segment_t)*ns);
    scratchSegs= ns;
    scratchPoints= (sizeof(gp_segment_t)*ns)/sizeof(gp_point_t);
  }
}

long gp_int_points(const gp_xymap_t *map, long maxPoints, long n,
                 const gp_real_t *x, const gp_real_t *y, gp_point_t **result)
{
  gp_real_t scalx= map->x.scale, offx= map->x.offset;
  gp_real_t scaly= map->y.scale, offy= map->y.offset;
  long i, np= maxPoints<n? maxPoints : n;
  gp_point_t *point;

  if (np+1>scratchPoints) MoreScratch(np+1, 0); /* allow for closure pt */
  *result= point= scratch;

  for (i=0 ; i<np ; i++) {
    point[i].x= (short)(scalx*x[i]+offx);
    point[i].y= (short)(scaly*y[i]+offy);
  }

  return np;
}

long gp_int_segments(const gp_xymap_t *map, long maxSegs, long n,
               const gp_real_t *x1, const gp_real_t *y1,
               const gp_real_t *x2, const gp_real_t *y2, gp_segment_t **result)
{
  gp_real_t scalx= map->x.scale, offx= map->x.offset;
  gp_real_t scaly= map->y.scale, offy= map->y.offset;
  long i, ns= maxSegs<n? maxSegs : n;
  gp_segment_t *seg;

  if (ns>scratchSegs) MoreScratch(0, ns);
  *result= seg= scratch;

  for (i=0 ; i<ns ; i++) {
    seg[i].x1= (short)(scalx*x1[i]+offx);
    seg[i].y1= (short)(scaly*y1[i]+offy);
    seg[i].x2= (short)(scalx*x2[i]+offx);
    seg[i].y2= (short)(scaly*y2[i]+offy);
  }

  return ns;
}

/* ------------------------------------------------------------------------ */

void gp_put_gray(int nColors, pl_col_t *palette)
{
  /*
  while (nColors--) {
    palette->gray=
      ((int)palette->red+(int)palette->green+(int)palette->blue)/3;
    palette++;
  }
  */
}

void gp_put_ntsc(int nColors, pl_col_t *palette)
{
  /*
  while (nColors--) {
    palette->gray=
      (30*(int)palette->red+59*(int)palette->green+11*(int)palette->blue)/100;
    palette++;
  }
  */
}

void gp_put_rgb(int nColors, pl_col_t *palette)
{
  /*
  while (nColors--) {
    palette->red= palette->green= palette->blue= palette->gray;
    palette++;
  }
  */
}

int gp_set_palette(gp_engine_t *engine, pl_col_t *palette, int nColors)
{
  if (!engine) return 0;
  if (nColors<0) {
    palette= 0;
    nColors= 0;
  }
  engine->palette= palette;
  engine->nColors= nColors;
  engine->colorChange= 1;
  return engine->ChangePalette(engine);
}

int gp_get_palette(gp_engine_t *engine, pl_col_t **palette)
{
  *palette= engine? engine->palette : 0;
  return engine? engine->nColors : 0;
}

int gp_dump_colors(gp_engine_t *engine, int colorMode)
{
  if (!engine) {
    for (engine=gp_next_active_engine(0) ; engine ; engine=gp_next_active_engine(engine))
      { engine->colorMode= colorMode;   engine->colorChange= 1; }
  } else {
    engine->colorMode= colorMode;   engine->colorChange= 1;
  }
  return 0;
}

/* ------------------------------------------------------------------------ */

long gp_clip_cells(gp_map_t *map, gp_real_t *px, gp_real_t *qx,
                 gp_real_t xmin, gp_real_t xmax, long ncells, long *off)
{
  long imin, imax;
  gp_real_t p, q, dx;
  gp_real_t scale= map->scale;
  gp_real_t offset= map->offset;

  xmin= xmin*scale+offset;
  xmax= xmax*scale+offset;
  if (xmin>xmax) {gp_real_t tmp=xmin; xmin=xmax; xmax=tmp;}
  p= (*px)*scale+offset;
  q= (*qx)*scale+offset;

  if (p<q && q>=xmin && p<=xmax) {
    dx= (q-p)/(gp_real_t)ncells;
    if (p<xmin) {
      imin= (long)((xmin-p)/dx);
      p+= dx*(gp_real_t)imin;
    } else {
      imin= 0;
    }
    if (q>xmax) {
      imax= (long)((q-xmax)/dx);
      q-= dx*(gp_real_t)imax;
      imax= ncells-imax;
    } else {
      imax= ncells;
    }
    if (imax-imin<2) {
      if (imax==imin) {
        if (p<xmin) p= xmin;
        if (q>xmax) q= xmax;
      } else {
        if (p<xmin && q>xmax) {
          if (q-xmax > xmin-p) { q-= xmin-p;  p= xmin; }
          else { p+= q-xmax;  q= xmax; }
        }
      }
    }
  } else if (p>q && p>=xmin && q<=xmax) {
    dx= (p-q)/(gp_real_t)ncells;
    if (q<xmin) {
      imax= (long)((xmin-q)/dx);
      q+= dx*(gp_real_t)imax;
      imax= ncells-imax;
    } else {
      imax= ncells;
    }
    if (p>xmax) {
      imin= (long)((p-xmax)/dx);
      p-= dx*(gp_real_t)imin;
    } else {
      imin= 0;
    }
    if (imax-imin<2) {
      if (imax==imin) {
        if (q<xmin) q= xmin;
        if (p>xmax) p= xmax;
      } else {
        if (q<xmin && p>xmax) {
          if (p-xmax > xmin-q) { p-= xmin-q;  q= xmin; }
          else { q+= p-xmax;  p= xmax; }
        }
      }
    }
  } else {
    imin= 0;
    imax= -1;
  }

  *px= p;
  *qx= q;
  *off= imin;

  return imax-imin;
}

/* ------------------------------------------------------------------------ */

int gp_intersect(const gp_box_t *box1, const gp_box_t *box2)
{
  /* Algorithm assumes min<max for x and y in both boxes */
  return box1->xmin<=box2->xmax && box1->xmax>=box2->xmin &&
         box1->ymin<=box2->ymax && box1->ymax>=box2->ymin;
}

int gp_contains(const gp_box_t *box1, const gp_box_t *box2)
{
  /* Algorithm assumes min<max for x and y in both boxes */
  return box1->xmin<=box2->xmin && box1->xmax>=box2->xmax &&
         box1->ymin<=box2->ymin && box1->ymax>=box2->ymax;
}

void gp_swallow(gp_box_t *preditor, const gp_box_t *prey)
{
  /* Algorithm assumes min<max for x and y in both boxes */
  if (preditor->xmin>prey->xmin) preditor->xmin= prey->xmin;
  if (preditor->xmax<prey->xmax) preditor->xmax= prey->xmax;
  if (preditor->ymin>prey->ymin) preditor->ymin= prey->ymin;
  if (preditor->ymax<prey->ymax) preditor->ymax= prey->ymax;
}

/* ------------------------------------------------------------------------ */

/* These recondite routines are required to handle editing a drawing
   on one or more interactive engines.  The restriction to few
   routines builds in certain inefficiencies; if every drawing were always
   associated with one interactive engine some of the inefficiency could
   be reduced.  These are not intended for external use.  */

extern int gdNowRendering, gdMaxRendered;
int gdNowRendering= -1;
int gdMaxRendered= -1;

int _gd_begin_dr(gd_drawing_t *drawing, gp_box_t *damage, int landscape)
{
  int needToRedraw= 0;
  gp_engine_t *eng;

  if (damage) {
    /* If drawing has incurred damage, report damage to ALL engines
       interested in the drawing (not just active engines).  */
    for (eng=gp_next_engine(0) ; eng ; eng=gp_next_engine(eng))
      if (eng->drawing==drawing) gp_damage(eng, drawing, damage);
  }

  /* Loop on active engines to alert them that drawing is coming.  */
  for (eng=gp_next_active_engine(0) ; eng ; eng=gp_next_active_engine(eng)) {
    if (eng->drawing!=drawing) {
      /* This engine is not marked as interested in this drawing.
         Mark it, and reset damaged and lastDrawn flags so that no
         elements will be inhibited.  */
      eng->drawing= drawing;
      eng->lastDrawn= -1;
      eng->damaged= 0;
      if (landscape != eng->landscape) {
        eng->landscape= landscape;
        /* This change will be detected and acted upon by the first call
           to the ChangeMap method (gp_set_transform).  */
      }
      /* The semantics here are subtle --
         After a ClearDrawing, gd_detach zeroes eng->drawing in order to
         communicate that the drawing has been cleared.  Thus, the code
         gets here on a gd_draw after the drawing has been cleared, so
         the time has come to carry out the deferred clearing of this
         engine's plotting surface.  */
      gp_clear(eng, GP_CONDITIONALLY);
      needToRedraw= 1;

    } else if (eng->damaged) {
      /* This engine was interested in the drawing, which has been
         damaged.  Clear damaged area in preparation for repair work.
         (This is redundant if the damage was due to an X windows
          expose event, but the resulting inefficiency is very small.)  */
      eng->ClearArea(eng, &eng->damage);
      needToRedraw= 1;

    } else if (eng->lastDrawn<drawing->nElements-1) {
      needToRedraw= 1;
    }
  }

  gdNowRendering= gdMaxRendered= -1;
  return needToRedraw;
}

int _gd_begin_sy(gp_box_t *tickOut, gp_box_t *tickIn, gp_box_t *viewport,
              int number, int sysIndex)
{
  gp_engine_t *eng;
  int value= 0;
  long sysMask;

  /* Note that this is harmless if sysIndex>2*sizeof(long)--
     just slightly inefficient in that ticks and elements will GP_ALWAYS
     be drawn...  This shouldn't be a practical problem.  */
  if (sysIndex>sizeof(long)) {
    sysMask= 1 << (sysIndex-sizeof(long));
    sysIndex= 1;
  } else {
    sysMask= 1 << sysIndex;
    sysIndex= 0;
  }

  /* Loop on active engines to determine whether any require ticks or
     elements to be drawn.  Set inhibit switches for ticks.  */
  for (eng=gp_next_active_engine(0) ; eng ; eng=gp_next_active_engine(eng)) {
    if ( ! (eng->systemsSeen[sysIndex] & sysMask) ) {
      /* this engine has never seen this system */
      value|= 3;
      eng->inhibit= 0;
      eng->systemsSeen[sysIndex]|= sysMask;

    } else if (eng->damaged && gp_intersect(tickOut, &eng->damage)) {
      /* engine damage touches this coordinate system--
         redraw ticks if region between tickIn and tickOut damaged,
         redraw elements if viewport damaged */
      if (!tickIn || !gp_contains(tickIn, &eng->damage)) {
        value|= 2;
        eng->inhibit= 0;
      } else eng->inhibit= 1;
      if (number>eng->lastDrawn || gp_intersect(viewport, &eng->damage))
        value|= 1;

    } else {
      /* engine undamaged or damage doesn't touch this system--
         redraw elements if any new ones, don't redraw ticks */
      eng->inhibit= 1;
      if (number>eng->lastDrawn) value|= 1;
    }
  }

  return value;
}

int _gd_begin_el(gp_box_t *box, int number)
{
  gp_engine_t *eng;
  int value= 0;

  /* Loop on active engines to determine whether any require this
     element to be drawn, and to set inhibit switches so that some
     may draw it and others not.  */
  for (eng=gp_next_active_engine(0) ; eng ; eng=gp_next_active_engine(eng)) {
    if (number>eng->lastDrawn) {
      /* this engine hasn't seen this element before */
      eng->inhibit= 0;
      value= 1;
      if (eng->damaged && gdMaxRendered<=eng->lastDrawn) {
        /* If this is the first new element, the damage flag
           must be reset, and ChangeMap must be called to set the
           clip rectangle back to its undamaged boundary.  */
        eng->damaged= 0;
        eng->ChangeMap(eng);
      }

    } else if (box && eng->damaged && gp_intersect(box, &eng->damage)) {
      /* engine damage touches this element */
      eng->inhibit= 0;
      value= 1;

    } else {
      /* this element has been seen before and hasn't been damaged */
      eng->inhibit= 1;
    }

    /* set number of element currently being drawn for _gd_end_dr */
    gdNowRendering= number;
    if (gdMaxRendered<gdNowRendering) gdMaxRendered= gdNowRendering;
  }

  return value;
}

void _gd_end_dr(void)
{
  gp_engine_t *eng;
  /* Done with this drawing- reset inhibit, damaged, and lastDrawn flags */
  for (eng=gp_next_active_engine(0) ; eng ; eng=gp_next_active_engine(eng)) {
    if (eng->lastDrawn<gdMaxRendered) eng->lastDrawn= gdMaxRendered;
    eng->inhibit= eng->damaged= 0;
  }
}

void gp_damage(gp_engine_t *eng, gd_drawing_t *drawing, gp_box_t *box)
{
  if (eng->drawing!=drawing || !eng->marked) return;
  if (eng->ClearArea==&DefaultClearArea) {
    /* This engine doesn't need to record the damage box */
    eng->damaged= 1;
  } else if (eng->damaged) {
    /* drawing is already damaged on this engine */
    if (eng->damage.xmin>box->xmin) eng->damage.xmin= box->xmin;
    if (eng->damage.xmax<box->xmax) eng->damage.xmax= box->xmax;
    if (eng->damage.ymin>box->ymin) eng->damage.ymin= box->ymin;
    if (eng->damage.ymax<box->ymax) eng->damage.ymax= box->ymax;
  } else {
    /* drawing is currently undamaged on this engine */
    eng->damaged= 1;
    eng->damage= *box;
  }
}

void gd_detach(gd_drawing_t *drawing, gp_engine_t *engine)
{
  gp_engine_t *eng;
  for (eng=gp_next_engine(0) ; eng ; eng=gp_next_engine(eng)) {
    if (!drawing || eng->drawing==drawing) {
      eng->drawing= 0;
      eng->inhibit= eng->damaged= 0;
      eng->lastDrawn= -1;
    }
  }
}

/* ------------------------------------------------------------------------ */
