/*
 * $Id: timeu.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * pl_cpu_secs for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "play2.h"

/* functions in kernel32.lib */
#include <windows.h>

/* note: Windows NT provides a GetProcessTimes function that
 * returns system time as well as user time (also GetCurrentProcess) */

static double pl_cpu_unit = 0.;
double
pl_cpu_secs(double *sys)
{
  LARGE_INTEGER t;
  if (sys) *sys = 0.;
  if (pl_cpu_unit == 0.) {
    if (QueryPerformanceFrequency(&t))
      pl_cpu_unit = 1./(t.LowPart + 4294967296.*t.HighPart);
    else
      pl_cpu_unit = -1.;
  }
  if (pl_cpu_unit == -1. ||
      !QueryPerformanceCounter(&t))
    return pl_wall_secs();
  return pl_cpu_unit*(t.LowPart + 4294967296.*t.HighPart);
}
