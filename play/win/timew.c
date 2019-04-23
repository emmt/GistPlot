/*
 * $Id: timew.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * pl_wall_secs for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "play2.h"

/* GetTickCount in kernel32.lib */
#include <windows.h>

static int pl_wall_init = 0;

static double pl_wall_0 = 0.0;
static DWORD pl_wall_1 = 0;
double
pl_wall_secs(void)
{
  DWORD wall = GetTickCount();
  if (!pl_wall_init) {
    pl_wall_1 = wall;
    pl_wall_init = 1;
  } else if (wall < pl_wall_1) {
    pl_wall_0 += 1.e-3*((~(DWORD)0) - pl_wall_1);
    pl_wall_0 += 1.e-3;
    pl_wall_1 = 0;
  }
  pl_wall_0 += 1.e-3*(wall-pl_wall_1);
  pl_wall_1 = wall;
  return pl_wall_0;
}
