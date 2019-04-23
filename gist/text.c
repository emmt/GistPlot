/*
 * $Id: text.c,v 1.2 2009-03-27 02:12:17 dhmunro Exp $
 * Define GIST text utilities
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist/text.h"
#include <string.h>

int gp_do_escapes= 1;

/* Return t->alignH, t->alignV, guaranteed not GP_HORIZ_ALIGN_NORMAL or GP_VERT_ALIGN_NORMAL */
void gp_get_text_alignment(const gp_text_attribs_t *t, int *alignH, int *alignV)
{
  *alignH= t->alignH;
  *alignV= t->alignV;
  if (*alignH==GP_HORIZ_ALIGN_NORMAL) *alignH= GP_HORIZ_ALIGN_LEFT;
  if (*alignV==GP_VERT_ALIGN_NORMAL) *alignV= GP_VERT_ALIGN_BASE;
}

/* Get shape of text input to gd_add_text, given a function Width which can
   compute the width of a simple text string (no imbedded \n).  Returns
   largest value of Width for any line, and a line count.  */
int gp_get_text_shape(const char *text, const gp_text_attribs_t *t,
                gp_text_width_computer_t Width, gp_real_t *widest)
{
  int path= t->orient;
  gp_real_t wdest, thisWid;
  int nLines, nChars;

  /* Count lines in this text, find widest line */
  nLines= 0;
  wdest= 0.0;
  while ((text= gp_next_text_line(text, &nChars, path))) {
    nLines++;
    if (Width) thisWid= Width(text, nChars, t);
    else thisWid= (gp_real_t)nChars;
    if (thisWid>wdest) wdest= thisWid;
    text+= nChars;
  }

  *widest= wdest;
  return nLines;
}

/* Return the next line of text--
   returns text and a count of the characters in the
   line, nChars.  If text is '\0', or '\n', returns text+1 and
   a count of the number of characters to the next '\n' or '\0'.  */
const char *gp_next_text_line(const char *text, int *nChars, int path)
{
  char first= text? text[0] : '\0';

  if (!first) {
    *nChars= 0;
    return 0;
  }

  if (first=='\n') text+= 1;
  *nChars= strcspn(text, "\n");

  return text;
}
