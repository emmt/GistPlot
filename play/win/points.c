/*
 * $Id: points.c,v 1.1 2005-09-18 22:05:38 dhmunro Exp $
 * pl_i_pnts, pl_d_pnts, pl_d_map for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"

POINT _pl_w_pt_list[2050];
int _pl_w_pt_count = 0;
static double w_pt_xa=1., w_pt_xb=0., w_pt_ya=1., w_pt_yb=0.;

/* ARGSUSED */
void
pl_d_map(pl_win_t *w, double xt[], double yt[], int set)
{
  if (set) {
    w_pt_xa = xt[0];
    w_pt_xb = xt[1];
    w_pt_ya = yt[0];
    w_pt_yb = yt[1];
  } else {
    xt[0] = w_pt_xa;
    xt[1] = w_pt_xb;
    yt[0] = w_pt_ya;
    yt[1] = w_pt_yb;
  }
}

void
pl_i_pnts(pl_win_t *w, const int *x, const int *y, int n)
{
  if (n == -1) {
    if (_pl_w_pt_count < 2048) {
      n = _pl_w_pt_count++;
      _pl_w_pt_list[n].x = x[0];
      _pl_w_pt_list[n].y = y[0];
    } else {
      _pl_w_pt_count = 0;
    }
  } else {
    POINT *wrk = _pl_w_pt_list;
    if (n >= 0) {
      _pl_w_pt_count = n;
    } else {
      wrk += _pl_w_pt_count;
      n = -n;
      _pl_w_pt_count += n;
    }
    if (_pl_w_pt_count <= 2048) {
      while (n--) {
        wrk[0].x = *x++;
        wrk[0].y = *y++;
        wrk++;
      }
    } else {
      _pl_w_pt_count = 0;
    }
  }
}

/* ARGSUSED */
void
pl_d_pnts(pl_win_t *w, const double *x, const double *y, int n)
{
  if (n == -1) {
    if (_pl_w_pt_count < 2048) {
      n = _pl_w_pt_count++;
      _pl_w_pt_list[n].x = (long)(w_pt_xa*x[0] + w_pt_xb);
      _pl_w_pt_list[n].y = (long)(w_pt_ya*y[0] + w_pt_yb);
    } else {
      _pl_w_pt_count = 0;
    }
  } else {
    POINT *wrk = _pl_w_pt_list;
    if (n >= 0) {
      _pl_w_pt_count = n;
    } else {
      wrk += _pl_w_pt_count;
      n = -n;
      _pl_w_pt_count += n;
    }
    if (_pl_w_pt_count <= 2048) {
      while (n--) {
        wrk[0].x = (long)(w_pt_xa*(*x++) + w_pt_xb);
        wrk[0].y = (long)(w_pt_ya*(*y++) + w_pt_yb);
        wrk++;
      }
    } else {
      _pl_w_pt_count = 0;
    }
  }
}
