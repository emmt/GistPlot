/*
 * $Id: gist/eps.h,v 1.1 2005-09-18 22:04:37 dhmunro Exp $
 * Declare the Encapsulated PostScript pseudo-engine for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_EPS_H
#define _GIST_EPS_H

#include <gist2.h>

extern gp_engine_t *gp_eps_preview(gp_engine_t *engine, char *file);

extern int gp_eps_fm_bug;

#endif
