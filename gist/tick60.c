/*
 * $Id: tick60.c,v 1.1 2005-09-18 22:04:36 dhmunro Exp $
 * Implement base 60 ticks and labels for coordinate systems.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist2.h"

/* Use sprintf function in labeling functions */
#include <stdio.h>

extern double ceil(double);

/* Here are the possible intervals for a base 60 scale.
   The 720 interval is skipped if the 1800 was present, otherwise all
   these intervals will be present in the tick hierarchy.  */
#define N_CUTS 7
static gp_real_t cutoffs[N_CUTS]= { 1800., 720., 360., 180., 90., 30., 10. };

int ga_base60_ticker(gp_real_t lo, gp_real_t hi, gp_real_t nMajor, gp_real_t nMinor,
                gp_real_t *ticks, int nlevel[GA_TICK_LEVELS])
     /* Puts ticks at multiples of 30, failing if lo<-3600 or hi>+3600.
        For ticks at multiples of 10 or less, the subdivisions are
        identical to the default decimal tick scheme.  */
{
  gp_real_t finest= (hi-lo)/nMajor;
  gp_real_t delta= cutoffs[0];
  gp_real_t tick0;
  int i0, ntot, nlevs, phase, base, ndivs;

  if (lo<-3600. || hi>3600. ||
      finest<=cutoffs[N_CUTS-1] || finest>delta) return 1;

  for (i0=0 ; i0<N_CUTS && finest<=cutoffs[i0] ; i0++) delta= cutoffs[i0];
  tick0= ceil(lo/delta)*delta;
  for (ntot=0 ; tick0<=hi ; ntot++,tick0+=delta) ticks[ntot]= tick0;
  nlevel[0]= ntot;
  nlevs= 1;

  finest= (hi-lo)/nMinor;

  /* perform base 60 part of subdivision */
  for (; i0<N_CUTS && finest<=cutoffs[i0] ; i0++) {
    if (i0==1) {
      i0++;                 /* skip 720 starting from 1800 */
      if (finest>cutoffs[i0]) break;
      ndivs= 5;
    } else {
      ndivs= i0<5? 2 : 3;   /* 90->30 and 30->10 are 3, others all 2 */
    }
    delta= cutoffs[i0];
    tick0= ceil(lo/delta);
    phase= (int)(tick0-ceil(tick0/ndivs-.00001)*ndivs);
    tick0*= delta;
    for ( ; tick0<=hi ; tick0+=delta,phase=(phase+1)%ndivs)
      if (phase) ticks[ntot++]= tick0;
    nlevel[nlevs]= ntot;
    if (++nlevs>=GA_TICK_LEVELS) return 0;
  }
  if (i0<N_CUTS || finest>5.) return 0;

  /* perform base 10 part of subdivision if necessary */
  delta= 5.;
  base= 5;
  ndivs= 2;
  for (; nlevs<GA_TICK_LEVELS ; nlevs++) {
    tick0= ceil(lo/delta);
    phase= (int)(tick0-ceil(tick0/ndivs-.00001)*ndivs);
    tick0*= delta;
    for ( ; tick0<=hi ; tick0+=delta,phase=(phase+1)%ndivs)
      if (phase) ticks[ntot++]= tick0;
    nlevel[nlevs]= ntot;
    if (base==2) break;
    if (base==5) {
      delta*= 0.2;
      base= 1;
      ndivs= 5;
    } else if (finest<=0.1*delta) {
      delta*= 0.5;
      base= 5;
      ndivs= 2;
      continue;
    } else {
      delta*= 0.2;
      base= 2;
      ndivs= 5;
    }
    if (finest>delta) break;
  }
  return 0;
}

int ga_degree_labeler(char *label, gp_real_t value)
     /* Prints (value+180)%360-180 instead of just value.  */
{
  gp_real_t dv;
  int val;
  if (value<-3600. || value>3600.) return 1;
  dv= ceil(value-0.00001);
  val= (int)dv;
  if (dv<value) dv= value-dv;
  else dv= dv-value;
  if (dv>0.00001) return 1;
  if (!label) return 0;
  val= (val+180)%360;
  if (val<=0) val+= 360;
  sprintf(label, "%d", val-180);
  return 0;
}

int ga_hour_labeler(char *label, gp_real_t value)
     /* Prints hh:mm (or mm:ss) for value 60*hh+mm (or 60*mm+ss).  */
{
  gp_real_t dv;
  int hh, mm, neg;
  if (value<-3600. || value>3600.) return 1;
  dv= ceil(value-0.00001);
  neg= dv<0.;
  if (neg) hh= (int)(-dv);
  else hh= (int)dv;
  if (dv<value) dv= value-dv;
  else dv= dv-value;
  if (dv>0.00001) return 1;
  if (!label) return 0;
  mm= hh%60;
  hh/= 60;
  sprintf(label, "%s%02d:%02d", (neg?"-":""), hh, mm);
  return 0;
}
