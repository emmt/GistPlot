/*
 * $Id: umain.c,v 1.1 2005-09-18 22:05:40 dhmunro Exp $
 * UNIX objects referenced by main.c that goes with play model
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/std.h"
#include "play2.h"
#include "playu.h"

#include <setjmp.h>

typedef enum {
  false = 0,
  true = 1
} bool;

void (*_pl_u_abort_hook)(void) = NULL;
void (*_pl_u_exception)(int, char *) = NULL;
char *_pl_u_errmsg = NULL;
volatile int pl_signalling = PL_SIG_NONE;

static int (*quitter)(void) = NULL;
static bool quitting = false;
static bool launched = false;
static jmp_buf mainloop;
static int fault_loop = 0;

int
_pl_u_main_loop(int (*on_launch)(int,char**), int argc, char **argv)
{
  _pl_u_fpu_setup(-1);
  pl_fpehandling(2);
  if (setjmp(mainloop)) {
    _pl_u_fpu_setup(0);
  }
  if (!quitting && !launched) {
    int result;
    if (argc > 0) {
      argv[0] = pl_strcpy(_pl_u_track_link(_pl_u_find_exe(argv[0])));
    }
    launched = true;
    result = on_launch(argc, argv);
    if (result != 0) {
      return result;
    }
  }
  while (!quitting) {
    _pl_u_waiter(1);
  }
  pl_signalling = PL_SIG_NONE; /* ignore signals after quitting flag is set */
  return quitter != NULL ? quitter() : 0;
}

void
pl_abort(void)
{
  if (pl_signalling == PL_SIG_NONE) {
    pl_signalling = PL_SIG_SOFT;
  }
  if (_pl_u_abort_hook != NULL) {
    _pl_u_abort_hook();
  }
  longjmp(mainloop, 1);
}

void
pl_quitter(int (*on_quit)(void))
{
  quitter = on_quit;
}

void
pl_quit(void)
{
  quitting = true;
}

int
_pl_u_waiter(int wait)
{
  int serviced_event = 0;

  if (pl_signalling != PL_SIG_NONE) {
    /* first priority is to catch any pending signals */
    int i = pl_signalling;
    pl_signalling = PL_SIG_NONE;
    if (!fault_loop && _pl_u_exception) {
      fault_loop = 1;    /* don't trust _pl_u_exception not to fault */
      _pl_u_exception(i, _pl_u_errmsg);
      serviced_event = 1;
      fault_loop = 0;
    }
    _pl_u_errmsg = 0;

  } else {
    bool have_timeout = false;
    serviced_event = _pl_u_poll(0);   /* anything pending? */
    if (!serviced_event) {        /* if not, wait for input */
      int timeout;
      double wait_secs = pl_timeout();
      have_timeout = (wait_secs > 0.0);
      if (wait && wait_secs) {
        do { /* int timeout may not handle > 32.767 s at once */
          if (wait_secs < 0.0)          timeout = -1;
          else if (wait_secs < 32.767)  timeout = (int)(1000.*wait_secs);
          else                          timeout = 32767;
          serviced_event = _pl_u_poll(timeout);
          if (pl_signalling != PL_SIG_NONE) return 0;
          if (serviced_event) break;
          wait_secs -= 32.767;
        } while (wait_secs > 0.0);
      }
    }
    if (serviced_event != 0) {
      if (serviced_event == -3) {
        pl_quit();
      }
      pl_on_idle(1);
    } else {
      pl_on_idle(0);
      serviced_event = have_timeout ? 1 : 0;
    }
    fault_loop = 0;
  }
  return serviced_event;
}
