/*
 * $Id: playw.h,v 1.2 2010-07-19 07:39:13 thiebaut Exp $
 * MS Windows-private portability layer declarations
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include <play/win.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern void _pl_w_initialize(HINSTANCE i, HWND w,  void (*wquit)(void),
                         int (*wstdinit)(void(**)(char*,long),
                                         void(**)(char*,long)),
                         HWND (*wparent)(int, int, char *, int));
extern int _pl_w_on_quit(void);

extern void _pl_w_caught(void);
extern int _pl_w_sigint(int delay);
extern void _pl_w_siginit(void);
extern void _pl_w_fpu_setup(void);
extern int _pl_w_protect(int (*run)(void));

extern DWORD _pl_w_id_worker;  /* required for PostThreadMessage */
extern HANDLE _pl_w_worker;
extern int _pl_w_work_idle(void);
extern void _pl_w_pollinit(void);
extern UINT _pl_w_add_msg(void (*on_msg)(MSG *));
extern int _pl_w_app_msg(MSG *msg);
extern int (*_pl_w_msg_hook)(MSG *msg);
extern int _pl_w_add_input(HANDLE wait_obj, void (*on_input)(void *),
                       void *context);
extern void _pl_w_prepoll(void (*on_prepoll)(void *), void *context, int remove);

extern int _pl_w_no_mdi;
extern int con_stdinit(void(**)(char*,long), void(**)(char*,long));
extern int (*_pl_w_stdinit)(void (**wout)(char*,long),
                        void (**werr)(char*,long));
extern void con_stdout(char *output_line, long len);
extern void con_stderr(char *output_line, long len);
extern void _pl_w_deliver(char *buf);   /* calls on_stdin */
extern char *_pl_w_sendbuf(long len);

extern int _pl_w_nwins;  /* count of graphics windows */

extern HINSTANCE _pl_w_app_instance;
extern HWND _pl_w_main_window;
extern HWND (*_pl_w_parent)(int width, int height, char *title, int hints);
extern LRESULT CALLBACK _pl_w_winproc(HWND hwnd, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam);

extern char *_pl_w_pathname(const char *name);
extern char *_pl_w_unixpath(const char *wname);

/* if set non-zero, pl_abort will call this */
extern void (*_pl_w_abort_hook)(void);

/* ------------------------------------------------------------------------ */

extern pl_scr_t _pl_w_screen;
extern HCURSOR _pl_w_cursor(int cursor);
extern LPCTSTR _pl_w_win_class;
extern LPCTSTR _pl_w_menu_class;

extern COLORREF _pl_w_color(pl_win_t *w, unsigned long color);
extern HDC _pl_w_getdc(pl_win_t *w, int flags);

/* all bitmap functions seem to get colors backwards?? */
#define W_SWAPRGB(c) (((c)&0xff00)|(((c)>>16)&0xff)|(((c)&0xff)<<16))

extern POINT _pl_w_pt_list[2050];
extern int _pl_w_pt_count;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
