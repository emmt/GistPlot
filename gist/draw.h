/*
 * $Id: gist/draw.h,v 1.1 2005-09-18 22:04:28 dhmunro Exp $
 * Declare display list structures of GIST C interface
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_DRAW_H
#define _GIST_DRAW_H

#include <gist2.h>

/* Virtual function table for display list objects */
typedef struct _gd_operations gd_operations_t;
struct _gd_operations {
  int type;    /* element type-- GD_ELEM_LINES, ... GD_ELEM_SYSTEM */

  /* Kill is virtual destructor (constructors are gd_add_lines, etc.)
     If all is non-0, then every element in this ring is killed */
  void (*Kill)(void *el);

  /* GetProps sets gd_properties and ga_attributes from element, returns element type */
  int (*GetProps)(void *el);

  /* SetProps sets element properties from ga_attributes, gd_properties
     If xyzChanged has GD_CHANGE_XY or GD_CHANGE_Z or both set, then
     log x and y arrays are freed or contours are recomputed or both.
     Returns 0 on success or 1 if memory error recomputing contours.  */
  int (*SetProps)(void *el, int xyzChanged);

  /* Draw generates appropriate drawing primitives for this element.
     Returns 0 if successful.  */
  int (*Draw)(void *el, int xIsLog, int yIsLog);

  /* Scan rescans the (x,y) extent of this element according to the
     flags (extreme values, log scaling) and limits, setting extreme
     limits as appropriate for this curve.  If the limits are restricted
     and if no points will be plotted, limits.xmin=limits.xmax or
     limits.ymin=limits.ymax on return.  The el->box is also set
     appropriately.  */
  int (*Scan)(void *el, int flags, gp_box_t *limits);

  /* Margin returns a box representing the amount of NDC space which
     should be left around the element's box to allow for finite line
     thickness, projecting text, and the like. */
  void (*Margin)(void *el, gp_box_t *margin);
};

/* Generic display list element */
typedef struct _gd_element gd_element_t;
struct _gd_element {
  gd_operations_t *ops;  /* virtual function table */
  gd_element_t *next, *prev;  /* elements form doubly linked rings */
  gp_box_t box;       /* extreme values of coordinates for this object */
  int hidden;      /* hidden flag */
  char *legend;    /* descriptive text */
  int number;      /* drawing->nElements when this element added */
};

/* gd_add_lines element */
typedef struct _ge_lines ge_lines_t;
struct _ge_lines {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  int n;                        /* number of points */
  gp_real_t *x, *y, *xlog, *ylog;  /* (x,y) points */
  gp_line_attribs_t l;              /* line attributes */
  ga_line_attribs_t dl;             /* decorated line attributes */
  gp_marker_attribs_t m;            /* marker attributes */
};

/* gd_add_disjoint element */
typedef struct _ge_disjoint ge_disjoint_t;
struct _ge_disjoint {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  int n;                        /* number of segments */
  gp_real_t *x, *y, *xlog, *ylog;  /* (px,py) points */
  gp_real_t *xq, *yq, *xqlog, *yqlog;  /* (qx,qy) points */
  gp_line_attribs_t l;              /* line attributes */
};

/* gd_add_text element */
typedef struct _ge_text ge_text_t;
struct _ge_text {
  gd_element_t el;
  gp_real_t x0, y0;                /* text position */
  char *text;                   /* text string */
  gp_text_attribs_t t;              /* text attributes */
};

/* gd_add_cells element */
typedef struct _ge_cells ge_cells_t;
struct _ge_cells {
  gd_element_t el;
  gp_real_t px, py, qx, qy;        /* colors[0][width-1] at (qx,py) */
  long width, height;           /* width and height of image */
  gp_color_t *colors;              /* image array */
  int rgb;
};

/* gd_add_fill element */
typedef struct _ge_polys ge_polys_t;
struct _ge_polys {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  gp_real_t *x, *y, *xlog, *ylog;  /* (px,py) points */
  long n, *pn;                  /* pn[n] list of polygon lengths */
  gp_color_t *colors;              /* n fill colors */
  gp_line_attribs_t e;              /* edge attributes */
  int rgb;
};

/* gd_add_mesh element */
typedef struct _ge_mesh ge_mesh_t;
struct _ge_mesh {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  int noCopy;                   /* also has bit for mesh.reg */
  ga_mesh_t mesh;              /* mesh coordinates */
  gp_real_t *xlog, *ylog;          /* 0 unless log axes */
  int region, boundary;         /* region number, boundary flag */
  gp_line_attribs_t l;              /* line attributes */
  int inhibit;                  /* +1 to inhibit j=const lines,
                                   +2 to inhibit i=const lines */
};

/* gd_add_fillmesh element */
typedef struct _ge_fill ge_fill_t;
struct _ge_fill {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  int noCopy;                   /* also has bit for mesh.reg */
  ga_mesh_t mesh;              /* mesh coordinates */
  gp_real_t *xlog, *ylog;          /* 0 unless log axes */
  int region;                   /* this and above must match ge_mesh_t */

  gp_color_t *colors;              /* color array */
  long nColumns;                /* row dimension of colors */
  gp_line_attribs_t e;              /* edge attributes */
  int rgb;
};

/* gd_add_vectors element */
typedef struct _ge_vectors ge_vectors_t;
struct _ge_vectors {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  int noCopy;                   /* also has bit for mesh.reg */
  ga_mesh_t mesh;              /* mesh coordinates */
  gp_real_t *xlog, *ylog;          /* 0 unless log axes */
  int region;                   /* this and above must match ge_mesh_t */

  gp_real_t *u, *v;                /* iMax-by-jMax vector components (u,v) */
  gp_real_t scale;                 /* vectors plot as (dx,dy)=scale*(u,v) */
  gp_line_attribs_t l;              /* line attributes */
  gp_fill_attribs_t f;              /* filled area attributes */
  ga_vect_attribs_t vect;           /* vector attributes */
};

/* gd_add_contours element */
typedef struct _ge_contours ge_contours_t;
struct _ge_contours {
  gd_element_t el;
  gp_box_t linBox, logBox;         /* extreme values */
  int noCopy;                   /* also has bit for mesh.reg */
  ga_mesh_t mesh;              /* mesh coordinates */
  gp_real_t *xlog, *ylog;          /* 0 unless log axes */
  int region;                   /* this and above must match ge_mesh_t */

  gp_real_t *z;                    /* iMax-by-jMax contour array */
  int nLevels;                  /* number of contour levels */
  gp_real_t *levels;               /* list of contour levels */
  ge_lines_t **groups;             /* groups of contour curves at each level */
  gp_line_attribs_t l;              /* line attributes */
  ga_line_attribs_t dl;             /* decorated line attributes */
  gp_marker_attribs_t m;            /* marker attributes */
};

/* Coordinate system-- a gp_transform_t with a tick style */
typedef struct _ge_system ge_system_t;
struct _ge_system {
  gd_element_t el;
  ga_tick_style_t ticks;      /* tick style */
  gp_transform_t trans;      /* viewport and window */
  int flags;              /* for computing the limits (see gd_properties_t) */
  int rescan;             /* set if must be rescanned before redraw */
  int unscanned;          /* element number of 1st unscanned element */
  gd_element_t *elements;    /* elements in this coordinate system */
  gp_box_t savedWindow;      /* saved window limits for gd_save_limits */
  int savedFlags;         /* saved flags for gd_save_limits */
  ga_ticker_t *xtick, *ytick;    /* alternative tick and label generators */
  ga_labeler_t *xlabel, *ylabel;
};

typedef struct _ge_legend_box ge_legend_box_t;
struct _ge_legend_box {
  gp_real_t x, y;              /* NDC location of this legend box */
  gp_real_t dx, dy;            /* if non-zero, offset to 2nd column */
  gp_text_attribs_t textStyle;  /* font, size, etc. of these legends */
  int nchars, nlines;       /* max number of characters per line, lines */
  int nwrap;                /* max number of lines to wrap long legends */
};

/* Drawing-- the complete display list */
struct _gd_drawing {
  gd_drawing_t *next;          /* drawings kept in a list */
  int cleared;            /* next element added will delete everything */
  int nSystems;           /* total number of systems (now) in drawing */
  int nElements;          /* total number of elements (ever) in drawing */
  ge_system_t *systems;      /* coordinate systems */
  gd_element_t *elements;    /* elements not belonging to any system */
  int damaged;            /* set if damage box meaningful */
  gp_box_t damage;           /* region damaged by gd_edit, etc. */
  int landscape;          /* non-0 for landscape, 0 for portrait */
  ge_legend_box_t legends[2]; /* 0 is normal legend box, 1 is contours */
};

/* The list of GIST drawings */
PL_EXTERN gd_drawing_t *gd_draw_list;

/* The following functions are intended to assist in writing the
   constructors for new types of gd_drawing_t Elements */

/* generic function for adding elements to current system--
   note that you must reset el->ops by hand, since only the predefined
   types are treated properly */
PL_EXTERN void ge_add_element(int type, gd_element_t *element);

/* generic function to mark element as unscanned if in a coordinate
   system, else set its box */
PL_EXTERN void ge_mark_for_scan(gd_element_t *el, gp_box_t *linBox);

/* generic function to get the current mesh, returning a GeMeshXY*,
   and optionally not copying the ga_attributes.mesh arrays.  The return
   value is iMax*jMax (0 on failure) */
PL_EXTERN long ge_get_mesh(int noCopy, ga_mesh_t *meshin, int region,
                      void *vMeshEl);

/* Updates the system limits if necessary -- do not use lightly.  */
PL_EXTERN int gd_scan(ge_system_t *system);

/* Defined in engine.c.  These routines communicate state information
   from the drawing to the engine.  They are not intended for external
   use.  */
PL_EXTERN int _gd_begin_dr(gd_drawing_t *drawing, gp_box_t *damage, int landscape);
   /* _gd_begin_sy returns 1 bit to draw elements, 2 bit to draw ticks */
PL_EXTERN int _gd_begin_sy(gp_box_t *tickOut, gp_box_t *tickIn,
                       gp_box_t *viewport, int number, int sysIndex);
   /* _gd_begin_el returns 1 if elements should be drawn, else 0 */
PL_EXTERN int _gd_begin_el(gp_box_t *box, int number);
PL_EXTERN void _gd_end_dr(void);

#endif
