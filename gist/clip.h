/*
 * $Id: gist/clip.h,v 1.1 2005-09-18 22:04:25 dhmunro Exp $
 * Routines to perform floating point clipping operations.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */
/*
    Interface routines:

    *** include <gist/clip.h> to define gp_xclip, gp_yclip ***
    int nc;
    gp_clip_setup(xmin, xmax, ymin, ymax);

       *** to draw an open curve, use ***
    if (gp_clip_begin(x, y, n, 0)) DrawPolyline(x, y, n);
    else while (nc=gp_clip_more()) DrawPolyline(gp_xclip, gp_yclip, nc);

       *** to draw a closed curve, use ***
    if (gp_clip_begin(x, y, n, 1)) DrawPolygon(x, y, n);
    else while (nc=gp_clip_more()) DrawPolyline(gp_xclip, gp_yclip, nc);

       *** to draw a set of unconnected points, use ***
    if (nc=gp_clip_points(x, y, n)) DrawPolymarker(gp_xclip, gp_yclip, nc);

       *** to draw a closed filled curve, use ***
    if (nc=gp_clip_filled(x, y, n)) DrawFilledPolygon(gp_xclip, gp_yclip, nc);

       *** to draw a disjoint segments, use ***
    if (nc=gp_clip_disjoint(x0, y0, x1, y1, n))
       DrawDisjoint(gp_xclip, gp_yclip, gp_xclip1, gp_yclip1, nc);
 */

#ifndef _GIST_CLIP_H
#define _GIST_CLIP_H

#ifndef _GIST2_H
#ifndef SINGLE_P
typedef double gp_real_t;
#else
typedef float gp_real_t;
#endif
#endif

extern gp_real_t *gp_xclip, *gp_yclip, *gp_xclip1, *gp_yclip1;

extern void gp_clip_free_workspace(void);

extern void gp_clip_setup(gp_real_t xmn, gp_real_t xmx, gp_real_t ymn, gp_real_t ymx);

extern int gp_clip_begin(const gp_real_t* xx, const gp_real_t* yy, int nn, int clsd);

extern int gp_clip_more(void);

extern int gp_clip_points(const gp_real_t* xx, const gp_real_t* yy, int nn);

extern int gp_clip_filled(const gp_real_t* xx, const gp_real_t* yy, int nn);

extern int gp_clip_disjoint(const gp_real_t* x0, const gp_real_t* y0,
                        const gp_real_t* x1, const gp_real_t* y1, int nn);

extern int gp_clip_test(const gp_real_t* xx, const gp_real_t* yy, int nn, int clsd,
                    const gp_real_t* box);

#endif
