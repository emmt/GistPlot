/*
 * $Id: events.c,v 1.3 2007-06-24 20:32:49 dhmunro Exp $
 * X11 event handler
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "playx.h"

#include "playu.h"
#include "play/std.h"

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

static void (*xon_expose)(void *c,int *xy)= 0;
static void (*xon_destroy)(void *c)= 0;
static void (*xon_resize)(void *c,int w,int h)= 0;
static void (*xon_focus)(void *c,int in)= 0;
static void (*xon_key)(void *c,int k,int md)= 0;
static void (*xon_click)(void *c,int b,int md,int x,int y,unsigned long ms)=0;
static void (*xon_motion)(void *c,int md,int x,int y)= 0;
static void (*xon_deselect)(void *c)= 0;

static void x_wirer(pl_x_display_t *xdpy, int disconnect);
static void x_event(void *wsdata);
static int x_prepoll(void *wsdata);

static int x_keycode(pl_x_display_t *xdpy, XKeyEvent *xkey, int *pkey, int *pmods);
static int x_button(unsigned int button);
static int x_modifiers(pl_x_display_t *xdpy, unsigned int state);
static void x_sel_send(pl_x_display_t *xdpy, pl_win_t *w, XEvent *event);
static Bool xmotion_counter(Display *dpy, XEvent *event, char *arg);
static Bool xmatch_all(Display *dpy, XEvent *event, char *arg);
static Bool xselect_find(Display *dpy, XEvent *event, char *arg);

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
  xon_expose = on_expose;
  xon_destroy = on_destroy;
  xon_resize = on_resize;
  xon_focus = on_focus;
  xon_key = on_key;
  xon_click = on_click;
  xon_motion = on_motion;
  xon_deselect = on_deselect;
  _pl_x_on_panic = on_panic;
  _pl_x_wire_events = &x_wirer;
}

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
  *on_expose = xon_expose;
  *on_destroy = xon_destroy;
  *on_resize = xon_resize;
  *on_focus = xon_focus;
  *on_key = xon_key;
  *on_click = xon_click;
  *on_motion = xon_motion;
  *on_deselect = xon_deselect;
  *on_panic = _pl_x_on_panic;
}

static void
x_wirer(pl_x_display_t *xdpy, int disconnect)
{
  if (!disconnect) {
    _pl_u_event_src(ConnectionNumber(xdpy->dpy), &x_event, xdpy);
    _pl_u_prepoll(&x_prepoll, xdpy);
  } else {
    _pl_u_event_src(ConnectionNumber(xdpy->dpy), (void (*)(void*))0, xdpy);
    _pl_u_prepoll((int (*)(void*))0, xdpy);
  }
}

static int
x_prepoll(void *wsdata)
{
  pl_x_display_t *xdpy = wsdata;
  Display *dpy = xdpy->dpy;
  if (QLength(dpy)) {
    x_event(xdpy);
    return 1;
  }
  XFlush(dpy);
  xdpy->motion_q = 0;  /* noop unless XSync has cleared off pending events */
  if (pl_signalling) pl_abort();
  return 0;
}

static void
x_event(void *wsdata)
{
  pl_x_display_t *xdpy = wsdata;
  Display *dpy = xdpy->dpy;
  pl_win_t *w = 0;
  Window xwin;
  XEvent event;

  /* X error events trigger poll, but XNextEvent doesn't return them
   * and can block forever waiting for a true event after calling
   * the application on_error.  Sigh. */
  /* XNextEvent(dpy, &event); */
  if (!XCheckIfEvent(dpy, &event, &xmatch_all, (char *)0))
    return;
  xwin = event.xany.window;
  w = _pl_x_pwin(xdpy, xwin);
  if (!w) {
    /* this window is lost, be a good citizen for confused other client */
    if (event.type==SelectionRequest) x_sel_send(xdpy, (pl_win_t*)0, &event);
    return;
  }

  switch (event.type) {
  case Expose:
    /* expose triggers all drawing operations */
    if (xon_expose) {
      int xy[4], xx, yy;
      xy[0] = event.xexpose.x;
      xy[1] = event.xexpose.y;
      xy[2] = event.xexpose.x+event.xexpose.width;
      xy[3] = event.xexpose.y+event.xexpose.height;
      /* modern window managers generate tons of expose events
       * during resize operations with count==0
       * -- in fact, with "opaque resize", event the newer code
       *    can cause unwanted redraws (maybe no more than one?)
       * while (event.xexpose.count) {
       *   XWindowEvent(dpy, xwin, ExposureMask, &event);
       */
      while (XCheckWindowEvent(dpy, xwin, ExposureMask, &event)) {
        if (event.xexpose.x<xy[0]) xy[0] = event.xexpose.x;
        if (event.xexpose.y<xy[1]) xy[1] = event.xexpose.y;
        xx = event.xexpose.x+event.xexpose.width;
        yy = event.xexpose.y+event.xexpose.height;
        if (xx>xy[2]) xy[2] = xx;
        if (yy>xy[3]) xy[3] = yy;
      }
      xon_expose(w->context, (xy[0]<=0 && xy[1]<=0 && xy[2]>=w->width &&
                              xy[3]>=w->height)? 0 : xy);
    }
    break;

  case ConfigureNotify:
    /* if test only necessary if SubstructureNotifyMask used someday */
    if (event.xconfigure.window == xwin) {
      int resize = (event.xconfigure.width!=w->width ||
                   event.xconfigure.height!=w->height);
      Window root, parent, *child = 0;
      unsigned int nchild, wd, ht, bo, dp;
      int x, y, xw, yw;
      w->x = event.xconfigure.x;
      w->y = event.xconfigure.y;
      /* event.xconfigure returns bogus x and y values
       * after a resize, although seems okay after a move */
      xw = yw = 0;
      root = None;
      while (XGetGeometry(dpy, xwin, &root, &x, &y, &wd, &ht, &bo, &dp) &&
             XQueryTree(dpy, xwin, &root, &parent, &child, &nchild)) {
        if (child) XFree(child);
        child = 0;
        xw += x;
        yw += y;
        xwin = parent;
        if (xwin == root) break;
      }
      if (child) XFree(child);
      if (xwin==root) w->x = xw, w->y = yw;
      if (resize && xon_resize)
        xon_resize(w->context,
                   (w->width = event.xconfigure.width),
                   (w->height = event.xconfigure.height));
    }
    break;

  case FocusIn:
  case FocusOut:
    if (xon_focus) xon_focus(w->context, event.type==FocusIn);
    break;

  case EnterNotify:
  case LeaveNotify:
    if (xon_focus) xon_focus(w->context, (event.type==EnterNotify)|2);
    break;

  case KeyPress:
    if (xon_key) {
      int key, mods;
      if (x_keycode(xdpy, &event.xkey, &key, &mods))
        xon_key(w->context, key, mods);
    }
    break;

  case ButtonPress:
  case ButtonRelease:
    if (event.xbutton.same_screen) {
      if (xon_click) {
        int btn = x_button(event.xbutton.button);
        int mods = x_modifiers(xdpy, event.xbutton.state);
        xon_click(w->context, btn, mods,
                  event.xbutton.x, event.xbutton.y,
                  event.xbutton.time);
      }
    }
    break;

  case MotionNotify:
    if (event.xbutton.same_screen) {
      /* skip this if we've already seen more queued motion events */
      if (!xdpy->motion_q) {
        int x = event.xmotion.x;
        int y = event.xmotion.y;
        if (xon_motion) {
          int mods = x_modifiers(xdpy, event.xmotion.state);
          xon_motion(w->context, mods, x, y);
        }
        /* count number of queued motion events when this one finished
         * being serviced -- all but final will be skipped */
        xdpy->motion_q = 0;
        XCheckIfEvent(dpy, &event, &xmotion_counter,
                      (char *)&xdpy->motion_q);
        if (xdpy->motion_q) xdpy->motion_q--;
      } else {
        xdpy->motion_q--;
      }
    }
    break;

  case ClientMessage:
    if (xon_destroy && event.xclient.format==32 &&
        event.xclient.message_type==xdpy->wm_protocols &&
        event.xclient.data.l[0]==xdpy->wm_delete) {
      xon_destroy(w->context);
      pl_destroy(w);
    }
    break;

  case DestroyNotify:
    /* this is equivalent to above ClientMessage for subwindow case */
    if (xon_destroy && w->d!=None) {
      xon_destroy(w->context);
      pl_destroy(w);
    }
    break;

  case SelectionClear:
    if (xon_deselect) xon_deselect(w->context);
    break;
  case SelectionNotify:
    /* should never get these - handled in pl_sel_paste */
    break;
  case SelectionRequest:
    /* somebody wants our selection */
    x_sel_send(xdpy, w, &event);
    break;

  default:
    /* other possibilities are:
     * StructureNotifyMask: CirculateNotify, GravityNotify,
     *                      MapNotify, ReparentNotify, UnmapNotify
     * (always selected): MappingNotify */
    break;
  }
}

void
pl_qclear(void)
{
  pl_x_display_t *xdpy;
  Display *dpy = 0;
  for (xdpy=_pl_x_displays ; xdpy ; xdpy=xdpy->next) {
    dpy = (xdpy && !xdpy->panic)? xdpy->dpy : 0;
    if (dpy) {
      XEvent event;
      /* could use XSync here, but that would be antisocial if another
       * client has sent us a SelectionRequest which is currently queued */
      if (xdpy->sel_owner) pl_scopy(xdpy->sel_owner, (char *)0, 0);
      else if (xdpy->sel_string) _pl_x_tmpzap(&xdpy->sel_string);
      while (XCheckIfEvent(dpy, &event, &xmatch_all, (char *)0))
        if (event.type==SelectionRequest)
          x_sel_send(xdpy, (pl_win_t*)0, &event);
    }
  }
}

static void
x_sel_send(pl_x_display_t *xdpy, pl_win_t *w, XEvent *event)
{
  Window requestor = event->xselectionrequest.requestor;
  if (xdpy->sel_owner==w && xdpy->sel_string &&
      event->xselectionrequest.selection==XA_PRIMARY &&
      event->xselectionrequest.target==XA_STRING) {
    int len = 0;
    if (xdpy->sel_string) while (xdpy->sel_string[len]) len++;
    event->xselection.property = event->xselectionrequest.property;
    if (event->xselection.property==None)
      event->xselection.property = XA_STRING;
    XChangeProperty(xdpy->dpy, requestor, event->xselection.property,
                    XA_STRING, 8, PropModeReplace,
                    (void *)xdpy->sel_string, len);
  } else {
    event->xselection.property = None;
  }
  event->type = SelectionNotify;
  event->xselection.send_event = True;
  event->xselection.requestor = requestor;
  event->xselection.selection = XA_PRIMARY;
  event->xselection.target = XA_STRING;
  event->xselection.time = event->xselectionrequest.time;
  XSendEvent(xdpy->dpy, requestor, False, 0L, event);
}

static int x_keypad[15] = {
  PL_F1, PL_F2, PL_F3, PL_F4, PL_HOME, PL_LEFT, PL_UP, PL_RIGHT, PL_DOWN,
  PL_PGUP, PL_PGDN, PL_END, 0, PL_INSERT, '\177' };

static int
x_keycode(pl_x_display_t *xdpy, XKeyEvent *xkey, int *pkey, int *pmods)
{
  char buf[16];
  KeySym keysym;
  XComposeStatus compose;
  int key;
  int len = XLookupString(xkey, buf, 15, &keysym, &compose);
  int mods = x_modifiers(xdpy, xkey->state);

  if (keysym>=XK_KP_Space && keysym<=XK_KP_9) {
    mods |= PL_KEYPAD;
    if (keysym==XK_KP_Space)
      key = ' ';
    else if (keysym>=XK_KP_F1 && keysym<=XK_KP_Delete)
      key = x_keypad[keysym-XK_KP_F1];
    else
      key = (keysym-XK_KP_Space);
  } else if (keysym==XK_Home) key = PL_HOME;
  else if (keysym==XK_Left)   key = PL_LEFT;
  else if (keysym==XK_Up)     key = PL_UP;
  else if (keysym==XK_Right)  key = PL_RIGHT;
  else if (keysym==XK_Down)   key = PL_DOWN;
  else if (keysym==XK_Prior)  key = PL_PGUP;
  else if (keysym==XK_Next)   key = PL_PGDN;
  else if (keysym==XK_End)    key = PL_END;
  else if (keysym==XK_Insert) key = PL_INSERT;
  else if (keysym>=XK_F1 && keysym<=XK_F35)
    key = PL_F1 + (keysym-XK_F1);
  else if (len==1) key = buf[0];
  else return 0;

  *pkey = key;
  *pmods = mods;
  return 1;
}

static int
x_button(unsigned int button)
{
  int b;

  /* depressingly stupid, since these really are 1-5 */
  if (button==Button1) b = 1;
  else if (button==Button2) b = 2;
  else if (button==Button3) b = 3;
  else if (button==Button4) b = 4;
  else if (button==Button5) b = 5;
  else b = 0;

  return b;
}

static int
x_modifiers(pl_x_display_t *xdpy, unsigned int state)
{
  int s = 0;

  if (state&Button1Mask) s |= PL_BTN1;
  if (state&Button2Mask) s |= PL_BTN2;
  if (state&Button3Mask) s |= PL_BTN3;
  if (state&Button4Mask) s |= PL_BTN4;
  if (state&Button5Mask) s |= PL_BTN5;
  if (state&ControlMask) s |= PL_CONTROL;
  if (state&ShiftMask) s |= PL_SHIFT;
  if (state&xdpy->meta_state) s |= PL_META;
  if (state&xdpy->alt_state) s |= PL_ALT;

  return s;
}

/* ARGSUSED */
static Bool
xmatch_all(Display *dpy, XEvent *event, char *arg)
{
  return True;
}

/* ARGSUSED */
static Bool
xmotion_counter(Display *dpy, XEvent *event, char *arg)
{
  int *pn_motion = (int *)arg;
  if (event->type==MotionNotify) (*pn_motion)+= 1;
  return False;
}

int
pl_scopy(pl_win_t *w, char *string, int n)
{
  int clearing = !string || n<0;
  pl_x_display_t *xdpy = w->s->xdpy;
  _pl_x_tmpzap(&xdpy->sel_string);
  if ((clearing? xdpy->sel_owner==w : xdpy->sel_owner!=w) && !xdpy->panic) {
    Window xwin;
    if (clearing) {
      xdpy->sel_owner = 0;
      xwin = None;
    } else {
      pl_win_t *tmp = xdpy->sel_owner;
      xdpy->sel_owner = w;
      xwin = w->d;
      w = tmp;
    }

    /* dehighlighting has to happen here (might be triggered by X event)
     * - highlighting should be done on return from pl_scopy */
    if (w && xon_deselect) xon_deselect(w->context);

    /* O'Reilly vol 1 section 12.4 (Interclient Communications/Selections)
     * cautions against using CurrentTime here, but in vol 2 under
     * XSetSelectionOwner, they specifically approve the practice
     * - since the event that triggers this could in principle have
     *   come from an entirely different input channel, anything
     *   other than CurrentTime here would be very difficult */
    XSetSelectionOwner(xdpy->dpy, XA_PRIMARY, xwin, CurrentTime);
    if (xwin!=None && XGetSelectionOwner(xdpy->dpy, XA_PRIMARY)!=xwin) {
      xdpy->sel_owner = 0;
      return 1;
    }
    if (pl_signalling) pl_abort();
  }

  if (!clearing)
    xdpy->sel_string = n? pl_strncat((char *)0, string, n) : pl_strcpy(string);
  return 0;
}

char *
pl_spaste(pl_win_t *w)
{
  Window xwin = w->d;
  pl_x_display_t *xdpy = w->s->xdpy;
  Display *dpy = xdpy->dpy;
  int fd, n, format;
  XEvent event;
  Atom type;
  unsigned long nitems, after;
  unsigned char *prop = 0;

  /* if we own the selection, just return it */
  if (xdpy->sel_owner) {
    pl_win_t *ww = xdpy->sel_owner;
    if (XGetSelectionOwner(dpy, XA_PRIMARY)==ww->d)
      return xdpy->sel_string;
    xdpy->sel_owner = 0;
  }
  _pl_x_tmpzap(&xdpy->sel_string);

  /* tell selection owner to copy selection to STRING property on xwin */
  XConvertSelection(dpy, XA_PRIMARY, XA_STRING, XA_STRING,
                    xwin, CurrentTime);
  /* wait for the SelectionNotify event to arrive
   * - if this were guaranteed, wouldn't need _pl_u_poll1, but I don't
   *   see how a guarantee of return in finite time can be made... */
  n = 0;
  fd = ConnectionNumber(dpy);
  while (!XCheckIfEvent(dpy, &event, &xselect_find, (char *)&xwin)) {
    if ((++n) > 20) return 0;  /* give up after at most 4 seconds */
    _pl_u_poll1(fd, 200);
  }

  /* retreve up to 16k characters, while deleting STRING property */
  if (XGetWindowProperty(dpy, xwin, XA_STRING, 0L, 4000L, True, XA_STRING,
                         &type, &format, &nitems, &after, &prop)==Success) {
    if (type==XA_STRING && format==8)
      xdpy->sel_string = pl_strcpy((char *)prop);
    if (prop) XFree((char *)prop);
  }

  if (pl_signalling) pl_abort();

  return xdpy->sel_string;
}

static Bool
xselect_find(Display *dpy, XEvent *event, char *arg)
{
  Window xwin = *((Window *)arg);
  return (event->type==SelectionNotify &&
          event->xselection.requestor==xwin);
}
