/*
 * $Id: pscr.c,v 1.3 2007-03-19 07:31:30 thiebaut Exp $
 * routines to initialize graphics for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

/* is this really necessary to get OCR_*?? */
#define OEMRESOURCE

#include "playw.h"
#include "play/std.h"

HINSTANCE _pl_w_app_instance = 0;
HWND _pl_w_main_window = 0;

pl_scr_t _pl_w_screen;

static void (*won_expose)(void *c,int *xy)= 0;
static void (*won_destroy)(void *c)= 0;
static void (*won_resize)(void *c,int w,int h)= 0;
static void (*won_focus)(void *c,int in)= 0;
static void (*won_key)(void *c,int k,int md)= 0;
static void (*won_click)(void *c,int b,int md,int x,int y,unsigned long ms)=0;
static void (*won_motion)(void *c,int md,int x,int y)= 0;
static void (*won_deselect)(void *c)= 0;

LPCTSTR _pl_w_win_class = "_pl_w_win_class";
LPCTSTR _pl_w_menu_class = "_pl_w_menu_class";

static char *clip_text = 0;
static HWND clip_owner = 0;
static void clip_free(int force);

void (*pl_on_connect)(int dis, int fd) = 0;

pl_scr_t *
pl_connect(char *server_name)
{
  static int registered = 0;
  HDC dc0 = GetDC(0);
  RECT r;
  int i;

  if (server_name) return 0;

  _pl_w_screen.width = GetSystemMetrics(SM_CXSCREEN);
  _pl_w_screen.height = GetSystemMetrics(SM_CYSCREEN);
  _pl_w_screen.depth = GetDeviceCaps(dc0, BITSPIXEL);

  /* adjust width and height for taskbar, other toolbar garbage */
  if (SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&r, 0)) {
    _pl_w_screen.x0 = r.left;
    _pl_w_screen.y0 = r.top;
    _pl_w_screen.width = r.right - r.left;
    _pl_w_screen.height = r.bottom - r.top;
  }

  /* windows linetypes are unusably ugly for thin lines */
  /*_pl_w_screen.does_linetypes = GetDeviceCaps(dc0, LINECAPS) & LC_WIDESTYLED;*/
  _pl_w_screen.does_linetypes = 0;
  _pl_w_screen.does_rotext = GetDeviceCaps(dc0, TEXTCAPS) & TC_CR_90;

  _pl_w_screen.sys_colors[255-PL_BLACK] = RGB(0,0,0);
  _pl_w_screen.sys_colors[255-PL_WHITE] = RGB(255,255,255);
  _pl_w_screen.sys_colors[255-PL_RED] = RGB(255,0,0);
  _pl_w_screen.sys_colors[255-PL_GREEN] = RGB(0,255,0);
  _pl_w_screen.sys_colors[255-PL_BLUE] = RGB(0,0,255);
  _pl_w_screen.sys_colors[255-PL_CYAN] = RGB(0,255,255);
  _pl_w_screen.sys_colors[255-PL_MAGENTA] = RGB(255,0,255);
  _pl_w_screen.sys_colors[255-PL_YELLOW] = RGB(255,255,0);
  _pl_w_screen.sys_colors[255-PL_BG] = GetSysColor(COLOR_WINDOW);
  _pl_w_screen.sys_colors[255-PL_FG] = GetSysColor(COLOR_WINDOWTEXT);
  _pl_w_screen.sys_colors[255-PL_GRAYD] = RGB(100,100,100);
  _pl_w_screen.sys_colors[255-PL_GRAYC] = RGB(150,150,150);
  _pl_w_screen.sys_colors[255-PL_GRAYB] = RGB(190,190,190);
  _pl_w_screen.sys_colors[255-PL_GRAYA] = RGB(214,214,214);
  _pl_w_screen.sys_colors[255-PL_XOR] = 0xffffff;  /* really fg^bg, but unused */
  i = GetDeviceCaps(dc0, NUMRESERVED);
  if (_pl_w_screen.depth==8 && i>1 && i<=32 &&
      (GetDeviceCaps(dc0, RASTERCAPS)&RC_PALETTE) &&
      GetDeviceCaps(dc0, SIZEPALETTE)==256) {
    PALETTEENTRY *pal = pl_malloc(sizeof(PALETTEENTRY)*i);
    if (pal) {
      struct {
        WORD version;
        WORD nentries;
        PALETTEENTRY entry[32];
      } yuck;
      HPALETTE syspal;
      int sz = GetDeviceCaps(dc0, SIZEPALETTE);
      int n = i;
      _pl_w_screen.sys_offset = n/2;
      _pl_w_screen.sys_pal = pal;
      GetSystemPaletteEntries(dc0, 0, n/2, pal);
      GetSystemPaletteEntries(dc0, sz-n/2, n/2, pal+n/2);
      yuck.version = 0x300;
      yuck.nentries = n;
      for (i=0 ; i<n ; i++) yuck.entry[i] = pal[i];
      syspal = CreatePalette((LOGPALETTE *)&yuck);
      if (syspal) {
        int j;
        for (i=0 ; i<15 ; i++) {
          j = GetNearestPaletteIndex(syspal, _pl_w_screen.sys_colors[i]);
          _pl_w_screen.sys_index[i] = PALETTEINDEX((j==CLR_INVALID)? 0 : j);
        }
        DeleteObject(syspal);
      }
    }
  } else {
    for (i=0 ; i<15 ; i++) _pl_w_screen.sys_index[i] = _pl_w_screen.sys_colors[i];
    _pl_w_screen.sys_offset = 0;
    _pl_w_screen.sys_pal = 0;
  }

  _pl_w_screen.gui_font = GetStockObject(ANSI_FIXED_FONT);
  /* _pl_w_screen.def_palette = GetStockObject(DEFAULT_PALETTE); */
  /* _pl_w_screen.null_brush = GetStockObject(NULL_BRUSH); */
  /* _pl_w_screen.null_pen = CreatePen(PS_NULL, 0, 0); */

  for (i=0 ; i<PL_FONTS_CACHED ; i++) {
    _pl_w_screen.font_order[i] = i;
    _pl_w_screen.font_cache[i].hfont = 0;
  }
  _pl_w_screen.font_win = 0;

  {
    char *sys_cursor[PL_NONE] = {
      IDC_ARROW, IDC_CROSS, IDC_IBEAM, 0, 0, 0, 0,
      IDC_SIZENS, IDC_SIZEWE, IDC_SIZEALL, 0, 0, 0 };
    int i;
    for (i=0 ; i<PL_NONE ; i++) {
      if (sys_cursor[i])
        _pl_w_screen.cursors[i] = LoadCursor(0, sys_cursor[i]);
      else
        _pl_w_screen.cursors[i] = 0;
    }
  }

  _pl_w_screen.first_menu = _pl_w_screen.active = 0;

  if (!registered) {
    WNDCLASSEX class_data;
    class_data.cbSize = sizeof(WNDCLASSEX);
    class_data.style = CS_OWNDC;
    class_data.lpfnWndProc = (WNDPROC)_pl_w_winproc;
    class_data.cbClsExtra = 0;
    class_data.cbWndExtra = 0;
    class_data.hInstance = _pl_w_app_instance;
    class_data.hIcon = 0;
    class_data.hCursor = 0;
    class_data.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    class_data.lpszMenuName = 0;
    class_data.lpszClassName = _pl_w_win_class;
    class_data.hIconSm = 0;
    if (!RegisterClassEx(&class_data)) return 0;

    class_data.cbSize = sizeof(WNDCLASSEX);
    class_data.style = CS_OWNDC | CS_SAVEBITS;
    class_data.lpfnWndProc = (WNDPROC)_pl_w_winproc;
    class_data.cbClsExtra = 0;
    class_data.cbWndExtra = 0;
    class_data.hInstance = _pl_w_app_instance;
    class_data.hIcon = 0;
    class_data.hCursor = 0;
    class_data.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    class_data.lpszMenuName = 0;
    class_data.lpszClassName = _pl_w_menu_class;
    class_data.hIconSm = 0;
    if (!RegisterClassEx(&class_data)) return 0;
    registered = 1;
  }

  if (pl_on_connect) pl_on_connect(0, -1);

  return &_pl_w_screen;
}

/* expose MSWindows specific data required to create subwindow */
HINSTANCE
_pl_w_linker(LPCTSTR *pw_class, WNDPROC *pw_proc)
{
  *pw_proc = (WNDPROC)_pl_w_winproc;
  *pw_class = _pl_w_win_class;
  return _pl_w_app_instance;
}

int
pl_sshape(pl_scr_t *s, int *width, int *height)
{
  *width = s->width;
  *height = s->height;
  return s->depth;
}

/* ARGSUSED */
pl_scr_t *
pl_multihead(pl_scr_t *other, int number)
{
  return 0;
}

/* ARGSUSED */
void
pl_disconnect(pl_scr_t *s)
{
  int i;
  if (pl_on_connect) pl_on_connect(1, -1);
  for (i=0 ; i<PL_FONTS_CACHED && s->font_cache[i].hfont ; i++) {
    DeleteObject(s->font_cache[i].hfont);
    s->font_cache[i].hfont = 0;
  }
  for (i=3 ; i<=12 ; i++) {
    if ((i>6 && i<10) || !s->cursors[i] || s->cursors[i]==s->cursors[0])
      continue;
    DestroyCursor(s->cursors[i]);
    s->cursors[i] = s->cursors[0];
  }
  if (s->sys_pal) pl_free(s->sys_pal), s->sys_pal = 0;
}

/* ARGSUSED */
void
pl_gui(void (*on_expose)(void *c, int *xy),
      void (*on_destroy)(void *c),
      void (*on_resize)(void *c,int w,int h),
      void (*on_focus)(void *c,int in),
      void (*on_key)(void *c,int k,int md),
      void (*on_click)(void *c,int b,int md,int x,int y,
                       unsigned long ms),
      void (*on_motion)(void *c,int md,int x,int y),
      void (*on_deselect)(void *c),
      void (*on_panic)(pl_scr_t *s))
{
  won_expose = on_expose;
  won_destroy = on_destroy;
  won_resize = on_resize;
  won_focus = on_focus;
  won_key = on_key;
  won_click = on_click;
  won_motion = on_motion;
  won_deselect = on_deselect;
}

/* ARGSUSED */
void
pl_gui_query(void (**on_expose)(void *c, int *xy),
	    void (**on_destroy)(void *c),
	    void (**on_resize)(void *c,int w,int h),
	    void (**on_focus)(void *c,int in),
	    void (**on_key)(void *c,int k,int md),
	    void (**on_click)(void *c,int b,int md,int x,int y,
			     unsigned long ms),
	    void (**on_motion)(void *c,int md,int x,int y),
	    void (**on_deselect)(void *c),
	    void (**on_panic)(pl_scr_t *s))
{
  *on_expose = won_expose;
  *on_destroy = won_destroy;
  *on_resize = won_resize;
  *on_focus = won_focus;
  *on_key = won_key;
  *on_click = won_click;
  *on_motion = won_motion;
  *on_deselect = won_deselect;
  *on_panic = 0; /* on_panic not used by Windows interface */
}

/* ARGSUSED */
void
pl_flush(pl_win_t *w)
{
  GdiFlush();
}

void
pl_clear(pl_win_t *w)
{
  RECT r;
  if (!pl_signalling && GetClientRect(w->w? w->w : w->parent->w, &r)) {
    if (w->dc) {
      HBRUSH b = CreateSolidBrush(_pl_w_color(w, w->bg));
      if (b) {
        FillRect(w->dc, &r, b);
        DeleteObject(b);
      }
    }
  }
}

void
pl_winloc(pl_win_t *w, int *x, int *y)
{
  POINT p;
  p.x = p.y = 0;
  if (w->w && ClientToScreen(w->w, &p)) {
    *x = p.x - w->s->x0;
    *y = p.y - w->s->y0;
  } else {
    *x = *y = 0;
  }
}

void
pl_resize(pl_win_t *w, int width, int height)
{
  HWND hw = w->w;
  if (hw) {
    RECT rin, rout;
    if (GetClientRect(hw, &rin)) {
      if (w->ancestor) hw = w->ancestor;
      if (GetWindowRect(hw, &rout)) {
        width += (rout.right-rout.left) - rin.right;
        height += (rout.bottom-rout.top) - rin.bottom;
      }
    }
    SetWindowPos(hw, 0, 0,0, width,height,
                 SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
  }
}

void
pl_raise(pl_win_t *w)
{
  ShowWindow(w->w, SW_SHOW);
  /* also SetActiveWindow, SetForegroundWindow */
  BringWindowToTop(w->w);
}

int _pl_w_nwins = 0;

int
pl_wincount(pl_scr_t *s)
{
  return (s==&_pl_w_screen)? _pl_w_nwins : 0;
}

LRESULT CALLBACK
_pl_w_winproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  pl_win_t *pw = (pl_win_t *)GetWindowLong(hwnd, GWL_USERDATA);
  if (pw && pw->w==hwnd) {
    int button = 0;

    if (msg == WM_CHAR) {  /* horrible design */
      if (won_key && pw->ctx) {
        int key = (int)(pw->keydown&&0xffff);
        if (!key) {
          int state = (int)(pw->keydown>>16);
          key = wp;
          if (GetKeyState(VK_SHIFT)<0) state |= PL_SHIFT;
          if (GetKeyState(VK_CONTROL)<0) state |= PL_CONTROL;
          if (HIWORD(lp)&KF_ALTDOWN) state |= PL_ALT;  /* doesnt work */
          won_key(pw->ctx, key, state);
        }
        pw->keydown = 0;
      }
      return 0;
    }
    pw->keydown &= 0; /* ((unsigned long)PL_META)<<16; */

    switch (msg) {
    case WM_PAINT:
      /* call BeginPaint or ValidateRect to avoid another WM_PAINT */
      if (won_expose && pw->ctx) {
        RECT r;
        int xy[4];
        if (GetUpdateRect(hwnd, &r, 0)) {
          xy[0] = r.left;
          xy[1] = r.top;
          xy[2] = r.right;
          xy[3] = r.bottom;
        } else {
          r.left = r.top = r.right = r.bottom = 0;
        }
        if (GetClientRect(hwnd, &r))
          won_expose(pw->ctx, (xy[0]<=0 && xy[1]<=0 && xy[2]>=r.right &&
                               xy[3]>=r.bottom)? 0 : xy);
      }
      ValidateRect(hwnd, 0);
      return 0;
    case WM_ERASEBKGND:
      pl_clear(pw);
      return 1;
    case WM_SIZE:
      if (won_resize && pw->ctx && wp!=SIZE_MINIMIZED &&
          wp!=SIZE_MAXHIDE && wp!=SIZE_MAXSHOW) {
        int width = LOWORD(lp);
        int height = HIWORD(lp);
        won_resize(pw->ctx, width, height);
      }
      return 0;
    case WM_DESTROY:
      _pl_w_nwins--;
      pw->w = 0;
      pw->dc = 0;
      if (pw == pw->s->active) pw->s->active = 0;
      if (pw == pw->s->first_menu) {
        pw->s->first_menu = 0;
        ReleaseCapture();
      }
      if (hwnd == clip_owner) clip_free(1);
      if (pw->pixels) {
        pl_free(pw->pixels);
        pw->pixels = 0;
      }
      if (pw->palette) {
        UnrealizeObject(pw->palette);
        DeleteObject(pw->palette);
        pw->palette = 0;
      }
      if (won_destroy && pw->ctx) won_destroy(pw->ctx);
      pw->ctx = 0;
      SetWindowLong(hwnd, GWL_USERDATA, (LONG)0);
      if (pw->ancestor) {
        hwnd = pw->ancestor;
        pw->ancestor = 0;
        SendMessage(hwnd, WM_CLOSE, 0, 0);
      }
      pl_free(pw);
      return 0;

    case WM_SETFOCUS:
      if (pw->palette) {
        pw->s->active = pw;
        SelectPalette(pw->dc, pw->palette, 0);
        if (RealizePalette(pw->dc))
          InvalidateRect(hwnd, 0, 0);
        SelectPalette(pw->dc, pw->palette, 1);
        RealizePalette(pw->dc);
      }
      button = 1;
    case WM_KILLFOCUS:
      if (won_focus && pw->ctx)
        won_focus(pw->ctx, button);
      return 0;

    case WM_QUERYNEWPALETTE:
      if (pw->palette || pw->s->active) {
        if (!pw->palette) pw = pw->s->active;
        else if (pw->s->active != pw) pw->s->active = pw;
        SelectPalette(pw->dc, pw->palette, 0);
        button = RealizePalette(pw->dc);
        if (button) InvalidateRect(hwnd, 0, 0);
        SelectPalette(pw->dc, pw->palette, 1);
        RealizePalette(pw->dc);
      }
      return button;

    case WM_KEYDOWN:
      if (won_key && pw->ctx) {  /* wp is virtual key code */
        int key = 0;
        int state = 0; /* (int)((pw->keydown>>16)&PL_META); */
        if (wp>=VK_NUMPAD0 && wp<=VK_F24) {
          if (wp<=VK_DIVIDE) {
            if (wp<=VK_NUMPAD9) key = (wp-VK_NUMPAD0) + '0';
            else if (wp==VK_MULTIPLY) key = '*';
            else if (wp==VK_ADD) key = '+';
            else if (wp==VK_SEPARATOR) key = '=';
            else if (wp==VK_SUBTRACT) key = '-';
            else if (wp==VK_DECIMAL) key = '.';
            else if (wp==VK_DIVIDE) key = '/';
            state |= PL_KEYPAD;
          } else {
            key = (wp-VK_F1) + PL_F1;
          }
        } else if (wp==VK_PRIOR) key = PL_PGUP;
        else if (wp==VK_NEXT) key = PL_PGDN;
        else if (wp==VK_END) key = PL_END;
        else if (wp==VK_HOME) key = PL_HOME;
        else if (wp==VK_LEFT) key = PL_LEFT;
        else if (wp==VK_UP) key = PL_UP;
        else if (wp==VK_RIGHT) key = PL_RIGHT;
        else if (wp==VK_DOWN) key = PL_DOWN;
        else if (wp==VK_INSERT) key = PL_INSERT;
        else if (wp==VK_DELETE) key = 0x7f;
        if (key) {
          if (GetKeyState(VK_SHIFT)<0) state |= PL_SHIFT;
          if (GetKeyState(VK_CONTROL)<0) state |= PL_CONTROL;
          /* VK_MENU doesnt work */
          won_key(pw->ctx, key, state);
        } else if (wp==VK_SPACE && GetKeyState(VK_CONTROL)<0) {
          if (GetKeyState(VK_SHIFT)<0) state |= PL_SHIFT;
          won_key(pw->ctx, 0, state);
          key = ' ';
        } else if (wp==VK_SHIFT || wp==VK_CONTROL || wp==VK_MENU ||
                   wp==VK_CAPITAL || wp==VK_NUMLOCK) {
          key = 0x8000;
        } else if (wp==VK_LWIN || wp==VK_RWIN) {
          /* state |= PL_META;   this idea didnt work */
        }
        pw->keydown = (((unsigned long)state)<<16) | key;
      }
      return 0;
    case WM_KEYUP:
      /* if (wp==VK_LWIN || wp==VK_RWIN) pw->keydown = 0; */
      break;

    case WM_RBUTTONDOWN:
      button++;
    case WM_MBUTTONDOWN:
      button++;
    case WM_LBUTTONDOWN:
      button++;
    case WM_RBUTTONUP:
      button++;
    case WM_MBUTTONUP:
      button++;
    case WM_LBUTTONUP:
      button++;
      if (won_click && pw->ctx) {
        int x = LOWORD(lp);
        int y = HIWORD(lp);
        int state = 0;
        int down = (button>3);
        if (down) button -= 3;
        if (wp&MK_LBUTTON) state |= PL_BTN1;
        if (wp&MK_MBUTTON) state |= PL_BTN2;
        if (wp&MK_RBUTTON) state |= PL_BTN3;
        if (wp&MK_CONTROL) state |= PL_CONTROL;
        if (wp&MK_SHIFT) state |= PL_SHIFT;
        /* if (GetKeyState(VK_MENU)<0) state |= PL_ALT; doesnt work */
        /* PL_META? */
        state ^= (1<<(button+2));  /* make consistent with X11 */
        won_click(pw->ctx, button, state, x, y, GetMessageTime());
      }
      return 0;

    case WM_MOUSEMOVE:
      if (won_motion && pw->ctx) {
        int x = LOWORD(lp);
        int y = HIWORD(lp);
        int state = 0;
        if (wp&MK_LBUTTON) state |= PL_BTN1;
        if (wp&MK_MBUTTON) state |= PL_BTN2;
        if (wp&MK_RBUTTON) state |= PL_BTN3;
        if (wp&MK_CONTROL) state |= PL_CONTROL;
        if (wp&MK_SHIFT) state |= PL_SHIFT;
        /* PL_META, PL_ALT? */
        won_motion(pw->ctx, state, x, y);
        /* counting messages is problematic because pl_qclear might remove
         * them, and servicing motion messages out of order seems a poor
         * idea -- so just deliver them all for now
         * -- perhaps play API should have a function to check if there are
         *    pending motion events? */
      }
      return 0;

    case WM_SETCURSOR:
      if (LOWORD(lp) == HTCLIENT) {
        SetCursor(pw->cursor);
        return 1;
      }
      break;

    case WM_DESTROYCLIPBOARD:
      clip_free(0);
      if (won_deselect && pw->ctx) won_deselect(pw->ctx);
      return 0;
    }

  } else if (msg == WM_CREATE) {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lp;
    pw = cs? cs->lpCreateParams : 0;
    if (pw) {
      HDC dc = GetDC(hwnd);
      SetWindowLong(hwnd, GWL_USERDATA, (LONG)pw);
      pw->w = hwnd;
      pw->dc = dc;
      SetBkColor(dc, _pl_w_color(pw, pw->bg));
      SetBkMode(dc, TRANSPARENT);
      SetTextAlign(dc, TA_LEFT | TA_BASELINE | TA_NOUPDATECP);
      if (pw->menu && !pw->s->first_menu)  {
        pw->s->first_menu = pw;
        SetCapture(hwnd);
      }
      _pl_w_nwins++;
      return 0;
    }
  }

  /* consider also DefFrameProc, DefMDIChildProc */
  return DefWindowProc(hwnd, msg, wp, lp);
}

static void
clip_free(int force)
{
  char *t = clip_text;

  if (force && clip_owner && OpenClipboard(0)) {
    EmptyClipboard();
    CloseClipboard();
  }

  clip_owner = 0;
  clip_text = 0;
  if (t) pl_free(t);
}

int
pl_scopy(pl_win_t *w, char *string, int n)
{
  int result = 1;
  HWND owner = (string && (n>0))? w->w : 0;
  if (!pl_signalling && OpenClipboard(owner)) {
    if (EmptyClipboard()) {
      int i;
      char *tedious = clip_text;
      clip_free(0);  /* actually called during EmptyClipboard */
      clip_text = pl_strncat(0, string, n);
      tedious = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, n+1);
      string = tedious? GlobalLock(tedious) : 0;
      if (string) {
        for (i=0 ; clip_text[i] ; i++) string[i] = clip_text[i];
        string[i] = '\0';
        GlobalUnlock(tedious);
        SetClipboardData(CF_TEXT, tedious);
        clip_owner = owner;
        result = 0;
      }
    }
    CloseClipboard();
  }
  return result;
}

char *
pl_spaste(pl_win_t *w)
{
  if (pl_signalling) return 0;
  if (!clip_owner && OpenClipboard(w->w)) {
    char *tedious = GetClipboardData(CF_TEXT);
    char *string = tedious? GlobalLock(tedious) : 0;
    clip_free(0);
    if (string) {
      clip_text = pl_strcpy(string);
      GlobalUnlock(tedious);
    }
    CloseClipboard();
  }
  return clip_text;
}
