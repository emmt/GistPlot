/*
 * $Id: feep.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * pl_feep for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"

/* ARGSUSED */
void
pl_feep(pl_win_t *w)
{
  MessageBeep(MB_OK);
}
