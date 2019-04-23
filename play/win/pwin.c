/*
 * $Id: pwin.c,v 1.3 2007-06-24 20:32:49 dhmunro Exp $
 * routines to create graphics devices for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"
#include "play/std.h"

HWND (*_pl_w_parent)(int width, int height, char *title, int hints)= 0;

static pl_win_t *w_pwin(void *ctx, pl_scr_t *s, HDC dc, HBITMAP bm,
                     unsigned long bg);

void
pl_cursor(pl_win_t *w, int cursor)
{
  if (cursor==PL_NONE) {
    w->cursor = 0;
    SetCursor(0);
    return;
  }
  if (cursor<0 || cursor>PL_NONE) cursor = PL_SELECT;
  w->cursor = w->s->cursors[cursor];
  if (!w->cursor)
    w->cursor = w->s->cursors[cursor] = _pl_w_cursor(cursor);
  SetCursor(w->cursor);
}

pl_win_t *pl_subwindow(pl_scr_t *s, int width, int height,
                   unsigned long parent_id, int x, int y,
                   pl_col_t bg, int hints, void *ctx)
{
  pl_win_t *pw = w_pwin(ctx, s, 0, 0, bg);
  HWND hw;
  HWND parent = (HWND)parent_id;
  DWORD xstyle = 0;
  DWORD style = WS_CHILD;

  if (!pw) return 0;

  pw->ancestor = 0;
  style |= WS_CHILD;

  hw = CreateWindowEx(xstyle, _pl_w_win_class, 0, style,
                      x, y, width, height, parent, 0,
                      _pl_w_app_instance, pw);
  /* CreateWindow already causes several calls to _pl_w_winproc
   * (including WM_CREATE) which will not have GWL_USERDATA set
   * -- WM_CREATE handler fills in and initializes pw */
  if (hw) {
    if (hints & PL_RGBMODEL) {
      pl_palette(pw, pl_595, 225);
      pw->rgb_mode = 1;
    }
    if (!(hints&PL_NOKEY)) SetActiveWindow(hw);
    ShowWindow(hw, (hints&PL_NOKEY)? SW_SHOWNA : SW_SHOW);
  } else {
    pl_destroy(pw);
    pw = 0;
  }

  return pw;
}

pl_win_t *
pl_window(pl_scr_t *s, int width, int height, char *title,
         unsigned long bg, int hints, void *ctx)
{
  pl_win_t *pw = w_pwin(ctx, s, 0, 0, bg);
  HWND hw;
  HWND parent = 0;
  DWORD xstyle = 0;
  DWORD style = 0;

  if (!pw) return 0;

  if (_pl_w_parent && !(hints & PL_DIALOG))
    parent = _pl_w_parent(width, height, title, hints);
  pw->ancestor = parent;
  if (parent) {
    style |= WS_CHILD;
  } else {
    RECT rect;
    style |= WS_OVERLAPPEDWINDOW;
    if (hints & PL_NORESIZE)
      style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    if (AdjustWindowRectEx(&rect, style, 0, xstyle)) {
      width = rect.right - rect.left;
      height = rect.bottom - rect.top;
    }
  }

  hw = CreateWindowEx(xstyle, _pl_w_win_class, parent? 0 : title, style,
                      CW_USEDEFAULT, 0, width, height, parent, 0,
                      _pl_w_app_instance, pw);
  /* CreateWindow already causes several calls to _pl_w_winproc
   * (including WM_CREATE) which will not have GWL_USERDATA set
   * -- WM_CREATE handler fills in and initializes pw */
  if (hw) {
    if (hints & PL_RGBMODEL) {
      pl_palette(pw, pl_595, 225);
      pw->rgb_mode = 1;
    }
    if (!(hints&PL_NOKEY)) SetActiveWindow(hw);
    ShowWindow(hw, (hints&PL_NOKEY)? SW_SHOWNA : SW_SHOW);
  } else {
    pl_destroy(pw);
    pw = 0;
  }

  return pw;
}

static pl_win_t *
w_pwin(void *ctx, pl_scr_t *s, HDC dc, HBITMAP bm, unsigned long bg)
{
  pl_win_t *pw = pl_malloc(sizeof(pl_win_t));
  if (pw) {
    pw->ctx = ctx;
    pw->s = s;
    pw->dc = dc;
    pw->w = 0;
    pw->bm = bm;
    pw->menu = 0;

    pw->parent = 0;
    pw->cursor = 0;
    pw->palette = 0;

    if (dc || bm) {
      pw->pixels = 0;
    } else {
      COLORREF *pixels = pl_malloc(sizeof(COLORREF)*256);
      if (pixels) {
        int i;
        for (i=0 ; i<=PL_EXTRA ; i++) pixels[i] = _pl_w_screen.sys_colors[255-PL_FG];
        for (; i<256 ; i++) pixels[i] = _pl_w_screen.sys_colors[255-i];
        pw->pixels = pixels;
      } else {
        pl_free(pw);
        return 0;
      }
      pw->cursor = s->cursors[PL_SELECT];
    }
    pw->n_pixels = 0;
    pw->rgb_mode = 0;

    pw->color = PL_FG;

    pw->pbflag = 0;
    pw->brush = 0;
    pw->brush_color = PL_BG;
    pw->pen = 0;
    pw->pen_color = PL_BG;
    pw->pen_width = 1;
    pw->pen_type = PL_SOLID;

    pw->font_color = PL_BG;
    pw->font = pw->pixsize = pw->orient = 0;

    pw->bg = bg;

    pw->keydown = 0;
    pw->ancestor = 0;

    _pl_w_getdc(pw, 24);
  }
  return pw;
}

void
pl_destroy(pl_win_t *pw)
{
  if (pw->s && pw->s->font_win==pw) pw->s->font_win = 0;
  if ((pw->pbflag&1) && pw->pen)
    DeleteObject(pw->pen), pw->pen = 0;
  if ((pw->pbflag&2) && pw->brush)
    DeleteObject(pw->brush), pw->brush = 0;
  if (pw->w) {
    pw->ctx = 0;           /* do not call on_destroy handler */
    DestroyWindow(pw->w);  /* WM_DESTROY handler does most of work */
  } else if (pw->bm) {
    /* which order is correct here? */
    DeleteDC(pw->dc), pw->dc = 0;
    DeleteObject(pw->bm), pw->bm = 0;
    pw->pixels = 0;  /* really belongs to parent window */
  } else {
    HENHMETAFILE meta = CloseEnhMetaFile(pw->dc);
    if (meta) DeleteEnhMetaFile(meta);
    pw->dc = 0;
    pw->pixels = 0;  /* really belongs to parent window */
  }
}

pl_win_t *
pl_offscreen(pl_win_t *parent, int width, int height)
{
  pl_win_t *pw = 0;
  HDC dc = _pl_w_getdc(parent, 0);
  dc = (parent->parent || !dc)? 0 : CreateCompatibleDC(dc);
  if (dc) {
    HBITMAP bm = CreateCompatibleBitmap(parent->dc, width, height);
    if (bm) {
      pw = w_pwin(0, parent->s, dc, bm, parent->bg);
      if (pw) {
        pw->parent = parent;
        SetBkColor(dc, _pl_w_color(pw, pw->bg));
        SelectObject(dc, bm);
      } else {
        DeleteObject(bm);
        bm = 0;
      }
    }
    if (!bm) DeleteDC(dc);
  }
  return pw;
}

pl_win_t *
pl_menu(pl_scr_t *s, int width, int height, int x, int y,
       unsigned long bg, void *ctx)
{
  pl_win_t *pw = w_pwin(ctx, s, 0, 0, bg);
  HWND hw;
  DWORD xstyle = WS_EX_TOPMOST;
  DWORD style = WS_POPUP | WS_VISIBLE;

  if (!pw) return 0;
  pw->menu = 1;

  hw = CreateWindowEx(xstyle, _pl_w_menu_class, 0, style,
                      x+s->x0, y+s->y0, width, height, _pl_w_main_window, 0,
                      _pl_w_app_instance, pw);
  /* see pl_window comment */
  if (hw) {
    SetActiveWindow(hw);
    ShowWindow(hw, SW_SHOW);
  } else {
    pl_destroy(pw);
    pw = 0;
  }

  return pw;
}

pl_win_t *
pl_metafile(pl_win_t *parent, char *filename,
           int x0, int y0, int width, int height, int hints)
{
  /* filename should end in ".emf" or ".EMF"
   * add missing extension here if necessary
   * -- note that this feature can be overridden by terminating
   *    filename with "." (Windows ignores trailing . in filenames) */
  pl_win_t *pw = 0;
  HDC mf = 0;
  HDC dc = parent->parent? 0 : _pl_w_getdc(parent, 0);

  if (dc) {
    long dxmm = GetDeviceCaps(dc, HORZSIZE);
    long dymm = GetDeviceCaps(dc, VERTSIZE);
    long dx = GetDeviceCaps(dc, HORZRES);
    long dy = GetDeviceCaps(dc, VERTRES);
    RECT r;  /* in 0.01 mm units */
    int i, n;
    r.left = r.top = 0;
    r.right = (width * dxmm * 100) / dx;
    r.bottom = (height * dymm * 100) / dy;

    filename = _pl_w_pathname(filename);  /* result always in pl_wkspc.c */
    for (n=0 ; filename[n] ; n++);
    for (i=1 ; i<=4 && i<n ; i++) if (filename[n-i]=='.') break;
    if (i>4) filename[n] = '.', filename[n+1] = 'e', filename[n+2] = 'm',
               filename[n+3] = 'f', filename[n+4] = '\0';
    mf = CreateEnhMetaFile(dc, filename, &r, 0);
    if (mf) {
      pw = w_pwin(0, parent->s, mf, 0, parent->bg);
      if (pw) {
        pw->parent = parent;
        pw->rgb_mode = 1;  /* inhibit pl_palette */
        pw->pixels = parent->pixels;
        pw->n_pixels = parent->n_pixels;
      } else {
        DeleteEnhMetaFile((HENHMETAFILE)mf);
      }
    }
  }

  return pw;
}
