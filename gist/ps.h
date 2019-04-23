/*
 * $Id: gist/ps.h,v 1.1 2005-09-18 22:04:36 dhmunro Exp $
 * Declare the PostScript engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_PS_H
#define _GIST_PS_H

#include <gist2.h>
#include <gist/engine.h>

#include <stdio.h>

typedef struct _gp_ps_bbox gp_ps_bbox_t;
struct _gp_ps_bbox {
  int xll, yll, xur, yur;
};

typedef struct _gp_ps_engine gp_ps_engine_t;
struct _gp_ps_engine {
  gp_engine_t e;

  /* --------------- Specific to gp_ps_engine_t ------------------- */

  char *filename;
  pl_file_t *file;     /* 0 until file is actually written into */
  int closed;     /* if file==0 and closed!=0, there was a write error */

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

  gp_ps_bbox_t pageBB;   /* bounding box for current page */
  gp_ps_bbox_t docBB;    /* bounding box for entire document */
  int currentPage;  /* current page number, incremented by EndPage */
  long fonts;       /* bits correspond to Gist fonts (set when used) */

  /* The ps.ps GistPrimitives assume knowledge of the several
     graphical state parameters, mostly from ga_attributes.  These are
     reset at the beginning of each page.  */
  gp_box_t clipBox;
  int curClip;
  unsigned long curColor;
  int curType;
  gp_real_t curWidth;
  int curFont;
  gp_real_t curHeight;
  int curAlignH, curAlignV;
  int curOpaque;

  /* When clipping is turned off, the ps.ps GistPrimitives state
     partially reverts to its condition when clipping was turned on.  */
  unsigned long clipColor;
  int clipType;
  gp_real_t clipWidth;
  int clipFont;
  gp_real_t clipHeight;

  char line[80];   /* buffer in which to build current output line */
  int nchars;      /* current number of characters in line */
};

PL_EXTERN gp_ps_engine_t *gp_ps_engine(gp_engine_t *engine);

#endif
