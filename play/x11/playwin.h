/*
 * $Id: playwin.h,v 1.1 2005-09-18 22:05:34 dhmunro Exp $
 * platform-dependent exposure of pl_win_t struct, X11 version
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _PLAY_WIN_H
#define _PLAY_WIN_H 1

#include <play2.h>
#include <play/hash.h>
#include <X11/Xlib.h>

#define PL_FONTS_CACHED 6

/* the Display struct may be shared among several root windows,
 * in the unusual case that a single server drives several screens
 * - fonts, however, are shared among all screens of a server
 *   (I ignore a possible exception of the default font, which could
 *    in principle differ from screen to screen on a server -- this
 *    may be a severe problem if the two screens differ greatly in
 *    resolution, so that what is legible on one is not on the other
 *    -- hopefully this is even rarer than multiple screens) */

typedef struct _pl_x_display pl_x_display_t;
struct _pl_x_display {
  int panic;
  pl_scr_t *screens;       /* list of screens on this server (for panic) */
  pl_x_display_t *next;      /* list of all servers */
  Display *dpy;

  Atom wm_protocols, wm_delete;
  pl_hashtab_t *id2pwin;   /* use phash instead of XContext */

  XFontStruct *font;    /* default font to use on this server */
  int unload_font;      /* non-0 if font must be unloaded */

  struct {
    XFontStruct *f;
    int font, pixsize, next;
  } cached[PL_FONTS_CACHED];
  int most_recent;

  struct {
    int nsizes, *sizes;
    char **names;
  } available[20];

  Cursor cursors[14];

  /* number of motion events queued when previous motion callback
   * completed -- all but the last will be skipped */
  int motion_q;

  unsigned int meta_state, alt_state;  /* masks for modifier keys */

  /* selection data */
  pl_win_t *sel_owner;
  char *sel_string;

  /* menu count for pointer grabs */
  int n_menus;
};

extern pl_x_display_t *_pl_x_displays;
typedef struct _pl_x_cshared pl_x_cshared_t;

struct _pl_scr {
  pl_x_display_t *xdpy;  /* may be accessing multiple screens on server */
  pl_scr_t *next;       /* keep list of all screens on this server */

  int scr_num;          /* screen number on this server */
  Window root;          /* root window on this screen */
  int width, height, depth;  /* of root window */
  int nwins;            /* window count */

  int vclass;           /* visual class for this screen */
  /* pixels==0 (part of pl_win_t) for PseudoColor visual
   * for all others, points to pixels[256] */
  pl_col_t *pixels;
  /* red, green, and blue masks for TrueColor and DirectColor */
  pl_col_t rmask, gmask, bmask;
  Colormap cmap;        /* None except for GrayScale and DirectColor */

  XColor colors[14];    /* standard colors */
  int free_colors;      /* bit flags for which ones need to be freed */
  Pixmap gray;          /* in case PL_GRAYA-PL_GRAYD are stipples */
  int gui_flags;        /* marker for stippled grays */
  pl_x_cshared_t *shared;    /* tables for shared PseudoColor colors */

  /* generic graphics context and its current state */
  GC gc;
  pl_col_t gc_color;
  int gc_fillstyle;     /* invalid for stippled colors, see colors.c */
  pl_win_t *gc_w_clip;     /* gc contains clipping for this window */
  int gc_width, gc_type;
  int gc_font, gc_pixsize;

  /* temporaries required for rotating fonts (see textout.c) */
  void *tmp;
  XImage *image;
  int own_image_data;
  Pixmap pixmap;
  GC rotgc;
  int rotgc_font, rotgc_pixsize, rotgc_orient;
};

struct _pl_win {
  void *context;     /* application context for event callbacks */
  pl_scr_t *s;

  Drawable d;
  pl_win_t *parent;     /* non-0 only for offscreen pixmaps */
  int is_menu;       /* non-0 only for menus */

  Colormap cmap;
  pl_col_t *pixels, *rgb_pixels;
  int n_palette;  /* number of pixels[] belonging to palette */
  int x, y, width, height, xyclip[4];
};

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* need to expose this for use by oglx.c */
PL_EXTERN int _pl_x_rgb_palette(pl_win_t *w);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _PLAY_WIN_H */
