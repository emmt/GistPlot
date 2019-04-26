/*
 * $Id: play2.h,v 1.4 2010-04-08 10:53:49 thiebaut Exp $
 * portability layer programming model declarations
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _PLAY2_H
#define _PLAY2_H 1

#include <play/extern.h>

PL_BEGIN_EXTERN_C

/* application entry point */
extern int pl_on_launch(int argc, char *argv[]);

/* main event loop control and system services */
PL_EXTERN void pl_quit(void);
PL_EXTERN void pl_abort(void);               /* never returns to caller */
PL_EXTERN void pl_qclear(void);              /* clears event queue */
PL_EXTERN void pl_stdout(char *output_line); /* only after pl_stdinit */
PL_EXTERN void pl_stderr(char *output_line); /* only after pl_stdinit */
PL_EXTERN double pl_wall_secs(void);         /* must interoperate with on_poll */
PL_EXTERN double pl_cpu_secs(double *sys);
/* pl_getenv and pl_getuser return pointers to static memory */
PL_EXTERN char *pl_getenv(const char *name);
PL_EXTERN char *pl_getuser(void);

/* dont do anything critical if this is set -- call pl_abort */
PL_EXTERN volatile int pl_signalling;

/* turn off SIGFPE handling (for libraries which require that, like OpenGL)
 * pl_fpehandling(0) masks SIGFPE, pl_fpehandling(1) restores SIGFPE handling
 * multiple succesive calls to pl_fpehandling(1) require an equal number of
 *   calls pl_fpehandling(1) restore
 * pl_fpehandling(2) restores SIGFPE handling no matter how many calls
 *   to pl_fpehandling(1) have been made
 * requires fenv.h
 */
PL_EXTERN void pl_fpehandling(int on);
/* partial support for floating point exceptions in FPU_IGNORE environments
 * pl_softfpe() raises SIGFPE if fpu exception bits are set
 * requires fenv.h
 */
#ifdef USE_SOFTFPE
#define PL_SOFTFPE_TEST pl_softfpe()
#else
#define PL_SOFTFPE_TEST
#endif
PL_EXTERN void pl_softfpe(void);

/* data structures (required for screen graphics only)
 * - pl_scr_t represents a screen (plus keyboard and mouse)
 * - pl_win_t represents a window on a screen
 * all are opaque to platform independent code */
typedef struct _pl_scr pl_scr_t;
typedef struct _pl_win pl_win_t;
typedef unsigned long pl_col_t;

/* routines to establish callback functions for various events
 * - application calls these either once or never */
PL_EXTERN void pl_quitter(int (*on_quit)(void));
PL_EXTERN void pl_stdinit(void (*on_stdin)(char *input_line));
PL_EXTERN void pl_handler(void (*on_exception)(int signal, char *errmsg));
PL_EXTERN void pl_gui(void (*on_expose)(void *c, int *xy),
                    void (*on_destroy)(void *c),
                    void (*on_resize)(void *c,int w,int h),
                    void (*on_focus)(void *c,int in),
                    void (*on_key)(void *c,int k,int md),
                    void (*on_click)(void *c,int b,int md,int x,int y,
                                     unsigned long ms),
                    void (*on_motion)(void *c,int md,int x,int y),
                    void (*on_deselect)(void *c),
                    void (*on_panic)(pl_scr_t *screen));
PL_EXTERN void pl_gui_query(void (**on_expose)(void *c, int *xy),
			  void (**on_destroy)(void *c),
			  void (**on_resize)(void *c,int w,int h),
			  void (**on_focus)(void *c,int in),
			  void (**on_key)(void *c,int k,int md),
			  void (**on_click)(void *c,int b,int md,int x,int y,
					    unsigned long ms),
			  void (**on_motion)(void *c,int md,int x,int y),
			  void (**on_deselect)(void *c),
			  void (**on_panic)(pl_scr_t *screen));
PL_EXTERN void (*pl_on_connect)(int dis, int fd);

/* asynchronous subprocesses using callbacks (see also pl_popen, pl_system) */
typedef struct _pl_spawn pl_spawn_t;
PL_EXTERN pl_spawn_t *pl_spawn(char *name, char **argv,
                          void (*callback)(void *ctx, int err),
                          void *ctx, int err);
PL_EXTERN long pl_recv(pl_spawn_t *proc, char *msg, long len);
/* in pl_send msg=0, len<0 sends signal -len */
PL_EXTERN int pl_send(pl_spawn_t *proc, char *msg, long len);
PL_EXTERN void pl_spawf(pl_spawn_t *proc, int nocallback);

/* socket interface */
typedef struct _pl_sckt pl_sckt_t;
typedef void pl_sckt_cb_t(pl_sckt_t *sock, void *ctx);
/* two kinds of sockets -- listener and data
 *   - sock argument to accept must be a listener socket
 *   - sock argument to send, recv must be a data socket
 *   - listen returns listener socket, which waits for incoming connections
 *   - accept and connect return data socket
 * listen: sets *pport to actual port if *pport == 0 on input
 *   - non-zero callback to listen in background (callback calls accept)
 * accept: wait for listening socket, return connection socket and addr
 * connect: connect to remote listener
 *   - non-zero callback to wait for data in background (callback calls recv)
 * recv, send: use the socket
 * close: free all resources for socket, close(0) closes all sockets (on_quit)
 */
PL_EXTERN pl_sckt_t *pl_sckt_listen(int *pport, void *ctx, pl_sckt_cb_t *callback);
PL_EXTERN pl_sckt_t *pl_sckt_accept(pl_sckt_t *listener, char **ppeer, void *ctx,
                               pl_sckt_cb_t *callback);
PL_EXTERN pl_sckt_t *pl_sckt_connect(const char *addr, int port, void *ctx,
                                pl_sckt_cb_t *callback);
PL_EXTERN long pl_sckt_recv(pl_sckt_t *sock, void *msg, long len);
PL_EXTERN int pl_sckt_send(pl_sckt_t *sock, const void *msg, long len);
PL_EXTERN void pl_sckt_close(pl_sckt_t *sock);

/* screen graphics connection */
PL_EXTERN pl_scr_t *pl_connect(char *server_name);  /* server_name 0 gets default */
PL_EXTERN void pl_disconnect(pl_scr_t *screen);
PL_EXTERN pl_scr_t *pl_multihead(pl_scr_t *other_screen, int number);

/* screen graphics queries (note when parameter is screen not window) */
PL_EXTERN int pl_txheight(pl_scr_t *s, int font, int pixsize, int *baseline);
PL_EXTERN int pl_txwidth(pl_scr_t *s, const char *text,int n, int font,int pixsize);
PL_EXTERN int pl_sshape(pl_scr_t *s, int *width, int *height);
PL_EXTERN int pl_wincount(pl_scr_t *s);
PL_EXTERN void pl_winloc(pl_win_t *w, int *x, int *y);

/* screen graphics window and pixmap management */
PL_EXTERN pl_win_t *pl_window(pl_scr_t *s, int width, int height, char *title,
                         pl_col_t bg, int hints, void *ctx);
PL_EXTERN pl_win_t *pl_subwindow(pl_scr_t *s, int width, int height,
                            unsigned long parent_id, int x, int y,
                            pl_col_t bg, int hints, void *ctx);
PL_EXTERN pl_win_t *pl_menu(pl_scr_t *s, int width, int height, int x, int y,
                       pl_col_t bg, void *ctx);
PL_EXTERN pl_win_t *pl_offscreen(pl_win_t *parent, int width, int height);
PL_EXTERN pl_win_t *pl_metafile(pl_win_t *parent, char *filename,
                           int x0, int y0, int width, int height, int hints);
PL_EXTERN void pl_destroy(pl_win_t *w);

/* screen graphics interactions with selection or clipboard */
PL_EXTERN int pl_scopy(pl_win_t *w, char *string, int n);
PL_EXTERN char *pl_spaste(pl_win_t *w);

/* screen graphics control functions */
PL_EXTERN void pl_feep(pl_win_t *w);
PL_EXTERN void pl_flush(pl_win_t *w);
PL_EXTERN void pl_clear(pl_win_t *w);
PL_EXTERN void pl_resize(pl_win_t *w, int width, int height);
PL_EXTERN void pl_raise(pl_win_t *w);
PL_EXTERN void pl_cursor(pl_win_t *w, int cursor);
PL_EXTERN void pl_palette(pl_win_t *w, pl_col_t *colors, int n);
PL_EXTERN void pl_clip(pl_win_t *w, int x0, int y0, int x1, int y1);

/* screen graphics property setting functions */
PL_EXTERN void pl_color(pl_win_t *w, pl_col_t color);
PL_EXTERN void pl_font(pl_win_t *w, int font, int pixsize, int orient);
PL_EXTERN void pl_pen(pl_win_t *w, int width, int type);

/* set point list for pl_dots, pl_lines, pl_fill, pl_segments (pairs in list)
 * if n>=0, creates a new list of points
 * if n<0, appends to existing list of points
 * total number of points (after all appends) will be <=2048
 * any drawing call resets the point list */
PL_EXTERN void pl_i_pnts(pl_win_t *w, const int *x, const int *y, int n);
PL_EXTERN void pl_d_pnts(pl_win_t *w, const double *x, const double *y, int n);
/* query or set coordinate mapping for pl_d_pnts */
PL_EXTERN void pl_d_map(pl_win_t *w, double xt[], double yt[], int set);

/* screen graphics drawing functions */
PL_EXTERN void pl_text(pl_win_t *w, int x0, int y0, const char *text, int n);
PL_EXTERN void pl_rect(pl_win_t *w, int x0, int y0, int x1, int y1, int border);
PL_EXTERN void pl_ellipse(pl_win_t *w, int x0, int y0, int x1, int y1, int border);
PL_EXTERN void pl_dots(pl_win_t *w);
PL_EXTERN void pl_segments(pl_win_t *w);
PL_EXTERN void pl_lines(pl_win_t *w);
PL_EXTERN void pl_fill(pl_win_t *w, int convexity);
PL_EXTERN void pl_ndx_cell(pl_win_t *w, unsigned char *ndxs, int ncols, int nrows,
                         int x0, int y0, int x1, int y1);
PL_EXTERN void pl_rgb_cell(pl_win_t *w, unsigned char *rgbs, int ncols, int nrows,
                         int x0, int y0, int x1, int y1);
PL_EXTERN void pl_bitblt(pl_win_t *w, int x, int y, pl_win_t *offscreen,
                       int x0, int y0, int x1, int y1);

PL_EXTERN void pl_rgb_read(pl_win_t *w, unsigned char *rgbs,
                         int x0, int y0, int x1, int y1);

/*------------------------------------------------------------------------*/
/* following have generic implementations */

/* idle and alarm "events" */
PL_EXTERN void pl_idler(int (*on_idle)(void));
PL_EXTERN void pl_on_idle(int reset);
PL_EXTERN double pl_timeout(void);
PL_EXTERN void pl_set_alarm(double secs, void (*on_alarm)(void *context),
                          void *context);
PL_EXTERN void pl_clr_alarm(void (*on_alarm)(void *c), void *context);

/* bitmap rotation, lsbit first and msbit first versions */
PL_EXTERN unsigned char pl_bit_rev[256];
PL_EXTERN void pl_lrot180(unsigned char *from, unsigned char *to,
                        int fcols, int frows);
PL_EXTERN void pl_lrot090(unsigned char *from, unsigned char *to,
                        int fcols, int frows);
PL_EXTERN void pl_lrot270(unsigned char *from, unsigned char *to,
                        int fcols, int frows);
PL_EXTERN void pl_mrot180(unsigned char *from, unsigned char *to,
                        int fcols, int frows);
PL_EXTERN void pl_mrot090(unsigned char *from, unsigned char *to,
                        int fcols, int frows);
PL_EXTERN void pl_mrot270(unsigned char *from, unsigned char *to,
                        int fcols, int frows);

/* 5x9x5 rgb colormap for pl_palette(w,pl_595,225) */
PL_EXTERN pl_col_t pl_595[225];

PL_END_EXTERN_C

/*------------------------------------------------------------------------*/

/* on_exception arguments and pl_signalling values */
#define PL_SIG_NONE  0
#define PL_SIG_SOFT  1
#define PL_SIG_INT   2
#define PL_SIG_FPE   3
#define PL_SIG_SEGV  4
#define PL_SIG_ILL   5
#define PL_SIG_BUS   6
#define PL_SIG_IO    7
#define PL_SIG_OTHER 8

/* window hints */
#define PL_PRIVMAP  0x01
#define PL_NOKEY    0x02
#define PL_NOMOTION 0x04
#define PL_NORESIZE 0x08
#define PL_DIALOG   0x10
#define PL_MODAL    0x20
#define PL_RGBMODEL 0x40

/* cursors */
#define PL_SELECT    0
#define PL_CROSSHAIR 1
#define PL_TEXT      2
#define PL_N         3
#define PL_S         4
#define PL_E         5
#define PL_W         6
#define PL_NS        7
#define PL_EW        8
#define PL_NSEW      9
#define PL_ROTATE   10
#define PL_DEATH    11
#define PL_HAND     12
#define PL_NONE     13

/* colors */
#define PL_IS_NDX(color) ((pl_col_t)(color)<256UL)
#define PL_IS_RGB(color) ((pl_col_t)(color)>=256UL)
#define PL_R(color) ((pl_col_t)(color)&0xffUL)
#define PL_G(color) (((pl_col_t)(color)>>8)&0xffUL)
#define PL_B(color) (((pl_col_t)(color)>>16)&0xffUL)
#define PL_RGB(r,g,b) ((pl_col_t)(r) | ((pl_col_t)(g)<<8) | ((pl_col_t)(b)<<16) | 0x01000000)
#define PL_BG      255UL
#define PL_FG      254UL
#define PL_BLACK   253UL
#define PL_WHITE   252UL
#define PL_RED     251UL
#define PL_GREEN   250UL
#define PL_BLUE    249UL
#define PL_CYAN    248UL
#define PL_MAGENTA 247UL
#define PL_YELLOW  246UL
#define PL_GRAYD   245UL
#define PL_GRAYC   244UL
#define PL_GRAYB   243UL
#define PL_GRAYA   242UL
#define PL_XOR     241UL
#define PL_EXTRA   240UL

/* fonts */
#define PL_COURIER     0
#define PL_TIMES       4
#define PL_HELVETICA   8
#define PL_SYMBOL     12
#define PL_NEWCENTURY 16
#define PL_GUI_FONT   20
#define PL_BOLD        1
#define PL_ITALIC      2
#define PL_OPAQUE     32

/* line types */
#define PL_SOLID      0
#define PL_DASH       1
#define PL_DOT        2
#define PL_DASHDOT    3
#define PL_DASHDOTDOT 4
#define PL_SQUARE     8

/* mouse buttons and shift keys */
#define PL_BTN1      000010
#define PL_BTN2      000020
#define PL_BTN3      000040
#define PL_BTN4      000100
#define PL_BTN5      000200
#define PL_SHIFT     000400
#define PL_CONTROL   001000
#define PL_META      002000
#define PL_ALT       004000
#define PL_COMPOSE   010000
#define PL_KEYPAD    020000

/* keys beyond ASCII */
#define PL_LEFT    0x0100
#define PL_RIGHT   0x0101
#define PL_UP      0x0102
#define PL_DOWN    0x0103
#define PL_PGUP    0x0104
#define PL_PGDN    0x0105
#define PL_HOME    0x0106
#define PL_END     0x0107
#define PL_INSERT  0x0108
#define PL_F0      0x0200
#define PL_F1      0x0201
#define PL_F2      0x0202
#define PL_F3      0x0203
#define PL_F4      0x0204
#define PL_F5      0x0205
#define PL_F6      0x0206
#define PL_F7      0x0207
#define PL_F8      0x0208
#define PL_F9      0x0209
#define PL_F10     0x020a
#define PL_F11     0x020b
#define PL_F12     0x020c

#endif /* _PLAY2_H */
