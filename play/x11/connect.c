/*
 * $Id: connect.c,v 1.1 2005-09-18 22:05:34 dhmunro Exp $
 * routines to connect to an X11 server
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "playx.h"

#include "play/std.h"

#include <string.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

static pl_scr_t *x_screen(pl_x_display_t *xdpy, int number);
static int pl_try_grays(Display *dpy, Colormap cmap, XColor *color,
                       int c, int dc);
static void x_disconnect(pl_x_display_t *xdpy);
static int x_err_installed = 0;

pl_x_display_t *_pl_x_displays = 0;

pl_scr_t *
pl_multihead(pl_scr_t *other, int number)
{
  pl_x_display_t *xdpy = other->xdpy;
  return (xdpy->dpy && number<ScreenCount(xdpy->dpy) && number>0)?
    x_screen(xdpy, number) : 0;
}

void (*pl_on_connect)(int dis, int fd) = 0;

pl_scr_t *
pl_connect(char *server_name)
{
  extern void _pl_x_parse_fonts(pl_x_display_t *xdpy);
  pl_x_display_t *xdpy;
  char *opt;
  int i;
  Display *dpy;
  if (!x_err_installed) {
    XSetErrorHandler(&_pl_x_err_handler);
    XSetIOErrorHandler(&_pl_x_panic);
    x_err_installed = 1;
  }
  dpy = XOpenDisplay(server_name);
  if (!dpy) return 0;
  if (pl_on_connect) pl_on_connect(0, ConnectionNumber(dpy));

  xdpy = pl_malloc(sizeof(pl_x_display_t));
  if (!xdpy) return 0;
  xdpy->panic = 0;
  xdpy->screens = 0;
  xdpy->next = 0;
  xdpy->dpy = dpy;
  xdpy->wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  xdpy->wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  xdpy->id2pwin = pl_halloc(16);

  for (i=0 ; i<=PL_NONE ; i++) xdpy->cursors[i] = None;

  xdpy->font = 0;
  xdpy->unload_font = 1;  /* assume success */
  for (i=0 ; i<PL_FONTS_CACHED ; i++) {
    xdpy->cached[i].f = 0;
    xdpy->cached[i].font = xdpy->cached[i].pixsize = 0;
    xdpy->cached[i].next = -1;
  }
  xdpy->most_recent = -1;
  for (i=0 ; i<20 ; i++) {
    xdpy->available[i].nsizes = 0;
    xdpy->available[i].sizes = 0;
    xdpy->available[i].names = 0;
  }
  _pl_x_parse_fonts(xdpy);  /* see fonts.c */

  /* find default font */

  if (_pl_x_xfont != NULL) {
    opt = _pl_x_xfont;
  } else {
    opt = XGetDefault(dpy, "Gist", "boldfont");
    if (opt == NULL) opt = XGetDefault(dpy, "Gist", "font");
    if (opt == NULL) opt = XGetDefault(dpy, "Gist", "Font");
  }
  if (opt != NULL) xdpy->font = XLoadQueryFont(dpy, opt);
  if (xdpy->font == NULL) xdpy->font = XLoadQueryFont(dpy, "9x15bold");
  if (xdpy->font == NULL) xdpy->font = XLoadQueryFont(dpy, "8x13bold");
  if (xdpy->font == NULL) xdpy->font = XLoadQueryFont(dpy, "9x15");
  if (xdpy->font == NULL) xdpy->font = XLoadQueryFont(dpy, "8x13");
  if (xdpy->font == NULL) xdpy->font = XLoadQueryFont(dpy, "fixed");
  if (xdpy->font == NULL) {
    /* note: section 6.2.2 of O'Reilly volume one promises that
     * the font associated with the default GC is always loaded */
    XGCValues values;
    GC gc = DefaultGC(dpy, DefaultScreen(dpy));
    xdpy->unload_font = 0;  /* better not try to unload this one */
    if (XGetGCValues(dpy, gc, GCFont, &values)) {
      xdpy->font = XQueryFont(dpy, XGContextFromGC(gc));
      /* XQueryFont returns fid==gc, really need font ID */
      if (xdpy->font) xdpy->font->fid = values.font;
    }
  }
  if (xdpy->font == NULL) { /* according to O'Reilly(1) 6.2.2, this is impossible */
    x_disconnect(xdpy);
    return 0;
  }

  xdpy->motion_q = 0;

  {
    XModifierKeymap *xmkm = XGetModifierMapping(dpy);
    int n = xmkm->max_keypermod;
    KeySym keysym;
    KeyCode *keys[5];
    unsigned int states[5];
    int k, i;
    keys[0] = xmkm->modifiermap + n*Mod1MapIndex;
    keys[1] = xmkm->modifiermap + n*Mod2MapIndex;
    keys[2] = xmkm->modifiermap + n*Mod3MapIndex;
    keys[3] = xmkm->modifiermap + n*Mod4MapIndex;
    keys[4] = xmkm->modifiermap + n*Mod5MapIndex;
    states[0] = Mod1Mask;
    states[1] = Mod2Mask;
    states[2] = Mod3Mask;
    states[3] = Mod4Mask;
    states[4] = Mod5Mask;
    xdpy->alt_state = xdpy->meta_state = 0;
    for (k=0 ; k<5 ; k++) {
      for (i=0 ; i<n ; i++) {
        keysym = XKeycodeToKeysym(dpy, keys[k][i], 0);
        if (keysym==XK_Meta_L || keysym==XK_Meta_R) {
          xdpy->meta_state = states[k];
          break;
        } else if (keysym==XK_Alt_L || keysym==XK_Alt_R) {
          xdpy->alt_state = states[k];
          break;
        }
      }
    }
    XFreeModifiermap(xmkm);
  }

  xdpy->sel_owner = 0;
  xdpy->sel_string = 0;
  xdpy->n_menus = 0;

  /* set up X event handler */
  if (_pl_x_wire_events) _pl_x_wire_events(xdpy, 0);

  xdpy->next = _pl_x_displays;
  _pl_x_displays = xdpy;
  return x_screen(xdpy, DefaultScreen(dpy));
}

/* if this isn't set by pl_gui, never reference any of the unix event stuff */
void (*_pl_x_wire_events)(pl_x_display_t *xdpy, int disconnect) = 0;

/* xdpyndx is index into xdpynative, xdpyplay
 * this is a (perhaps misguided) attempt to assure that the
 * xdpynative<->xdpyplay correspondence is always valid:
 * the two are both set before the xdpyndx index is set;
 * hopefully setting the index is an atomic operation */
static int xdpyndx = 0;
static Display *xdpynative[2] = {0,0};
static pl_x_display_t *xdpyplay[2] = {0,0};

/* hopefully this is faster than ctx hash, since total number of
 * dpy pointers is very small (usually just one) */
pl_x_display_t *
_pl_x_dpy(Display *dpy)
{
  if (dpy==xdpynative[xdpyndx]) {
    /* usual case is lots of calls with same dpy, return immediately */
    return xdpyplay[xdpyndx];
  } else {
    pl_x_display_t *xdpy;
    int i = 1-xdpyndx;
    for (xdpy=_pl_x_displays ; xdpy ; xdpy=xdpy->next)
      if (xdpy->dpy == dpy) {
        xdpynative[i] = dpy;
        xdpyplay[i] = xdpy;
        break;
      }
    if (xdpy) xdpyndx = i;
    return xdpy;
  }
}

static void
x_disconnect(pl_x_display_t *xdpy)
{
  Display *dpy = xdpy->dpy;
  if (xdpyplay[0]==xdpy) {
    xdpynative[0] = 0;
    xdpyplay[0] = 0;
  } else if (xdpyplay[1]==xdpy) {
    xdpynative[1] = 0;
    xdpyplay[1] = 0;
  }
  if (dpy) {
    int i, j;
    pl_x_display_t **pxdpy = &_pl_x_displays;
    pl_hashtab_t *id2pwin = xdpy->id2pwin;
    if (!xdpy->panic) {
      Cursor cur;
      XFontStruct *font = xdpy->font;
      if (pl_on_connect) pl_on_connect(1, ConnectionNumber(dpy));
      if (font) {
        xdpy->font = 0;
        if (xdpy->unload_font) XFreeFont(dpy, font);
        else XFreeFontInfo((void *)0, font, 1);
      }
      for (i=0 ; i<PL_FONTS_CACHED ; i++) {
        font = xdpy->cached[i].f;
        if (!font) continue;
        xdpy->cached[i].f = 0;
        XFreeFont(dpy, font);
      }
      for (i=0 ; i<=PL_NONE ; i++) {
        cur = xdpy->cursors[i];
        xdpy->cursors[i] = None;
        if (cur!=None) XFreeCursor(dpy, cur);
      }
    }
    for (i=0 ; i<20 ; i++) {
      _pl_x_tmpzap(&xdpy->available[i].sizes);
      if (xdpy->available[i].nsizes) {
        for (j=0 ; j<xdpy->available[i].nsizes+1 ; j++)
          _pl_x_tmpzap(&xdpy->available[i].names[j]);
        xdpy->available[i].nsizes = 0;
      }
      _pl_x_tmpzap(&xdpy->available[i].names);
    }
    _pl_x_tmpzap(&xdpy->sel_string);
    if (_pl_x_wire_events) _pl_x_wire_events(xdpy, 1);
    if (!xdpy->panic) XCloseDisplay(dpy);
    while (*pxdpy && *pxdpy!=xdpy) pxdpy = &xdpy->next;
    if (*pxdpy) *pxdpy = xdpy->next;
    if (id2pwin) {
      void (*no_action)(void *) = 0;
      xdpy->id2pwin = 0;
      pl_hfree(id2pwin, no_action);
    }
    xdpy->dpy = 0;
    if (!xdpy->panic) pl_free(xdpy);
  }
}

static pl_scr_t *
x_screen(pl_x_display_t *xdpy, int number)
{
  char *opt;
  XColor color;
  Colormap cmap;
  XGCValues values;
  int vclass;
  Display *dpy = xdpy->dpy;
  pl_scr_t *s = pl_malloc(sizeof(pl_scr_t));
  if (!s) return 0;

  s->xdpy = xdpy;

  s->scr_num = number;
  s->root = RootWindow(dpy, number);
  s->width = DisplayWidth(dpy, number);
  s->height = DisplayHeight(dpy, number);
  s->depth = DefaultDepth(dpy, number);
  s->nwins = 0;
  cmap = DefaultColormap(dpy, number);

  s->pixels = 0;
  s->cmap = None;
  s->free_colors = 0;
  s->gc = 0;
  {
    /* create bitmap (same as X11/bitmaps/gray) for hi/lo effect
     * -- may not be used, but see _pl_x_rotzap */
    char bits[2];
    bits[0] = 0x01;  bits[1] = 0x02;
    s->gray = XCreateBitmapFromData(dpy, s->root, bits, 2, 2);
  }
  s->shared = 0;

  /* interrupt-protected temporaries for rotated text */
  s->tmp = 0;
  s->image = 0;
  s->own_image_data = 0;
  s->pixmap = None;
  s->rotgc = 0;

  /* set up color handling
   *
   * - GrayScale and DirectColor models are silly and confusing:
   *   since the screen can simultaneously display every possible color,
   *   the changeable colormaps simply change the names of those colors
   *   (hopefully the server doesn't gratuitously flash all the other
   *   windows when colormaps are installed)
   *
   *   therefore, I create a complete colormap for those cases, and
   *   arrange for it to be installed for every Gist window -- this
   *   makes GrayScale equivalent to StaticGray and DirectColor
   *   equivalent to TrueColor
   *
   * - PseudoColor is infinitely more complicated, and requires special
   *   handling within each window, not here
   */
  s->vclass = vclass = DefaultVisual(dpy, number)->class;

  if (vclass != PseudoColor) {
    /* pixels[i] is the pixel value for gray (i=0 black, i=255 white)
     * - for TrueColor or DirectColor, an arbitrary color can be
     *   constructed by using the s, g, and b masks to build the
     *   required pixel:
     *   (pixels[r]&rmask)|(pixels[g]&gmask)|(pixels[b]&bmask)
     */
    int i;
    int mutible = (vclass==DirectColor || vclass==GrayScale);
    pl_col_t *pixels = s->pixels = pl_malloc(sizeof(unsigned long)*256);
    Visual *visual = DefaultVisual(dpy, number);

    if (!pixels) {
      pl_disconnect(s);
      return 0;
    }

    s->rmask = visual->red_mask;
    s->gmask = visual->green_mask;
    s->bmask = visual->blue_mask;

    if (mutible)
      cmap = s->cmap = XCreateColormap(dpy, s->root, visual, AllocNone);

    color.flags = color.pad = 0;
    color.pixel = 0;

    /* this is very slow, but there doesn't seem to be any faster way
     * to get the correspondence between pixel values and rgb values */
    for (i=0 ; i<256 ; i++) {
      color.red = color.green = color.blue = (i<<8);
      if (XAllocColor(dpy, cmap, &color)) {
        pixels[i] = color.pixel;
        if (!mutible) XFreeColors(dpy, cmap, &color.pixel, 1, 0UL);
      } else {
        /* should be impossible, but probably isn't (serious error) */
        pixels[i] = i;
      }
    }

  } else { /* PseudoColor */
    s->rmask = s->gmask = s->bmask = 0;
  }

  /* get standard colors */

  if (_pl_x_foreground != NULL) {
    opt = _pl_x_foreground;
  } else {
    opt = XGetDefault(dpy, "Gist", "foreground");
    if (opt == NULL) {
      opt = XGetDefault(dpy, "Gist", "Foreground");
    }
  }
  if (opt != NULL && XAllocNamedColor(dpy, cmap, opt,
                                      &s->colors[1], &color)) {
      s->free_colors |= 2;
  } else {
    s->colors[1].pixel = BlackPixel(dpy, number);
    XQueryColor(dpy, cmap, &s->colors[1]);
  }

  if (_pl_x_background != NULL) {
    opt = _pl_x_background;
  } else {
    opt = XGetDefault(dpy, "Gist", "background");
    if (opt == NULL) {
      opt = XGetDefault(dpy, "Gist", "Background");
    }
  }
  if (opt != NULL && XAllocNamedColor(dpy, cmap, opt,
                                      &s->colors[0], &color)) {
      s->free_colors |= 1;
  } else {
    long bright = ((long)s->colors[1].red +
                   (long)s->colors[1].green +
                   (long)s->colors[1].blue);
    s->colors[0].pixel = (bright < 98302 ?
                          WhitePixel(dpy, number) :
                          BlackPixel(dpy, number));
    XQueryColor(dpy, cmap, &s->colors[0]);
  }

  s->colors[2].pixel = BlackPixel(dpy, number);
  XQueryColor(dpy, cmap, &s->colors[2]);
  s->colors[3].pixel = WhitePixel(dpy, number);
  XQueryColor(dpy, cmap, &s->colors[3]);

#define alloc_xcolor(name, index)                                       \
  do {                                                                  \
    if (XAllocNamedColor(dpy, cmap, name, &s->colors[index], &color)) { \
      s->free_colors |= (1 << (index));                                 \
    } else {                                                            \
      s->colors[index] = s->colors[1];                                  \
    }                                                                   \
  } while (0)
  alloc_xcolor("red",     4);
  alloc_xcolor("green",   5);
  alloc_xcolor("blue",    6);
  alloc_xcolor("cyan",    7);
  alloc_xcolor("magenta", 8);
  alloc_xcolor("yellow",  9);
#undef alloc_xcolor

  /* initialize generic graphics context and state */

  values.font = xdpy->font->fid;
  values.foreground = s->gc_color = s->colors[1].pixel;
  values.background = s->colors[0].pixel;
  values.join_style = JoinRound;
  s->gc_font = PL_GUI_FONT | PL_BOLD;  /* not any font */
  s->gc_width = 0;
  s->gc_type = 1023; /* CapButt is default, not used by Gist */
  s->gc_fillstyle = FillSolid;
  s->gc_w_clip = 0;
  s->gc = XCreateGC(dpy, s->root,
                    GCForeground | GCBackground | GCFont | GCJoinStyle,
                    &values);

  /* get special GUI colors */

  if (pl_try_grays(dpy, cmap, &s->colors[10], 100, 11))
    s->free_colors |= 1024;
  else
    s->colors[10] = s->colors[2];
  if (pl_try_grays(dpy, cmap, &s->colors[11], 150, 11)) {
    s->free_colors |= 2048;
    if (!(s->free_colors&1024))
      s->colors[10] = s->colors[11];
  } else {
    s->colors[11] = s->colors[10];
  }
  if (pl_try_grays(dpy, cmap, &s->colors[13], 214, 11))
    s->free_colors |= 8192;
  else
    s->colors[13] = s->colors[3];
  if (pl_try_grays(dpy, cmap, &s->colors[12], 190, 11)) {
    s->free_colors |= 4096;
    if (!(s->free_colors&4096))
      s->colors[13] = s->colors[12];
  } else {
    s->colors[12] = s->colors[13];
  }

  /* set up special graphics contexts for GUI */

  s->gui_flags = 0;
  if (!(s->free_colors&1024) && !(s->free_colors&2048) &&
      !(s->free_colors&4096) && !(s->free_colors&8192)) {
    /* if got no grays at all, make middle two stippled */
    s->gui_flags |= 1;
    XSetStipple(dpy, s->gc, s->gray);
  }

  if (pl_signalling) pl_abort();

  s->next = xdpy->screens;
  xdpy->screens = s;
  return s;
}

static int
pl_try_grays(Display *dpy, Colormap cmap, XColor *color, int c, int dc)
{
  int i;
  for (i=0 ; dc-- ; i=((i<0)?1:-1)-i) {
    color->red = color->green = color->blue = ((c+i)<<8);
    if (XAllocColor(dpy, cmap, color))
      return 1;
  }
  return 0;
}

void (*_pl_x_on_panic)(pl_scr_t *s) = 0;

void
pl_disconnect(pl_scr_t *s)
{
  int i;
  pl_x_display_t *xdpy = s->xdpy;
  Display *dpy = xdpy? xdpy->dpy : 0;

  _pl_x_tmpzap(&s->pixels);
  _pl_x_rotzap(s);

  if (dpy && !xdpy->panic) {
    Colormap cmap = s->cmap;
    if (cmap==None) cmap = DefaultColormap(dpy, s->scr_num);
    for (i=0 ; s->free_colors && i<14 ; i++) {
      if (s->free_colors & (1<<i)) {
        s->free_colors &= ~(1<<i);
        XFreeColors(dpy, cmap, &s->colors[i].pixel, 1, 0UL);
      }
    }
    _pl_x_nuke_shared(s);

    _pl_x_cmzap(dpy, &s->cmap);
    _pl_x_pxzap(dpy, &s->gray);
    _pl_x_gczap(dpy, &s->gc);
  }

  if (xdpy) {
    pl_scr_t **pthis = &xdpy->screens;
    while (*pthis && *pthis!=s) pthis = &(*pthis)->next;
    if (*pthis) *pthis = s->next;
    if (xdpy->panic==1 && _pl_x_on_panic) _pl_x_on_panic(s);
    if (!xdpy->screens) x_disconnect(xdpy);
    s->xdpy = 0;
  }
  pl_free(s);
}

int
pl_sshape(pl_scr_t *s, int *width, int *height)
{
  *width = s->width;
  *height = s->height;
  return s->depth;
}

int
pl_wincount(pl_scr_t *s)
{
  return s? s->nwins : 0;
}

/* ------------------------------------------------------------------------ */

void
_pl_x_rotzap(pl_scr_t *s)
{
  pl_x_display_t *xdpy = s->xdpy;
  Display *dpy = xdpy->dpy;
  _pl_x_tmpzap(&s->tmp);
  if (!xdpy->panic) _pl_x_gczap(dpy, &s->rotgc);
  _pl_x_imzap(s);
  if (!xdpy->panic && s->pixmap!=None) {
    /* note -- XSetStipple gives error if pixmap is None?? */
    if (s->gray!=None) XSetStipple(dpy, s->gc, s->gray);
    XSetTSOrigin(dpy, s->gc, 0, 0);
    _pl_x_pxzap(dpy, &s->pixmap);
  }
}

void
_pl_x_tmpzap(void *p)
{
  void **ptmp = p;
  void *tmp = *ptmp;
  if (tmp) {
    *ptmp = 0;
    pl_free(tmp);
  }
}

void
_pl_x_gczap(Display *dpy, GC *pgc)
{
  GC gc = *pgc;
  if (gc) {
    *pgc = 0;
    XFreeGC(dpy, gc);
  }
}

void
_pl_x_imzap(pl_scr_t *s)
{
  XImage *im = s->image;
  if (im) {
    if (s->own_image_data) {
      void *data = im->data;
      if (data) {
        im->data = 0;
        pl_free(data);
      }
    }
    s->image = 0;
    XDestroyImage(im);
  }
}

void
_pl_x_pxzap(Display *dpy, Pixmap *ppx)
{
  Pixmap px = *ppx;
  if (px!=None) {
    *ppx = None;
    XFreePixmap(dpy, px);
  }
}

void
_pl_x_cmzap(Display *dpy, Colormap *pcm)
{
  Colormap cm = *pcm;
  if (cm!=None) {
    *pcm = None;
    XFreeColormap(dpy, cm);
  }
}
