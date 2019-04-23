/*
 * $Id: getdc.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * get a DC to draw into a window
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"
#include "play/std.h"

/* WARNING
 * implementations of pl_txwidth and pl_txheight assume
 * that pl_font will be called afterwards
 */
static HDC wp_font(pl_win_t *w, int font, int pixsize, int orient);
static void w_font(pl_scr_t *s, HDC dc, int font, int pixsize, int orient);

void
pl_font(pl_win_t *w, int font, int pixsize, int orient)
{
  wp_font(w, font, pixsize, orient);
}

static HDC
wp_font(pl_win_t *w, int font, int pixsize, int orient)
{
  HDC dc = _pl_w_getdc(w, 0);
  if (dc) {
    if (!w->s->does_rotext) {
      w->orient = orient;
      orient = 0;
    }
    if (w->font!=font || w->pixsize!=pixsize) {
      w_font(w->s, dc, font, pixsize, orient);
      w->font = font;
      w->pixsize = pixsize;
      w->s->font_win = w;
    }
  }
  return dc;
}

static void
w_font(pl_scr_t *s, HDC dc, int font, int pixsize, int orient)
{
  int face = ((unsigned int)(font&0x1c))>>2;

  if (face >= 5) {
    SelectObject(dc, s->gui_font);

  } else {
    int i, j;

    for (i=j=0 ; i<PL_FONTS_CACHED && s->font_cache[i].hfont ; i++) {
      j = s->font_order[i];
      if (s->font_cache[j].font==font &&
          s->font_cache[j].pixsize==pixsize &&
          s->font_cache[j].orient==orient) {
        /* install cached font and make it most recently used */
        for (; i>0 ; i--) s->font_order[i] = s->font_order[i-1];
        s->font_order[0] = j;
        SelectObject(dc, s->font_cache[j].hfont);
        return;
      }
    }
    if (i<PL_FONTS_CACHED) j = i++;

    {
      static DWORD families[5] = {
        FF_MODERN|FIXED_PITCH, FF_ROMAN|VARIABLE_PITCH,
        FF_SWISS|VARIABLE_PITCH, VARIABLE_PITCH, FF_ROMAN|VARIABLE_PITCH };
      /* note: my MS Windows box does not have Helvetica -- is Arial
       *       equivalent? -- hopefully substitution will be reasonable */
      static char *names[5] = { "Courier", "Times New Roman", "Helvetica",
                               "Symbol", "Century Schoolbook" };
      static int angles[4] = { 0, 900, 1800, 2700 };
      int ang = angles[orient];
      int weight = (font&PL_BOLD)? FW_BOLD : FW_NORMAL;
      DWORD charset = ((font&PL_SYMBOL)!=PL_SYMBOL)? ANSI_CHARSET:SYMBOL_CHARSET;
      HFONT hfont = CreateFont(pixsize, 0, ang, ang, weight,
                              (font&PL_ITALIC)!=0, 0, 0, charset,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              PROOF_QUALITY, families[face], names[face]);
      HFONT fold = s->font_cache[j].hfont;
      for (i-- ; i>0 ; i--)
        s->font_order[i] = s->font_order[i-1];
      s->font_order[0] = j;
      s->font_cache[j].hfont = hfont;
      s->font_cache[j].font = font;
      s->font_cache[j].pixsize = pixsize;
      s->font_cache[j].orient = orient;
      if (fold) DeleteObject(fold);
      SelectObject(dc, hfont);
    }
  }
}

int
pl_txheight(pl_scr_t *s, int font, int pixsize, int *baseline)
{
  int height = pixsize;
  int base = pixsize;
  if (!pl_signalling) {
    HFONT orig = 0;
    HDC dc;
    TEXTMETRIC tm;
    if (s->font_win) {
      dc = wp_font(s->font_win, font, pixsize, s->font_win->orient);
    } else {
      dc = GetDC(0);
      orig = dc? SelectObject(dc, GetStockObject(ANSI_FIXED_FONT)) : 0;
      w_font(s, dc, font, pixsize, 0);
    }
    if (dc && GetTextMetrics(dc, &tm)) {
      base = tm.tmAscent;
      height = tm.tmHeight + 1;
    }
    if (orig) SelectObject(dc, orig);
  } else {
    pl_abort();
  }
  if (baseline) *baseline = base;
  return height;
}

int
pl_txwidth(pl_scr_t *s, const char *text, int n, int font, int pixsize)
{
  int width = pixsize;
  if (!pl_signalling) {
    HFONT orig = 0;
    HDC dc;
    SIZE sz;
    if (s->font_win) {
      dc = wp_font(s->font_win, font, pixsize, s->font_win->orient);
    } else {
      dc = GetDC(0);
      orig = dc? SelectObject(dc, GetStockObject(ANSI_FIXED_FONT)) : 0;
      w_font(s, dc, font, pixsize, 0);
    }
    if (dc && GetTextExtentPoint32(dc, text, n, &sz))
      width = sz.cx;
    else
      width = n*(pixsize*3/5);
    if (orig) SelectObject(dc, orig);
  } else {
    pl_abort();
  }
  return width;
}

void
pl_pen(pl_win_t *w, int width, int type)
{
  if (pl_signalling) {
    pl_abort();
  } else {
    if (width<1) width = 1;
    else if (width>100) width = 100;
    if (w->pen_width!=width || w->pen_type!=type) {
      w->pen_width = width;
      w->pen_type = type;
      w->pen = 0;
    }
  }
}

void
pl_color(pl_win_t *w, unsigned long color)
{
  if (pl_signalling) pl_abort(), color = w->color;
  if (w->color != color) {
    int de_xor = (w->color == PL_XOR);
    w->color = color;
    if (de_xor || color==PL_XOR) {
      if (w->dc)
        SetROP2(w->dc, de_xor? R2_COPYPEN : R2_NOT);  /* or R2_XORPEN */
    }
  }
}

COLORREF
_pl_w_color(pl_win_t *w, unsigned long color)
{
  if (w->parent) w = w->parent;
  if (!PL_IS_RGB(color)) {
    return w->pixels[color];
  } else {
    unsigned int r = PL_R(color);
    unsigned int g = PL_G(color);
    unsigned int b = PL_B(color);
    if (w->w && !w->menu && w->s->sys_pal) {
      if (!w->rgb_mode) {
        pl_palette(w, pl_595, 225);
        w->rgb_mode = 1;
      }
      /* must be consistent with pl_palette in pals.c */
      r = (r+32)>>6;
      g = (g+16)>>5;
      b = (b+32)>>6;
      g += b+(b<<3);
      return PALETTEINDEX(w->s->sys_offset+r+g+(g<<2));  /* r+5*g+45*b */
    } else {
      return RGB(r, g, b);
    }
  }
}

HDC
_pl_w_getdc(pl_win_t *w, int flags)
{
  HDC dc = w->dc;
  if (pl_signalling) pl_abort(), dc = 0;
  if (dc) {
    if (flags) {
      COLORREF color = (flags&7)? _pl_w_color(w, w->color) : 0;

      if ((flags & 1) && (w->font_color!=w->color)) {
        /* text */
        w->font_color = w->color;
        SetTextColor(dc, color);
      }

      if (flags & 2) {
        /* pen */
        HPEN pen = 0;
        if (!w->pen || (w->pen_color != w->color)) {
          int width = w->pen_width;
#ifdef USE_GEOMETRIC_PEN
          /* NOTE: geometric pen is way to slow for practical use */
          int type = w->pen_type;
          static DWORD styles[8] = {
            PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT, PS_DASHDOTDOT, 0, 0, 0 };
          DWORD ltype = w->s->does_linetypes? styles[type&7] : PS_SOLID;
          DWORD cap = (type&PL_SQUARE)? PS_ENDCAP_SQUARE : PS_ENDCAP_ROUND;
          DWORD join = (type&PL_SQUARE)? PS_JOIN_MITER : PS_JOIN_ROUND;
          LOGBRUSH brush;
          brush.lbStyle = BS_SOLID;
          brush.lbColor = color;
          brush.lbHatch = 0;
          pen = ExtCreatePen(PS_GEOMETRIC | ltype | cap | join,
                             width, &brush, 0, 0);
#else
          /* downside of cosmetic pen is always round endcaps and joins,
           * so PL_SQUARE type modifier is ignored
           * -- go for high performance instead */
          pen = CreatePen(PS_SOLID, width, color);
#endif
          if (!pen) return 0;
          w->pen = pen;
          w->pen_color = w->color;
        } else if (w->pbflag&1) {
          /* null pen has been installed */
          pen = w->pen;
        }
        if (pen) {
          pen = SelectObject(dc, pen);
          if (pen) DeleteObject(pen);
          w->pbflag &= ~1;
        }
      }

      if (flags & 4) {
        /* brush */
        HBRUSH b = 0;
        if (!w->brush || (w->brush_color != w->color)) {
          b = CreateSolidBrush(color);
          if (!b) return 0;
          w->brush = b;
          w->brush_color = w->color;
        } else if (w->pbflag&2) {
          b = w->brush;
        }
        if (b) {
          b = SelectObject(dc, b);
          if (b) DeleteObject(b);
          w->pbflag &= ~2;
        }
      }

      if (flags & 8) {
        /* install null pen */
        HPEN pen = CreatePen(PS_NULL, 0, 0);
        if (pen) {
          pen = SelectObject(dc, pen);
          if (pen && pen==w->pen) w->pbflag |= 1;
        }
      }

      if (flags & 16) {
        /* install null brush */
        HBRUSH b = SelectObject(dc, GetStockObject(NULL_BRUSH));
        if (b && b==w->brush) w->pbflag |= 2;
      }
    }
  }
  return dc;
}
