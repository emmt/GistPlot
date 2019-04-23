/*
 * $Id: gist/cgmin.h,v 1.1 2005-09-18 22:04:36 dhmunro Exp $
 * Declare the CGM reader/echoer for GIST.
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _GIST_CGMIN_H
#define _GIST_CGMIN_H

#include <gist2.h>

extern int gp_open_cgm(char *file);
extern int gp_read_cgm(int *mPage, int *nPage, int *sPage, int nPageGroups);
extern int gp_cgm_relative(int offset);
extern void gp_cgm_info(void);
extern int gp_catalog_cgm(void);

extern void gp_cgm_warning(char *general, char *particular);
extern int gp_cgm_is_batch, gp_cgm_landscape;
extern gp_engine_t *gp_cgm_out_engines[8];
extern int gp_cgm_out_types[8];

extern int gp_cgm_bg0fg1;

#endif
