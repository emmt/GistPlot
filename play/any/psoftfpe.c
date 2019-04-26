/*
 * psoftfpe.c -
 *
 * Raise pending software SIGFPE using fenv.h
 *
 *------------------------------------------------------------------------------
 *
 * Copyright (c) 2015, David H. Munro.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play2.h"

#ifdef HAS_FENV_H

/* fenv.h is C99, signal.h is C89 */
#include <fenv.h>
#include <signal.h>

/* FE_INEXACT and FE_UNDERFLOW are not really exception conditions */
#define TRUE_EXCEPTIONS (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW)

/* code should use PL_SOFTFPE_TEST macro, never call this directly */
void
pl_softfpe(void)
{
  if (fetestexcept(TRUE_EXCEPTIONS)) {
    feclearexcept(TRUE_EXCEPTIONS);
    raise(SIGFPE);
  }
}

void
pl_fpehandling(int on)
{
  static fenv_t fenv;
  static int initialized = 0;
  static int depth = 0;  /* zero means at memorized fenv */

  if (initialized == 0 && fegetenv(&fenv) == 0) {
    initialized = 1;
  }
  if (initialized) {
    if (on == 0) {
      if (depth == 0) {
        fesetenv(FE_DFL_ENV);
      }
      ++depth;
    } else {
      if (on != 1) {
        depth = 1;
      }
      if (depth == 1) {
        fesetenv(&fenv);
      }
      --depth;
    }
  }
}

#else

/* never invoked if code uses PL_SOFTFPE_TEST macro as intended */
void
pl_softfpe(void)
{
}

void
pl_fpehandling(int on)
{
}

#endif
