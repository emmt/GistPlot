/*
 * $Id: gist/cgm.h,v 1.1 2005-09-18 22:04:24 dhmunro Exp $
 * Declare the CGM binary metafile engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_CGM_H
#define _GIST_CGM_H

#include <gist2.h>
#include <gist/engine.h>

#include <stdio.h>
#ifndef SEEK_CUR
/* Sun strikes again...  This may break other non-standard machines */
#define SEEK_CUR 1
#endif

/* This gp_engine_t is based on the ANSI CGM Standard, ANSI X3.122 - 1986
   Parts 1 and 3.  It is fully conforming with the following caveats:
   1. The font names in the FONT LIST metafile descriptor element are
      standard PostScript font names, rather than ISO standard names
      (which I couldn't find).  The font numbers match the numbering
      scheme used by the GPLOT program.
   2. A Gist smooth polyline is output using the ordinary POLYLINE
      primitive, preceded by an APPLICATION DATA comment.  This gives
      reasonable output (identical to that produced by the Gist X
      engine, in fact), but not as nice as the Gist PostScript engine.
 */

typedef unsigned char gp_octet_t;  /* as defined in the CGM standard */
#define GP_MAX_CGM_PARTITION 0x7ffc  /* maximum length of a CELL ARRAY partition */

typedef struct _gp_cgm_engine gp_cgm_engine_t;
struct _gp_cgm_engine {
  gp_engine_t e;

  /* --------------- Specific to gp_cgm_engine_t ------------------- */

  char *filename;
  gp_real_t scale;   /* VDC units per Gist NDC unit, 25545.2 by default */
  long fileSize;  /* approximate maximum in bytes, default is 1 Meg */
  void (*IncrementName)(char *filename);
                  /* function to increment filename IN PLACE
                     (a reasonable default is supplied) */
  pl_file_t *file;   /* 0 until file is actually written into */
  int state;      /* CGM state as described in fig. 12 of ANSI X3.122 */

  /* Page orientation and color table can only be changed at the beginning
     of each page, so the entries in the gp_engine_t base class are repeated
     here as the "currently in effect" values.  When a new page begins,
     these values are brought into agreement with those in the base
     class.  The ChangePalette virtual function temporarily resets colorMode
     to 0, to avoid any references to the new palette until the
     next page begins.  */
  int landscape;
  int colorMode;
  int nColors;

  int currentPage;  /* current page number, incremented by EndPage */

  /* The CGM engine must keep track of several graphical state
     parameters, mostly from ga_attributes.  These are reset at the
     beginning of each page.  */
  gp_box_t clipBox;
  int curClip;
  unsigned long curColor[5];
  int curType;
  gp_real_t curWidth;
  int curMark;
  gp_real_t curSize;
  int curFont;
  gp_real_t curHeight;
  int curAlignH, curAlignV;
  int curPath;
  int curOpaque;  /* unlike Gist, this applies to more than text... */
  int curEtype;
  gp_real_t curEwidth;
};

PL_EXTERN gp_cgm_engine_t *gp_cgm_engine(gp_engine_t *engine);

/* Default CGM scale factor and maximum file size */
PL_EXTERN gp_real_t gp_cgm_scale;
PL_EXTERN long gp_cgm_file_size;

/* To change away from the default fileSize, just change the
   gp_cgm_engine_t fileSize member.  To change the CGM scale factor
   (the initial default value is 25545.2, which corresponds to
   2400 dpi resolution), use gp_cgm_set_scale after creating the
   engine but BEFORE DRAWING ANYTHING WITH IT: */
PL_EXTERN void gp_cgm_set_scale(gp_cgm_engine_t *cgmEngine, gp_real_t scale);

#endif
