/*
 * $Id: gist/text.h,v 1.1 2005-09-18 22:04:33 dhmunro Exp $
 * Declare GIST text utilities
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_TEXT_H
#define _GIST_TEXT_H

#include <gist2.h>

/* Return t->alignH, t->alignV, guaranteed not GP_HORIZ_ALIGN_NORMAL or GP_VERT_ALIGN_NORMAL */
PL_EXTERN void gp_get_text_alignment(const gp_text_attribs_t *t,
                             int *alignH, int *alignV);

/* Get shape of text input to gd_add_text, given a function Width which can
   compute the width of a simple text string (no imbedded \n).  Returns
   largest value of Width for any line, and a line count.
   If gp_do_escapes==1 (default), and Width!=0, the Width function must
   handle the !, ^, and _ escape sequences.
   If Width==0, the default Width function is the number of
   non-escaped characters.  */
typedef gp_real_t (*gp_text_width_computer_t)(const char *text, int nChars,
                                const gp_text_attribs_t *t);
PL_EXTERN int gp_get_text_shape(const char *text, const gp_text_attribs_t *t,
                         gp_text_width_computer_t Width, gp_real_t *widest);

/* Return the next line of text-- if text[0] is not '\n' and path not
   T_UP or T_DOWN, returns text and a count of the characters in the
   line, nChars (always 1 if T_UP or T_DOWN).  If text is '\0', or '\n'
   with path T_UP or T_DOWN, returns 0.  Otherwise, returns text+1 and
   a count of the number of characters to the next '\n' or '\0'.  */
PL_EXTERN const char *gp_next_text_line(const char *text, int *nChars, int path);

/* Use ^ (superscript), _ (subscript), and ! (symbol) escape sequences
   in gp_draw_text.  These are on (gp_do_escapes==1) by default.  */
PL_EXTERN int gp_do_escapes;

#endif
