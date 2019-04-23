/* psoftfpe.c
 * raise pending software SIGFPE using fenv.h
 */
/* Copyright (c) 2015, David H. Munro.
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
#define PL_TRUE_EXCEPTIONS FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW

/* code should use PL_SOFTFPE_TEST macro, never call this directly */
void
pl_softfpe(void)
{
  if (fetestexcept(PL_TRUE_EXCEPTIONS)) {
    feclearexcept(PL_TRUE_EXCEPTIONS);
    raise(SIGFPE);
  }
}

static fenv_t pl_fenv;
static int pl_valid_fenv = 0;
static int pl_depth_fenv = 0;  /* zero means at pl_fenv */

void
pl_fpehandling(int on)
{
  pl_valid_fenv = (pl_valid_fenv || !fegetenv(&pl_fenv));
  if (pl_valid_fenv) {
    if (on) {
      if (on != 1) pl_depth_fenv = 1;
      if (pl_depth_fenv && !--pl_depth_fenv)
        fesetenv(&pl_fenv);
    } else {
      if (!pl_depth_fenv++)
        fesetenv(FE_DFL_ENV);
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
