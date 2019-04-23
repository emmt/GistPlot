/*
 * $Id: bitmrot.c,v 1.1 2005-09-18 22:05:44 dhmunro Exp $
 * routines to rotate bitmaps, most significant bit first version
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play2.h"

void
pl_mrot180(unsigned char *from, unsigned char *to, int fcols, int frows)
{
  int fbpl = (((unsigned int)(fcols-1)) >> 3) + 1;
  int shft = (fbpl<<3) - fcols;
  int i;
  if (frows<0) return;
  for (to+=frows*fbpl-fbpl,from+=fbpl-1 ; frows-- ; to-=fbpl,from+=fbpl) {
    /* first flip each row and each byte "in place"... */
    for (i=0 ; i<fbpl ; i++) to[i] = pl_bit_rev[from[-i]];
    if (!shft) continue;
    /* ...then shift bytes left pairwise to final position */
    for (i=0 ; i<fbpl-1 ; i++) to[i] = (to[i]<<shft)|(to[i+1]>>(8-shft));
    to[i]<<= shft;
  }
}

void
pl_mrot090(unsigned char *from, unsigned char *to, int fcols, int frows)
{
  int fbpl = (((unsigned int)(fcols-1)) >> 3) + 1;
  int shft = (fbpl<<3) - fcols;
  int tbpl = (((unsigned int)(frows-1)) >> 3) + 1;
  int j, ibyt, jbyt;
  unsigned char ibit, jbit;
  if (fcols<0) return;
  frows *= fbpl;
  shft = 1<<shft;
  for (ibyt=fbpl-1,ibit=shft ; fcols-- ; ibit<<=1,to+=tbpl) {
    if (!ibit) {
      ibit = 1;
      ibyt--;
    }
    for (j=0 ; j<tbpl ; j++) to[j] = 0;
    for (j=jbyt=0,jbit=128 ; j<frows ; j+=fbpl,jbit>>=1) {
      if (!jbit) {
        jbit = 128;
        jbyt++;
      }
      if (from[j+ibyt]&ibit) to[jbyt] |= jbit;
    }
  }
}

void
pl_mrot270(unsigned char *from, unsigned char *to, int fcols, int frows)
{
  int fbpl = (((unsigned int)(fcols-1)) >> 3) + 1;
  int tbpl = (((unsigned int)(frows-1)) >> 3) + 1;
  int shft = (tbpl<<3) - frows;
  int j, ibyt, jbyt;
  unsigned char ibit, jbit;
  if (fcols<0) return;
  frows *= fbpl;
  shft = 1<<shft;
  for (ibyt=0,ibit=128 ; fcols-- ; ibit>>=1,to+=tbpl) {
    if (!ibit) {
      ibit = 128;
      ibyt++;
    }
    for (j=0 ; j<tbpl ; j++) to[j] = 0;
    for (j=0,jbyt=tbpl-1,jbit=shft ; j<frows ; j+=fbpl,jbit<<=1) {
      if (!jbit) {
        jbit = 1;
        jbyt--;
      }
      if (from[j+ibyt]&ibit) to[jbyt] |= jbit;
    }
  }
}
