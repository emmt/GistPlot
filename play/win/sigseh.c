/*
 * $Id: sigseh.c,v 1.1 2005-09-18 22:05:38 dhmunro Exp $
 * signal handing using Microsoft structured exception handling
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"
#include "play/std.h"

static int w_catch(int code);
static int w_quitting = 0;
static int w_catch_count = 0;

int
_pl_w_protect(int (*run)(void))
{
  int result = 0;
  do {
    __try {
      if (pl_signalling != PL_SIG_NONE) _pl_w_caught(), w_catch_count = 0;
      result = run();
      w_quitting = 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      pl_signalling = w_catch(GetExceptionCode());
      if (w_catch_count++ > 6) w_quitting = 1;
    }
  } while (!w_quitting);
  return result;
}

void (*_pl_w_abort_hook)(void) = 0;

void
pl_abort(void)
{
  if (pl_signalling == PL_SIG_NONE) pl_signalling = PL_SIG_SOFT;
  if (_pl_w_abort_hook) _pl_w_abort_hook();
  RaiseException(0, 0, 0,0);  /* exception code 0 not used by Windows */
}

void
_pl_w_siginit(void)
{
}

static int
w_catch(int code)
{
  if (!code && pl_signalling != PL_SIG_NONE)      code = pl_signalling;
  else if (!code && pl_signalling == PL_SIG_NONE) code = PL_SIG_SOFT;
  else if (code==EXCEPTION_FLT_DIVIDE_BY_ZERO ||
           code==EXCEPTION_FLT_OVERFLOW ||
           code==EXCEPTION_FLT_INVALID_OPERATION ||
           code==EXCEPTION_INT_DIVIDE_BY_ZERO ||
           code==EXCEPTION_INT_OVERFLOW ||
           code==EXCEPTION_FLT_UNDERFLOW ||
           code==EXCEPTION_FLT_INEXACT_RESULT ||
           code==EXCEPTION_FLT_DENORMAL_OPERAND ||
           code==EXCEPTION_FLT_STACK_CHECK ||
           code==STATUS_FLOAT_MULTIPLE_FAULTS ||
           code==STATUS_FLOAT_MULTIPLE_TRAPS)     code = PL_SIG_FPE;
  else if (code==EXCEPTION_ACCESS_VIOLATION ||
           code==EXCEPTION_IN_PAGE_ERROR ||
           code==EXCEPTION_ARRAY_BOUNDS_EXCEEDED) code = PL_SIG_SEGV;
  else if (code==EXCEPTION_ILLEGAL_INSTRUCTION ||
           code==EXCEPTION_PRIV_INSTRUCTION ||
           code==EXCEPTION_BREAKPOINT ||
           code==EXCEPTION_SINGLE_STEP)           code = PL_SIG_ILL;
  else if (code==EXCEPTION_DATATYPE_MISALIGNMENT) code = PL_SIG_BUS;
  else if (code==EXCEPTION_NONCONTINUABLE_EXCEPTION ||
           code==EXCEPTION_INVALID_DISPOSITION ||
           code==EXCEPTION_STACK_OVERFLOW)        code = PL_SIG_OTHER;
  else                                            code = PL_SIG_OTHER;

  if (code == PL_SIG_FPE) {
    _pl_w_fpu_setup();
  }
  return code;
}
