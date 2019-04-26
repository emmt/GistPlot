/*
 * $Id: sigansi.c,v 1.1 2005-09-18 22:05:38 dhmunro Exp $
 * signal handing using POSIX/ANSI signals (GNU/cygwin)
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"
#include "play/std.h"

#include <signal.h>
#include <setjmp.h>

static int w_quitting = 0;

static jmp_buf w_mainloop;

static void sig_any(int sig);
static void sig_fpe(int sig);
static void sig_int(int sig);
static int w_catch_count = 0;

static int w_sigdbg = 0xffff;

int
_pl_w_protect(int (*run)(void))
{
  int result = 0;
  if (setjmp(w_mainloop)) {
    w_catch_count++;
    _pl_w_caught();
    w_catch_count = 0;
  }
  if (!w_quitting && w_catch_count<6)
    result = run();
  w_quitting = 1;
  return result;
}

void (*_pl_w_abort_hook)(void) = 0;

void
pl_abort(void)
{
  if (pl_signalling == PL_SIG_NONE) pl_signalling = PL_SIG_SOFT;
  if (_pl_w_abort_hook) _pl_w_abort_hook();
  /* Microsoft documentation warns that msvcrt.dll
   * may not be POSIX compliant for using longjmp out of an
   * interrupt handling routine -- oh well */
  if (!w_quitting)
    longjmp(w_mainloop, 1);
}

void
_pl_w_siginit(void)
{
  if (w_sigdbg&1) {
    signal(SIGFPE, &sig_fpe);
    _pl_w_fpu_setup();
  }
  /* SIGINT handled by SetConsoleCtrlHandler */
  if (w_sigdbg&4) signal(SIGSEGV, &sig_any);
#ifdef SIGBUS
  if (w_sigdbg&8) signal(SIGBUS, &sig_any);    /* not POSIX */
# define MY_SIGBUS SIGBUS
#else
# define MY_SIGBUS 0
#endif
  if (w_sigdbg&16) signal(SIGILL, &sig_any);
#ifdef SIGBUS
  if (w_sigdbg&32) signal(SIGPIPE, &sig_any);  /* not ANSI C */
# define MY_SIGPIPE SIGPIPE
#else
# define MY_SIGPIPE 0
#endif
}

static int sig_table[] = {
  0, 0, SIGINT, SIGFPE, SIGSEGV, SIGILL, MY_SIGBUS, MY_SIGPIPE };

static void
sig_any(int sig)
{
  int i;
  for (i=1 ; i<PL_SIG_OTHER ; i++) if (sig==sig_table[i]) break;
  pl_signalling = i;
  if (!w_quitting) signal(sig, &sig_any);
  pl_abort();
}

static void
sig_fpe(int sig)
{
  pl_signalling = PL_SIG_FPE;
  if (!w_quitting) signal(SIGFPE, &sig_fpe);
  _pl_w_fpu_setup();
  pl_abort();
}
