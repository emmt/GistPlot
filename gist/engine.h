/*
 * $Id: gist/engine.h,v 1.1 2005-09-18 22:04:26 dhmunro Exp $
 * Declare common properties of all GIST engines
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_ENGINE_H
#define _GIST_ENGINE_H

#include <gist2.h>

/* ------------------------------------------------------------------------ */

struct _gp_engine {
  gp_callbacks_t *on;
  gp_engine_t *next;
  gp_engine_t *nextActive;
  char *name;

  int active;
  int marked;    /* set if any marks have been made on current page */

  int landscape;          /* non-0 if page is wider than tall */
  gp_transform_t transform;    /* viewport in NDC, window in VDC */
  gp_xymap_t devMap; /* actual coefficients for NDC->VDC mapping */
  gp_xymap_t map;     /* actual coefficients for WC->VDC mapping */

  /* Current color palette (see gist2.h) */
  int colorChange;  /* set this to alert gp_engine_t that palette has changed */
  int colorMode;    /* for hardcopy devices, set to dump palette to file */
  int nColors;
  pl_col_t *palette;  /* pointer is NOT owned by this engine */

  /* Handling incremental changes and damage to a drawing.  */
  gd_drawing_t *drawing;
  int lastDrawn;
  long systemsSeen[2];
  int inhibit;     /* set by GdAlert if next primitive is to be ignored */
  int damaged;
  gp_box_t damage;    /* Never used if no ClearArea function provided */

  /* --------------- Virtual function table ------------------- */

  /* Close device and free all related memory (gp_kill_engine)  */
  void (*Kill)(gp_engine_t *engine);

  /* New page (frame advance) */
  int (*Clear)(gp_engine_t *engine, int always);

  /* Flush output buffers */
  int (*Flush)(gp_engine_t *engine);

  /* Change coordinate transformation (gp_set_transform)
     If engine damaged, must set clipping to damage.  */
  void (*ChangeMap)(gp_engine_t *engine);

  /* Change color palette (gp_set_palette) */
  int (*ChangePalette)(gp_engine_t *engine); /* returns maximum palette size */

  /* Polyline primitive.  Segments have already been clipped.  */
  int (*DrawLines)(gp_engine_t *engine, long n, const gp_real_t *px,
                   const gp_real_t *py, int closed, int smooth);
  /* Polymarker primitive.  Points have already been clipped.
     Can use gp_draw_pseudo_mark if no intrinsic polymarker primitive.  */
  int (*DrawMarkers)(gp_engine_t *engine, long n,
                     const gp_real_t *px, const gp_real_t *py);
  /* Text primitive.  No clipping has yet been done.  */
  int (*DrwText)(gp_engine_t *engine, gp_real_t x0, gp_real_t y0, const char *text);
  /* Filled area primitive.  Polygon has already been clipped.  */
  int (*DrawFill)(gp_engine_t *engine, long n, const gp_real_t *px, const gp_real_t *py);
  /* Cell array primitive.  No clipping has yet been done.  */
  int (*DrawCells)(gp_engine_t *engine, gp_real_t px, gp_real_t py, gp_real_t qx, gp_real_t qy,
                   long width, long height, long nColumns,
                   const gp_color_t *colors);
  /* Disjoint line primitive.  Segments have already been clipped.  */
  int (*DrawDisjoint)(gp_engine_t *engine, long n, const gp_real_t *px,
                      const gp_real_t *py, const gp_real_t *qx, const gp_real_t *qy);

  /* Clear a damaged area (box in NDC units) - set to 0 by gp_new_engine */
  void (*ClearArea)(gp_engine_t *engine, gp_box_t *box);
};

/* Linked lists of all engines and of all active engines */
PL_EXTERN gp_engine_t *gp_engines;
PL_EXTERN gp_engine_t *gp_active_engines;

/* Generic gp_engine_t constructor and destructor */
PL_EXTERN gp_engine_t *gp_new_engine(long size, char *name, gp_callbacks_t *on,
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
                      const gp_real_t*,const gp_real_t*));
PL_EXTERN void gp_delete_engine(gp_engine_t *engine);

/* ------------------------------------------------------------------------ */
/* Coordinate mapping */

/* Set engine->devMap from engine->transform */
PL_EXTERN void gp_set_device_map(gp_engine_t *engine);

/* Compose WC->VDC engine->map coefficients given gp_transform (WC->NDC) and
   engine->devMap (NDC->VDC).  */
PL_EXTERN void gp_compose_map(gp_engine_t *engine);

/* The X window, CGM, and PostScript devices can all be based on
   integer coordinate systems using points and segments which are
   short integers.  Here are some common routines for forming and
   manipulating these.  */

typedef struct _gp_point gp_point_t;
struct _gp_point {  /* same as X windows XPoint */
  short x, y;
};

typedef struct _gp_segment gp_segment_t;
struct _gp_segment {  /* same as X windows XSegment */
  short x1, y1, x2, y2;
};

/* Returns number of points processed this pass (<=maxPoints) */
PL_EXTERN long gp_int_points(const gp_xymap_t *map, long maxPoints, long n,
                          const gp_real_t *x, const gp_real_t *y, gp_point_t **result);

/* Returns number of segments processed this pass (<=maxSegs) */
PL_EXTERN long gp_int_segments(const gp_xymap_t *map, long maxSegs, long n,
                        const gp_real_t *x1, const gp_real_t *y1,
                        const gp_real_t *x2, const gp_real_t *y2,gp_segment_t **result);

/* ------------------------------------------------------------------------ */

/* DrawMarkers based on DrwText.  Uses Helvetica font, for use when:
   (1) A device has no polymarker primitive, or
   (2) You want the marker to be a character (used by ga_draw_lines)   */
/* Note: gp_draw_pseudo_mark is defined in gist.c to share static functions.  */
PL_EXTERN int gp_draw_pseudo_mark(gp_engine_t *engine, long n,
                          const gp_real_t *px, const gp_real_t *py);

/* Routine to clip cell array (one axis at a time) */
PL_EXTERN long gp_clip_cells(gp_map_t *map, gp_real_t *px, gp_real_t *qx,
                          gp_real_t xmin, gp_real_t xmax, long ncells, long *off);

/* Raw routine to inflict damage */
PL_EXTERN void gp_damage(gp_engine_t *eng, gd_drawing_t *drawing, gp_box_t *box);

/* ------------------------------------------------------------------------ */

#endif
