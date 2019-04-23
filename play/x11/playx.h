/*
 * $Id: playx.h,v 1.1 2005-09-18 22:05:32 dhmunro Exp $
 * declare routines and structs to use an X11 server
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _NPLAYX_H
#define _NPLAYX_H 1

/* X11 implementation files include this instead of play2.h */
#include <play/win.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* point list for polylines, fills, dots, segments */
extern XPoint x_pt_list[2050];
extern int x_pt_count;

/* retrieve pl_win_t* given Window id number, Display* (for event handling)
 * - the pl_win_t context can be used to back up the hierarchy further
 * - this could be implemented using XContext mechanism */
extern pl_x_display_t *_pl_x_dpy(Display *dpy);
extern pl_win_t *_pl_x_pwin(pl_x_display_t *xdpy, Drawable d);

/* ordinary and I/O X error handlers */
extern int _pl_x_err_handler(Display *dpy, XErrorEvent *event);
extern int _pl_x_panic(Display *dpy);
extern void (*x_on_panic)(pl_scr_t *s);

/* arrange to deliver X events for an X window to the event
 * handler for the corresponding pl_win_t
 * this is virtual to allow a simpler mode in case pl_gui is never called */
extern void (*x_wire_events)(pl_x_display_t *xdpy, int disconnect);

/* routines to convert Gist colors, linestyles, and fonts to X11 */
extern XFontStruct *_pl_x_font(pl_x_display_t *xdpy, int font, int pixsize);
extern void _pl_x_clip(Display *dpy, GC gc, int x0, int y0, int x1, int y1);
extern GC _pl_x_getgc(pl_scr_t *s, pl_win_t *w, int fillstyle);
extern pl_col_t _pl_x_getpixel(pl_win_t *w, pl_col_t color);
extern void _pl_x_nuke_shared(pl_scr_t *s);

/* optional X resource values (class Gist) */
extern char *x_xfont;       /* boldfont, font, Font */
extern char *x_foreground;  /* foreground, Foreground */
extern char *x_background;  /* background, Background */
extern char *x_guibg;       /* guibg */
extern char *x_guifg;       /* guifg */
extern char *x_guihi;       /* guihi */
extern char *x_guilo;       /* guilo */

extern Cursor _pl_x_cursor(pl_scr_t *s, int cursor);

/* simple destructors that zero input pointer */
extern void _pl_x_rotzap(pl_scr_t *s);
extern void _pl_x_tmpzap(void *ptmp);   /* _pl_x_tmpzap(anytype **ptmp) */
extern void _pl_x_gczap(Display *dpy, GC *pgc);
extern void _pl_x_imzap(pl_scr_t *s);
extern void _pl_x_pxzap(Display *dpy, Pixmap *ppx);
extern void _pl_x_cmzap(Display *dpy, Colormap *pcm);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _NPLAYX_H */
