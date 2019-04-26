/*
 * $Id: gist2.h,v 1.2 2009-10-19 04:37:51 dhmunro Exp $
 * Declare GIST interface for C programs
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST2_H
#define _GIST2_H 1

#include <play2.h>
#include <play/extern.h>
#include <play/io.h>

/* current code breaks if gp_real_t is float instead of double */
typedef double gp_real_t;
typedef unsigned char gp_color_t;

/* Normalized device coordinates (NDC) have a definite meaning to GIST--
   PostScript and printing devices like units of 1/72.27 inch,
   X11 release 4 likes units of 1/75 inch or 1/100 inch (for its fonts).
   Next, 1.0 NDC should correspond roughly to 10.5 inches (the wide
   dimension of an ordinary sheet of paper.  This will happen if
   one point is 0.0013 NDC units (1.0 NDC unit is about 10.64 inch).
   Then NDC origin is approximately in the lower left hand corner of
   the page, although this can be shifted slightly to get nice margins.
 */
#define GP_ONE_POINT 0.0013000
#define GP_INCHES_PER_POINT (1./72.27)
#define GP_ONE_INCH (72.27*GP_ONE_POINT)

PL_EXTERN char gp_error[128];  /* most recent error message */

PL_EXTERN char *gp_default_path;  /* set in Makefile or gread.c, can be
                                  overridden by resetting directly */

PL_EXTERN pl_file_t *gp_open(const char *name);
/* Open a Gist file.  First try with NAME, then, if NAME is not an absolute
   path, in the GISTPATH directories.  Return NULL if not found. */

/* ------------------------------------------------------------------------ */
/* Initialization Functions */

/*
  An engine in GIST is a set of device drivers.  A particular instance
  of an engine will also include a specific I/O connection for these
  drivers.  Instead of opening a GKS workstation, in GIST, you create
  a specific instance of a particular type of engine -- a PostScript
  engine, a CGM engine, or an X window engine.  Since there is a
  separate function for creating each different type of engine, GIST
  has the important advantage over GKS that only those device drivers
  actually used by your code are loaded.

   Four sets of device drivers are supplied with GIST:
     PostScript - hopefully can be fed to your laser printer
     CGM - binary format metafile, much more compact than PostScript,
           especially if there are many pages of graphics
     BX - basic play window
     FX - play window with odometer bar at top, mouse zooming, etc
 */

typedef struct _gp_engine gp_engine_t;
PL_EXTERN gp_engine_t *gp_new_ps_engine(char *name, int landscape, int mode, char *file);
PL_EXTERN gp_engine_t *gp_new_cgm_engine(char *name, int landscape, int mode, char *file);
PL_EXTERN gp_engine_t *gp_new_bx_engine(char *name, int landscape, int dpi, char *display);
PL_EXTERN gp_engine_t *gp_new_fx_engine(char *name, int landscape, int dpi, char *display);

PL_EXTERN void gp_kill_engine(gp_engine_t *engine);

PL_EXTERN int gp_input_hint, gp_private_map, gp_rgb_hint;
PL_EXTERN void gp_initializer(int *pargc, char *argv[]);
PL_EXTERN char *gp_set_path(char *gpath);
PL_EXTERN void (*gp_on_keyline)(char *msg);
PL_EXTERN void (*gp_stdout)(char *output_line);

typedef struct _gp_callbacks gp_callbacks_t;
struct _gp_callbacks {
  char *id;    /* just to ease debugging */
  /* window system events */
  void (*expose)(void *c, int *xy);
  void (*destroy)(void *c);
  void (*resize)(void *c,int w,int h);
  void (*focus)(void *c,int in);
  void (*key)(void *c,int k,int md);
  void (*click)(void *c,int b,int md,int x,int y, unsigned long ms);
  void (*motion)(void *c,int md,int x,int y);
  void (*deselect)(void *c);
};

PL_EXTERN int gp_expose_wait(gp_engine_t *eng, void (*e_callback)(void));

/* ------------------------------------------------------------------------ */
/* Control Functions */

/* Engines are kept in two lists - a list of all engines and
   a ring of active engines.  To ignore the active list and send
   only to a specific engine, call gp_preempt.  gp_preempt(0) turns
   off preemptive mode.  The preempting engine need not be active;
   it will still get all output until preempt mode is turned off, at
   which time it returns to its original state.  */
PL_EXTERN int gp_activate(gp_engine_t *engine);
PL_EXTERN int gp_deactivate(gp_engine_t *engine);
PL_EXTERN int gp_preempt(gp_engine_t *engine);
PL_EXTERN int gp_is_active(gp_engine_t *engine);  /* 1 if active or preempting, else 0 */

/* Pass engine==0 to gp_clear or gp_flush to affect all active engines.  */
PL_EXTERN int gp_clear(gp_engine_t *engine, int flag);
#define GP_CONDITIONALLY 0
#define GP_ALWAYS 1
PL_EXTERN int gp_flush(gp_engine_t *engine);

/* The Next routines provide a way to iterate throught the engine lists.
   Engines will be returned in order they are created or activated.
   Use engine==0 to get the first engine in the list; will return 0
   when there are no more engines in the list.  If preemptive mode has
   been set with gp_preempt, gp_next_active_engine returns the preempting engine,
   whether or not it is in the active list.  */
PL_EXTERN gp_engine_t *gp_next_engine(gp_engine_t *engine);
PL_EXTERN gp_engine_t *gp_next_active_engine(gp_engine_t *engine);

/* ------------------------------------------------------------------------ */
/* Transformations and Clipping */

typedef struct _gp_box gp_box_t;
struct _gp_box {
  gp_real_t xmin, xmax, ymin, ymax;
};

typedef struct _gp_transform gp_transform_t;
struct _gp_transform {
  gp_box_t viewport, window;
};

/* gp_set_transform sets the current transformation.  This involves notifying
   each active engine of the change, in turn, so that an appropriate
   transformation to device coordinates can be computed.  (gp_activate
   also sets the device coordinate transformation in the engine.)
   This transformation is also loaded into the ga_attributes attribute list.  */
PL_EXTERN int gp_set_transform(const gp_transform_t *trans);

/* Although NDC space has a particular meaning, an 8.5x11 sheet of
   paper may have x along the 8.5 inch edge (portrait mode), or x along
   the 11 inch edge (landscape mode).  In either case, the origin is the
   lower left corner of the page.  This property can be set on a per
   gp_engine_t basis using gp_set_landscape, or for all active engines with
   gp_set_landscape(0).  If you use the D level routines, you shouldn't
   need this.  */
PL_EXTERN int gp_set_landscape(gp_engine_t *engine, int landscape);

/* current transformation */
PL_EXTERN gp_transform_t gp_transform;  /* use gp_set_transform to set this properly */

/* Turn clipping on or off by setting gp_clipping.  */
PL_EXTERN int gp_clipping;         /* 1 to clip to map.viewport, 0 to not clip */

/* Often, the linear transformation represented by a gp_transform_t is
   required in the simpler form dst= scale*src+offset.  The following
   convenience routines used internally by GIST are provided:  */
typedef struct _gp_map gp_map_t;
struct _gp_map {
  gp_real_t scale, offset;
};

typedef struct _gp_xymap gp_xymap_t;
struct _gp_xymap {
  gp_map_t x, y;
};

PL_EXTERN void gp_set_map(const gp_box_t *src, const gp_box_t *dst, gp_xymap_t *map);

/* gp_portrait_box and gp_landscape_box contain, respectively, boxes representing
   an 8.5-by-11 inch and an 11-by8.5 inch page in NDC */
PL_EXTERN gp_box_t gp_portrait_box;
PL_EXTERN gp_box_t gp_landscape_box;

/* Utilities for gp_box_t, assuming min<=max for both boxes */
PL_EXTERN int gp_intersect(const gp_box_t *box1, const gp_box_t *box2);
PL_EXTERN int gp_contains(const gp_box_t *box1, const gp_box_t *box2); /* box1>=box2 */
PL_EXTERN void gp_swallow(gp_box_t *preditor, const gp_box_t *prey);

/* ------------------------------------------------------------------------ */
/* Output Primitives */

PL_EXTERN int gp_draw_lines(long n, const gp_real_t *px, const gp_real_t *py);
PL_EXTERN int gp_draw_markers(long n, const gp_real_t *px, const gp_real_t *py);
PL_EXTERN int gp_draw_text(gp_real_t x0, gp_real_t y0, const char *text);
  /* WARNING- for gp_draw_text, (x0,y0) are in WC, but the text size and
              orientation are specified in NDC (unlike GKS).  */
PL_EXTERN int gp_draw_fill(long n, const gp_real_t *px, const gp_real_t *py);
PL_EXTERN int gp_draw_cells(gp_real_t px, gp_real_t py, gp_real_t qx, gp_real_t qy,
                     long width, long height, long nColumns,
                     const gp_color_t *colors);

/* GKS seems to be missing a disjoint line primitive... */
PL_EXTERN int gp_draw_disjoint(long n, const gp_real_t *px, const gp_real_t *py,
                        const gp_real_t *qx, const gp_real_t *qy);

/* ------------------------------------------------------------------------ */
/* Output attributes */

/* Instead of a set and get function pair for each attribute, the GIST
   C interface simply provides global access to its state table.  */
typedef struct _ga_attributes ga_attributes_t;

typedef struct _gp_line_attribs gp_line_attribs_t;
struct _gp_line_attribs {
  unsigned long color;
  int type;       /* line types given by GP_LINE_SOLID, etc.  */
  gp_real_t width;   /* default 1.0 is normal width of a line */

#define GP_LINE_NONE 0
#define GP_LINE_SOLID 1
#define GP_LINE_DASH 2
#define GP_LINE_DOT 3
#define GP_LINE_DASHDOT 4
#define GP_LINE_DASHDOTDOT 5

#define GP_DEFAULT_LINE_WIDTH (0.5*GP_ONE_POINT)
#define GP_DEFAULT_LINE_INCHES (0.5*GP_INCHES_PER_POINT)
};

typedef struct _gp_marker_attribs gp_marker_attribs_t;
struct _gp_marker_attribs {
  unsigned long color;
  int type;       /* marker types given by GP_MARKER_ASTERISK, etc., or by ASCII  */
  gp_real_t size;    /* default 1.0 is normal size of a marker */

#define GP_MARKER_POINT 1
#define GP_MARKER_PLUS 2
#define GP_MARKER_ASTERISK 3
#define GP_MARKER_CIRCLE 4
#define GP_MARKER_CROSS 5

#define GP_DEFAULT_MARKER_SIZE (10.0*GP_ONE_POINT)
};

typedef struct _gp_fill_attribs gp_fill_attribs_t;
struct _gp_fill_attribs {
  unsigned long color;
  int style;      /* fill area styles given by GP_FILL_SOLID, etc.  */

#define GP_FILL_HOLLOW 0
#define GP_FILL_SOLID 1
#define GP_FILL_PATTERN 2
#define GP_FILL_HATCH 3
#define GP_FILL_EMPTY 4
};

typedef struct _gp_text_attribs gp_text_attribs_t;
struct _gp_text_attribs {
  unsigned long color;
  int font;         /* text font (GP_FONT_HELVETICA, etc.) */
  gp_real_t height;    /* character height in NDC, default 0.0156 (12pt)
                       UNLIKE GKS, GIST font sizes are always specified
                       in NDC.  This drastically simplifies coding for
                       devices like X windows, which must load a font
                       at each size.  It also conforms better with
                       a Mac-like user interface in which font size
                       in points is selected by the user.  */
  int orient;          /* text paths given by GP_ORIENT_RIGHT, etc.  */
  int alignH, alignV;  /* text alignments given by GP_HORIZ_ALIGN_NORMAL, etc.  */

  /* GKS is missing a text opacity flag.  */
  int opaque;

/* A font is a type face optionally ORed with GP_FONT_BOLD and/or GP_FONT_ITALIC. */
/* Available point sizes (for X) are 8, 10, 12, 14, 18, and 24 */
#define GP_FONT_BOLD 1
#define GP_FONT_ITALIC 2
#define GP_FONT_COURIER 0
#define GP_FONT_TIMES 4
#define GP_FONT_HELVETICA 8
#define GP_FONT_SYMBOL 12
#define GP_FONT_NEWCENTURY 16

#define GP_ORIENT_RIGHT 0
#define GP_ORIENT_UP 1
#define GP_ORIENT_LEFT 2
#define GP_ORIENT_DOWN 3

#define GP_HORIZ_ALIGN_NORMAL 0
#define GP_HORIZ_ALIGN_LEFT 1
#define GP_HORIZ_ALIGN_CENTER 2
#define GP_HORIZ_ALIGN_RIGHT 3

#define GP_VERT_ALIGN_NORMAL 0
#define GP_VERT_ALIGN_TOP 1
#define GP_VERT_ALIGN_CAP 2
#define GP_VERT_ALIGN_HALF 3
#define GP_VERT_ALIGN_BASE 4
#define GP_VERT_ALIGN_BOTTOM 5
};

/* ga_draw_lines output function supports polylines with occasional marker
   characters and/or ray arrows.  */
typedef struct _ga_line_attribs ga_line_attribs_t;
struct _ga_line_attribs {
  int closed;   /* 0 for open curve, 1 for closed curve */
  int smooth;   /* 0 for no smoothing, 1, 2, or 3 for progresively
                   smoother splines (not implemented for all Engines)
                   Note: If marks or rays, smooth is ignored.  */

  /* Note: ga_line_attribs_t and GaMarkerAttribs determine the style, size,
     and color of the line and occasional markers.  If l.style is GP_LINE_NONE,
     polymarkers will be plotted at every point rather than occasional
     markers.  */
  int marks;    /* 0 if no occasional markers, 1 if occasional markers */
  gp_real_t mSpace, mPhase;    /* occasional marker spacing and phase in NDC */

  int rays;     /* 0 if no ray arrows, 1 to get ray arrows */
  gp_real_t rSpace, rPhase;    /* ray spacing and phase in NDC */
  gp_real_t arrowL, arrowW;    /* ray arrowhead size, 1.0 is normal arrow */

#define GA_DEFAULT_ARROW_LENGTH (10.0*GP_ONE_POINT)
#define GA_DEFAULT_ARROW_WIDTH (4.0*GP_ONE_POINT)
};

typedef struct _ga_vect_attribs ga_vect_attribs_t;
struct _ga_vect_attribs {
  int hollow;      /* 1 to outline darts, 0 to fill
                      (uses current line or filled area attibutes) */
  gp_real_t aspect;   /*  half-width/length of dart arrows */
};

struct _ga_attributes {

  /* line attributes (gp_draw_lines, gp_draw_disjoint) */
  gp_line_attribs_t l;

  /* marker attributes (gp_draw_markers) */
  gp_marker_attribs_t m;

  /* filled area attributes (gp_draw_fill) */
  gp_fill_attribs_t f;

  /* text attributes (gp_draw_text) */
  gp_text_attribs_t t;

  /* decorated line attributes (ga_draw_lines) */
  ga_line_attribs_t dl;

  /* vector attributes (GpVectors) */
  ga_vect_attribs_t vect;

  /* edge attributes -- intended for 3D extensions (gp_draw_fill) */
  gp_line_attribs_t e;

  int rgb;  /* for gp_draw_cells */
};

PL_EXTERN ga_attributes_t ga_attributes;

typedef struct _ga_axis_style ga_axis_style_t;
struct _ga_axis_style {
#define GA_TICK_LEVELS 5
  gp_real_t nMajor, nMinor, logAdjMajor, logAdjMinor;
  int nDigits, gridLevel;
  int flags;   /* GA_TICK_L, ... GA_LABEL_L, ... GA_GRID_F below */

  gp_real_t tickOff, labelOff;  /* offsets in NDC from the edge of the
                                viewport to the ticks or labels */
  gp_real_t tickLen[GA_TICK_LEVELS];  /* tick lengths in NDC */

  gp_line_attribs_t tickStyle, gridStyle;
  gp_text_attribs_t textStyle;   /* alignment ignored, set correctly */
  gp_real_t xOver, yOver;       /* position for overflow label */

/* Flags determine whether there are ticks at the lower or upper
   (left or bottom is lower) edges of the viewport, whether the ticks
   go inward or outward, whether the lower or upper edge ticks have
   labels, and whether there is a full grid, or a single grid line
   at the origin.
   Also whether an alternative tick and/or label generator is used.  */
#define GA_TICK_L 0x001
#define GA_TICK_U 0x002
#define GA_TICK_C 0x004
#define GA_TICK_IN 0x008
#define GA_TICK_OUT 0x010
#define GA_LABEL_L 0x020
#define GA_LABEL_U 0x040
#define GA_GRID_F 0x080
#define GA_GRID_O 0x100
#define GA_ALT_TICK 0x200
#define GA_ALT_LABEL 0x400
};

typedef struct _ga_tick_style ga_tick_style_t;
struct _ga_tick_style {
  ga_axis_style_t horiz, vert;
  int frame;
  gp_line_attribs_t frameStyle;
};

/* ------------------------------------------------------------------------ */

/*
  GIST includes a few output routines at a higher level than GKS.
 */

PL_EXTERN int ga_draw_lines(long n, const gp_real_t *px, const gp_real_t *py);
       /* Like gp_draw_lines, but includes ga_attributes_t fancy line attributes
          as well as the line attributes from GpAttributes.  */

/* You must load the components of a mesh into a GaMeshXY data structure
   before using the A level routines that require a mesh.  Only the
   contour routines use the triangle array.  If you don't supply a
   reg array (mesh->reg==0), ga_draw_mesh, ga_fill_mesh ga_draw_vectors, or
   ga_init_contour will supply a default region number array for you.
   YOU ARE RESPONSIBLE FOR RELEASING THE ASSOCIATED STORAGE (using free).
   Failure to supply a triangle array may result in crossing contours
   in GaContours, but ga_init_contour will not allocate one.  */
typedef struct _ga_mesh ga_mesh_t;
struct _ga_mesh {
  long iMax, jMax; /* mesh logical dimensions */
  gp_real_t *x, *y;   /* iMax-by-jMax mesh coordinate arrays */
  int *reg;        /* iMax-by-(jMax+1)+1 mesh region number array:
                      reg[j][i] non-zero means that the zone bounded by
                        [j-1][i-1], [j-1][i], [j][i], and [j][i-1]
                        exists,
                      reg[0][i]= reg[j][0]= reg[jMax][i]= reg[j][iMax]= 0,
                        so that the (iMax-1)-by-(jMax-1) possible zones
                        are surrounded by non-existent zones.  */
  short *triangle; /* iMax-by-jMax triangulation marker array
                      triangle[j][i]= 1, -1, or 0 as the zone bounded by
                        [j-1][i-1], [j-1][i], [j][i], and [j][i-1]
                      has been triangulated from (-1,-1) to (0,0),
                      from (-1,0) to (0,-1), or has not yet been
                      triangulated.  */
};

PL_EXTERN int ga_draw_mesh(ga_mesh_t *mesh, int region, int boundary, int inhibit);
       /* Plots the quadrilateral mesh.  If boundary==0,
          a mesh line is plotted whenever either zone it borders
          belongs to the given region.  An exception is made if
          region==0; in this case the entire mesh is plotted.
          If boundary!=0, a mesh line is plotted whenever one but
          not both of its bordering zones belong to the given region.
          Again, if region==0, the boundary for the whole mesh is
          plotted.
          If inhibit&1, then lines at constant j are not drawn;
          if inhibit&2, then lines at constant i are not drawn.  */

PL_EXTERN int ga_fill_mesh(ga_mesh_t *mesh, int region, const gp_color_t *colors,
                        long nColumns);
       /* Fills each zone of the quadrilateral mesh according to the
          (iMax-1)-by-(jMax-1) array of colors.  The colors array may
          be a subarray of a larger rectangular array; therefore, you
          must specify the actual length of a row of the colors array
          using the nColumns parameter (nColumns>=iMax-1 for sensible
          results).  Only the region specified is plotted, unless
          region==0, in which case all non-zero regions are filled.
          (As a special case, if colors==0, every zone is filled with
          ga_attributes.f.color.)  If ga_attributes.e.type!=GP_LINE_NONE, an edge is drawn
          around each zone as it is drawn.  */

PL_EXTERN int ga_fill_marker(long n, const gp_real_t *px, const gp_real_t *py,
                          gp_real_t x0, gp_real_t y0);
       /* Fills (px,py)[n] in NDC with origin at (x0,y0) in world.  */

PL_EXTERN int ga_draw_vectors(ga_mesh_t *mesh, int region,
                       const gp_real_t *u, const gp_real_t *v, gp_real_t scale);
       /* Plot a vector scale*(u,v) at each point of the current
          mesh (ga_attributes.mesh).  If region==0, the entire mesh is
          drawn, otherwise only the specified region.  */

PL_EXTERN int ga_init_contour(ga_mesh_t *mesh, int region,
                           const gp_real_t *z, gp_real_t level);
       /* Find the edges cut by the current contour, remembering z, mesh
          region, and level for the ga_draw_contour routine, which actually
          walks the contour.  The z array represents function values at
          the mesh points.  If region==0, contours are drawn on the
          entire mesh, otherwise only on the specified region.
          The mesh->triangle array is updated by ga_draw_contour as follows:
          If a contour passes through an untriangulated saddle zone,
          ga_draw_contour chooses a triangulation and marks the triangle
          array approriately, so that subsequent calls to ga_draw_contour
          will never produce intersecting contour curves.
          If mesh->triangle==0 on input to ga_init_contour, the
          curves returned by ga_draw_contour may intersect the curves
          returned after a subsequent call to ga_init_contour with a
          new contour level.  */

PL_EXTERN int ga_draw_contour(long *cn, gp_real_t **cx, gp_real_t **cy, int *closed);
       /* After a call to ga_init_contour, ga_draw_contour must be called
          repeatedly to generate the sequence of curves obtained by
          walking the edges cut by the contour level plane.  ga_draw_contour
          returns 1 until there are no more contours to be plotted,
          when it returns 0.  ga_draw_contour signals an error by returning
          0, but setting *cn!=0.  The curve coordinates (*cx,*cy)
          use internal scratch space and the associated storage must
          not be freed.  (*cx, *cy) are valid only until the next
          ga_draw_contour or ga_draw_mesh call.  */

PL_EXTERN int ga_draw_ticks(ga_tick_style_t *ticks, int xIsLog, int yIsLog);
       /* Draws a system of tick marks and labels for the current
          transformation (gp_transform), according to the specified
          tick style.  */

PL_EXTERN int ga_free_scratch(void);
       /* Frees scratch space required by ga_draw_mesh, ga_draw_contour,
          and ga_init_contour, and ga_draw_ticks.  Ordinarily, there is no reason
          to call this, as the amount of scratch space required is modest
          in comparison to the size of the mesh, and it is reused
          in subsequent calls.  */

typedef int ga_ticker_t(gp_real_t lo, gp_real_t hi, gp_real_t nMajor, gp_real_t nMinor,
                       gp_real_t *ticks, int nlevel[GA_TICK_LEVELS]);
      /* An alternative tick generator function accepts the lo and hi
         axis limits (lo<hi) and the maximum total number of ticks
         allowed for all levels of the hierarchy.  It is given space
         ticks[ceil(nMinor)] to return the tick values, which are given
         in order within each level -- that is, all level 0 ticks first,
         followed by all level 1 ticks, then level 2, and so on.  The
         elements of nlevel array should be set to the number of ticks
         ticks returned in ticks up to that level.  On entry, nlevel[i]
         is set to zero for each i.  On exit, max(nlevel)<=maxnum.
         The function should return 0 if successful.  If it returns
         non-zero, the default tick generation scheme will be used.  */
typedef int ga_labeler_t(char *label, gp_real_t value);
      /* An alternative label generator stores label (<=31 characters)
         in label, which is to represent the given value.  The function
         should return 0 on success, non-zero to drop back to the default
         scheme.  If label==0 on input, the function should return the
         correct success value without actually computing the label;
         it will be called once in this mode for every value, and if it
         fails for any value, the default scheme will be used instead.  */

PL_EXTERN int ga_base60_ticker(gp_real_t lo, gp_real_t hi, gp_real_t nMajor, gp_real_t nMinor,
                         gp_real_t *ticks, int nlevel[GA_TICK_LEVELS]);
     /* Puts ticks at multiples of 30, failing if lo<=-3600 or hi>=+3600.
        For ticks at multiples of 10 or less, the subdivisions are
        identical to the default decimal tick scheme.  */
PL_EXTERN int ga_degree_labeler(char *label, gp_real_t value);
     /* Prints (value+180)%360-180 instead of just value.  */
PL_EXTERN int ga_hour_labeler(char *label, gp_real_t value);
     /* Prints hh:mm (or mm:ss) for value 60*hh+mm (or 60*mm+ss).  */

PL_EXTERN int ga_draw_alt_ticks(ga_tick_style_t *ticks, int xIsLog, int yIsLog,
                       ga_ticker_t *xtick, ga_labeler_t *xlabel,
                       ga_ticker_t *ytick, ga_labeler_t *ylabel);
       /* Like ga_draw_ticks, but uses the specified ticker and labeler for
          non-log axes.  Log axes always use the defaults.  Either
          ticker or labeler or both may be zero to use the default.  */

/* ------------------------------------------------------------------------ */

/* These general purpose contouring routines actually have nothing to
 * do with graphics; I provide them as a convenience for computing
 * inputs to the ga_draw_lines and gp_draw_fill routines.
 * gp_cont_init1 takes the same inputs as ga_init_contour.  It returns nparts,
 * the number of disjoint contour curves, and its return value is the
 * total number of points on all these parts.  (On closed curves, the
 * start point will be repeated, and so has been counted twice.)
 * gp_cont_init2 accepts two level values instead of one; it returns the
 * boundary of the region between these two contour levels, with
 * slits cut as necessary to render each disjoint part simply connected,
 * traced counterclockwise (in logical space) about the region between
 * the levels.  If you specify a non-zero value for nchunk, then the
 * mesh will be chunked into pieces of no more than nchunk^2 zones, which
 * limits the number of sides of any returned polygon to about 4*nchunk^2.
 * After you call gp_cont_init1 or gp_cont_init2, you must allocate px[ntotal],
 * py[ntotal], and n[nparts] arrays, then call gp_cont_trace to fill them.
 */
PL_EXTERN long gp_cont_init1(ga_mesh_t *mesh, int region, const gp_real_t *zz,
                      gp_real_t lev, long *nparts);
PL_EXTERN long gp_cont_init2(ga_mesh_t *mesh, int region, const gp_real_t *zz,
                      gp_real_t levs[2], long nchunk, long *nparts);
PL_EXTERN long gp_cont_trace(long *n, gp_real_t *px, gp_real_t *py);

/* ------------------------------------------------------------------------ */
/* Display list routines */

/* GIST maintains a list of drawings much like its list of engines.
   Unlike engines, only one drawing is active at a time.  The
   contents of the drawing can be rendered on all active engines by
   calling gd_draw.  This clears each engine before it begins to draw,
   so that each call to gd_draw results in one page of graphics output
   on all active engines.  X window engines may remember the most recent
   drawing rendered on them; they will then respond to the GdUpdate
   command by repairing any damage or making any additions to the
   window containing the drawing.  On metafile engines, GdUpdate is
   a noop.

   A drawing consists of a list of coordinate systems (a GKS normalization
   transformation plus GIST ticks and labels and a list of elements to be
   displayed), plus a list of graphical elements outside any coordinate
   systems (such as plot titles or legends).  The general layout of the
   coordinate systems (their viewport and tick and label style) can be
   specified using gd_new_system, but the preferred technique is to put
   this data into a "style sheet", which is an ASCII file.  */

typedef struct _gd_drawing gd_drawing_t;

PL_EXTERN gd_drawing_t *gd_new_drawing(char *gsFile);
PL_EXTERN void gd_kill_drawing(gd_drawing_t *drawing);

/* After gd_new_drawing, the new drawing becomes the current drawing.
   gd_set_drawing can be used to change the current drawing.  This
   action saves the state of the previous drawing (the current system,
   element, etc.), which can be recalled using gd_set_drawing(0).
   Otherwise, gd_set_drawing scans the drawing to find the latest
   system, element, and mesh, and these become current.  */
PL_EXTERN int gd_set_drawing(gd_drawing_t *drawing);

/* gd_clear marks the drawing as cleared, but no action is taken until
   something new is drawn, at which point the cleared display list
   elements are discarded.  gp_clear is not sent until the next
   gd_draw.  If drawing is 0, the current drawing is assumed.  */
PL_EXTERN int gd_clear(gd_drawing_t *drawing);

/* If changesOnly is non-zero, only new elements or damaged areas
   are redrawn.  The amount drawn may be recorded by each engine, and
   may vary from engine to engine.
   The gd_drawing_t* is also recorded on each engine.  */
PL_EXTERN int gd_draw(int changesOnly);

/* Graphical elements are added to the current drawing by setting attributes in
   ga_attributes, then calling gd_add_lines, gd_add_disjoint, gd_add_cells,
   gd_add_mesh, gd_add_fillmesh, gd_add_vectors, or gd_add_contours.
   gd_add_text puts the text outside all coordinate systems unless toSys is
   non-0.  Each of these routines returns a unique identification number (equal
   to the total number of objects defined in the current drawing since the last
   gd_clear).  They return -1 if an error occurs.  */

PL_EXTERN int gd_add_lines(long n, const gp_real_t *px, const gp_real_t *py);
PL_EXTERN int gd_add_disjoint(long n, const gp_real_t *px, const gp_real_t *py,
                        const gp_real_t *qx, const gp_real_t *qy);
PL_EXTERN int gd_add_text(gp_real_t x0, gp_real_t y0, const char *text, int toSys);
PL_EXTERN int gd_add_cells(gp_real_t px, gp_real_t py, gp_real_t qx, gp_real_t qy,
                     long width, long height, long nColumns,
                     const gp_color_t *colors);
PL_EXTERN int gd_add_fill(long n, const gp_color_t *colors, const gp_real_t *px,
                    const gp_real_t *py, const long *pn);

/* The other D level primitives involve mesh arrays.  It is often
   convenient NOT to copy mesh data, so that several objects may
   point to a single mesh array (strictly speaking, it is the mesh->x,
   mesh->y, mesh->reg, and mesh->triangle arrays which will be copied
   or not copied).  Therefore, a noCopy flag is provided, which can be
   constructed by ORing together GD_NOCOPY_MESH, etc.  If you set any of
   these bits, you are promising that you will not free the corresponding
   object for the lifetime of this display list object, conversely,
   when Gist discards the object, it will not free the pointers (the
   memory management responsibility is yours).  However, if you
   supply a gd_free routine, Gist will call that for uncopied objects.  */
PL_EXTERN int gd_add_mesh(int noCopy, ga_mesh_t *mesh, int region, int boundary,
                    int inhibit);
PL_EXTERN int gd_add_fillmesh(int noCopy, ga_mesh_t *mesh, int region,
                        gp_color_t *colors, long nColumns);
PL_EXTERN int gd_add_vectors(int noCopy, ga_mesh_t *mesh, int region,
                       gp_real_t *u, gp_real_t *v, gp_real_t scale);
PL_EXTERN int gd_add_contours(int noCopy, ga_mesh_t *mesh, int region,
                        gp_real_t *z, const gp_real_t *levels, int nLevels);
#define GD_NOCOPY_MESH 1
#define GD_NOCOPY_COLORS 2
#define GD_NOCOPY_UV 4
#define GD_NOCOPY_Z 8

/* graphical element types */
#define GD_ELEM_NONE 0
#define GD_ELEM_LINES 1
#define GD_ELEM_DISJOINT 2
#define GD_ELEM_TEXT 3
#define GD_ELEM_MESH 4
#define GD_ELEM_FILLED 5
#define GD_ELEM_VECTORS 6
#define GD_ELEM_CONTOURS 7
#define GD_ELEM_CELLS 8
#define GD_ELEM_POLYS 9
#define GD_ELEM_SYSTEM 10

/* Properties of drawing elements are similar to attributes of
   graphical primitives.  The properties include everything passed
   in the parameter lists to the primitives.  The global property
   list gd_properties is used to inquire about or change these values in
   the drawing elements-- i.e.- to edit the drawing.  */

typedef struct _gd_properties gd_properties_t;
struct _gd_properties {
  /* Properties of all elements */
  int hidden;           /* 1 if element should not be plotted */
  char *legend;         /* description of this element */

  /* Properties of coordinate systems */
  ga_tick_style_t ticks;    /* for ga_draw_ticks to produce ticks and labels */
  gp_transform_t trans;    /* transform for gp_set_transform for current system */
  int flags;            /* flags for computing the limits */
#define GD_LIM_XMIN 0x001
#define GD_LIM_XMAX 0x002
#define GD_LIM_YMIN 0x004
#define GD_LIM_YMAX 0x008
#define GD_LIM_RESTRICT 0x010
#define GD_LIM_NICE 0x020
#define GD_LIM_SQUARE 0x040
#define GD_LIM_LOGX 0x080
#define GD_LIM_LOGY 0x100
#define GD_LIM_ZOOMED 0x200
  gp_box_t limits;         /* current trans->window or exp10(trans->window) */

  /* Properties of elements */
  /* --- gd_add_lines- also uses ga_attributes.l, ga_attributes.dl, ga_attributes.m, gd_add_fill */
  int n;
  gp_real_t *x, *y;
  /* --- gd_add_disjoint */
  gp_real_t *xq, *yq;
  /* --- gd_add_text- also uses ga_attributes.t */
  gp_real_t x0, y0;
  char *text;
  /* --- gd_add_cells */
  gp_real_t px, py, qx, qy;
  long width, height;
  /* --- gd_add_cells, gd_add_fillmesh */
  long nColumns;         /* if colors copied, this is width or iMax-1 */
  gp_color_t *colors;       /* also gd_add_fill */
  /* --- gd_add_mesh, gd_add_fillmesh, gd_add_vectors, gd_add_contours */
  int noCopy;  /* if bit is set, Gist will not free corresponding array
                  -- however, if non-0, gd_free will be called */
/* if GD_NOCOPY_MESH was set, but reg or triangle was 0, then gist
   owns the reg or triangle array, and the following bits are NOT set: */
#define GD_NOCOPY_REG 16
#define GD_NOCOPY_TRI 32
  ga_mesh_t mesh;
  int region;
  /* --- gd_add_mesh- also uses ga_attributes.l */
  int boundary, inhibit;
  /* --- gd_add_vectors- also uses ga_attributes.l, ga_attributes.f, ga_attributes.vect */
  gp_real_t *u, *v, scale;
  /* --- gd_add_contours- individual level curves are like gd_add_lines
          contour itself uses ga_attributes.l, ga_attributes.dl, ga_attributes.m as defaults */
  gp_real_t *z, *levels;
  int nLevels;
  /* --- gd_add_fill */
  long *pn;
};

PL_EXTERN gd_properties_t gd_properties;

/* gd_set_system, gd_set_element, and gd_set_contour cause the contents
   of one drawing element to be copied into gd_properties and ga_attributes for
   examination or editing (with gd_edit or gd_remove).
   gd_new_system does an automatic gd_set_system, and gd_add_lines,
   gd_add_text, gd_add_cells, gd_add_mesh, gd_add_fillmesh, gd_add_vectors, and gd_add_contours
   do an automatic gd_set_element to the newly created element.
   The return values are GD_ELEM_LINES, ... GD_ELEM_SYSTEM; GD_ELEM_NONE on failure.  */
PL_EXTERN int gd_set_system(int sysIndex);
PL_EXTERN int gd_set_element(int elIndex);
PL_EXTERN int gd_set_contour(int levIndex);

/* return current sysIndex, or <0 if unavailable */
PL_EXTERN int gd_get_system(void);

/* The elIndex required by gd_set_element is NOT the id number returned
   by gd_add_lines, gd_add_mesh, etc.  Instead, elIndex begins at 0 and goes
   up to 1 less than the number of elements in the current system.
   To get the elIndex corresponding to a given id, use gd_find_index(id).
   The sysIndex varies from 0 (outside of all systems), up to the
   total number of coordinate systems defined in the current gd_drawing_t.
   To find the sysIndex for a given element id, use gd_find_system(id).  */
PL_EXTERN int gd_find_index(int id);  /* returns -1 if no such element
                                      if current system */
PL_EXTERN int gd_find_system(int id); /* returns -1 if no such element
                                      anywhere in current gd_drawing_t */

/* After you call one of the 3 GdSet... routines, you may
   alter the contents of ga_attributes and/or gd_properties, then call
   gd_edit to write the changes back into the element.
   If you change gd_properties.x, gd_properties.y, gd_properties.mesh->x, or gd_properties.mesh->y,
   use gd_edit(GD_CHANGE_XY) (this recomputes log coordinates if necessary),
   otherwise, use gd_edit(0).  If you want to change any of the
   pointers, you should first free the existing pointer (with free),
   and be aware that gd_kill_drawing, gd_clear, or gd_remove will
   free the pointer you are providing.  If the element is a set of
   contours, and you change gd_properties.z or gd_properties.levels, gd_properties.nLevels, or
   gd_properties.mesh->triangle, use gd_edit(GD_CHANGE_Z).
   Use gd_edit(GD_CHANGE_XY | GD_CHANGE_Z) if both apply.
   gd_remove completely removes the element from the drawing.  */
PL_EXTERN int gd_edit(int xyzChanged);
#define GD_CHANGE_XY 1
#define GD_CHANGE_Z 2
PL_EXTERN int gd_remove(void);

/* gd_get_limits copies the current coordinate systems limits into
   gd_properties.limits and gd_properties.flags.  gd_set_limits sets the limits in
   the current coordinate system to gd_properties.limits and gd_properties.flags
   (if the various adjustment flags are set, the window limits
   may be adjusted, but this will not happen until the drawing
   is rendered on at least one engine).  gd_set_viewport sets the tick
   style and viewport for the current coordinate system.  */
PL_EXTERN int gd_get_limits(void);
PL_EXTERN int gd_set_limits(void);
PL_EXTERN int gd_set_viewport(void);

/* Each coordinate system keeps a "saved" set of limits, which
   can be "reverted" to return the limits to the state at the time
   of the last save operation.  (gd_properties plays no role in this operation.)
   This is used by the fancy X gp_engine_t to save the limits prior to
   a series of mouse-driven zoom or pan operations.
   gd_revert_limits(1) reverts to the save limits only if the current
   limits are marked GD_LIM_ZOOMED, while gd_revert_limits(0) reverts
   unconditionally.  gd_save_limits(1) assures that GD_LIM_ZOOMED is not
   set in the saved flags.  */
PL_EXTERN int gd_save_limits(int resetZoomed);
PL_EXTERN int gd_revert_limits(int ifZoomed);

/* You can register an alternative tick and/or label generator for
   each axis of the current corodinate system.  They will not be used
   unless the GA_ALT_TICK or GA_ALT_LABEL limits flag is set.  Passing 0
   to gd_set_alt_ticker leaves the corresponding function unchanged -- there
   is no way to unregister it (just turn off the limits flag).  */
PL_EXTERN int gd_set_alt_ticker(ga_ticker_t *xtick, ga_labeler_t *xlabel,
                       ga_ticker_t *ytick, ga_labeler_t *ylabel);

/* Unlike the other `D' level drawing routines, gd_draw_legends acts
   immediately.  If engine==0, all active engines are used.
   The intent is that legends not be rendered in an X window or
   other interactive device, but this optional routine can render
   them on a hardcopy device.  */
PL_EXTERN int gd_draw_legends(gp_engine_t *engine);

/* ------------------------------------------------------------------------ */
/* The following Gd routines are not intended for ordinary use:
   gd_read_style, gd_new_system, gd_set_landscape, and gd_set_legend_box are
   unnecessary if you use drawing style files (see gread.c).
   gd_detach is provided here for completeness; several of the
   other Gd routines use it (e.g.- gd_clear).
   gd_clear_system is required for the specialized animation mode
   described in hlevel.c.  */

/* gd_read_style clears the drawing and removes all its current systems
   as a side effect.  It is the worker for gd_new_drawing.  */
PL_EXTERN int gd_read_style(gd_drawing_t *drawing, const char *gsFile);

/* Coordinate systems are usually defined in the styleFile, but
   a new coordinate system may be added with gd_new_system, which
   returns the index of the newly created system.  */
PL_EXTERN int gd_new_system(gp_box_t *viewport, ga_tick_style_t *ticks);

/* Set current drawing to landscape or portrait mode */
PL_EXTERN int gd_set_landscape(int landscape);

/* Set location and properties of legends for output.   */
PL_EXTERN int gd_set_legend_box(int which, gp_real_t x, gp_real_t y, gp_real_t dx, gp_real_t dy,
                         const gp_text_attribs_t *t, int nchars, int nlines,
                         int nwrap);

/* gd_detach detaches one or all (if drawing==0) drawings from one or all
   (if engine==0) engines.  Whether an engine is active or not is
   unimportant.  A drawing is attached to an engine by gd_draw.  */
PL_EXTERN void gd_detach(gd_drawing_t *drawing, gp_engine_t *engine);

/* Clear the current coordinate system-- returns 0 if there is no
   current system.  Damages viewport only if all limits fixed,
   otherwise damages both ticks and viewport.  Returns damaged box.  */
PL_EXTERN gp_box_t *gd_clear_system(void);

/* ------------------------------------------------------------------------ */
/* Color */

/* GIST cell arrays and filled meshes are aimed at displaying pseudocolor
   images, not true color images.  (Another primitive might be added to
   handle true color images.  This is probably most useful for shading
   projections of 3D objects, which requires additional primitives as
   anyway.)  The colors arrays in the G*Cells and G*FillMesh primitives
   are arrays of indices into a pseudocolor map.  The colors for the
   various attribute lists may come from this pseudocolor map, or
   they may be special values representing the foreground and background
   colors, or a primary color.  */

/* Each gp_engine_t maintains a list of colors for the pseudocolor
   map.  These can be set or retrieved directly from the nColors and
   palette gp_engine_t members.  The pseudocolor map is in addition to
   the 10 standard colors defined in gist2.h -- note that the meaning of
   a colorcell need not be the same from one engine to another, although
   the 4th component (gray) has been added to make it easier to control
   both gray and color devices using the same pl_col_t array.  Note
   that no gp_engine_t owns a pl_col_t array; allocating and freeing this
   storage is the responsibility of the application program.

   A change in the colormap for an gp_engine_t will generally have no effect
   until the next page of graphics output.  However, individual types
   of engines may implement a function to try to make immediate
   changes to the pseudocolor map.  */

/* duplicate pl_col_t from play2.h */
typedef unsigned long pl_col_t;

/* Two routines are provided to set the gray component of the cells in
   a colormap--  gp_put_gray for gray=(red+green+blue)/3 and gp_put_ntsc
   for gray=(30*red+59*green+11*blue).  The latter is the NTSC standard
   for converting color TV signals to monochrome.  gp_put_rgb copies the
   gray entry into the red, green, and blue entries.  */
/* these all no-ops now */
PL_EXTERN void gp_put_gray(int nColors, pl_col_t *palette);
PL_EXTERN void gp_put_ntsc(int nColors, pl_col_t *palette);
PL_EXTERN void gp_put_rgb(int nColors, pl_col_t *palette);

/* Set the palette; the change takes place as soon as possible for
   this engine and the effect on previously drawn objects is
   engine dependent.  The maximum usable palette size is returned.  */
PL_EXTERN int gp_set_palette(gp_engine_t *engine, pl_col_t *palette, int nColors);

/* gp_get_palette returns the number of colors in the palette-
   this is the nColors passed to gp_set_palette.  */
PL_EXTERN int gp_get_palette(gp_engine_t *engine, pl_col_t **palette);

/* gp_read_palette returns the number of colors found in the palette
   file (see gread.c for format), as well as the palette itself.
   If the number of colors in the palette exceeds maxColors, attempts
   to scale to maxColors.  */
PL_EXTERN int gp_read_palette(gp_engine_t *engine, const char *gpFile,
                           pl_col_t **palette, int maxColors);

/* The PostScript and CGM formats, unlike Xlib, guarantee that
   the color table from one page does not automatically carry over to
   the next.  Additionally, color images cannot be rendered (and may be
   inconvenient if they can be rendered) on most hardcopy devices.
   Consequently, by default, Gist produces a simple gray scale for
   PostScript or CGM output.  However, you can optionally force the
   current palette to be dumped on each output page by setting the
   color mode flag.  If any marks have been made on the current page,
   the color table cannot be dumped until the next page.
   For an X gp_engine_t, the colorMode is initially 0, which means to
   use the best available shared colors.  Setting colorMode to 1
   results in exact private colors, which may require a private
   colormap.  */
PL_EXTERN int gp_dump_colors(gp_engine_t *engine, int colorMode);

/* ------------------------------------------------------------------------ */
/* Error handlers */

/* The Xlib functions invoked by X engines will call exit unless you
   set an alternative error recovery routine.  */
PL_EXTERN int gp_set_xhandler(void (*ErrHandler)(char *errMsg));

/* ------------------------------------------------------------------------ */
/* Memory management */

/* gd_free, if non-0, will be called to free objects
   marked with the noCopy flag.  */
PL_EXTERN void (*gd_free)(void *);

#endif /* _GIST2_H */
