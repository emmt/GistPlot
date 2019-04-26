/*
 * $Id: handler.c,v 1.1 2005-09-18 22:05:39 dhmunro Exp $
 * exception handling for UNIX machines
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#endif
#ifndef _XOPEN_SOURCE
/* new Linux SIGINT broken -- use sysv_signal by defining _XOPEN_SOURCE */
#define _XOPEN_SOURCE 1
#endif

#include "config.h"
#include "play/std.h"
#include "play2.h"
#include "playu.h"

#include <signal.h>

extern void _pl_u_sigfpe(int sig);  /* may be needed by _pl_u_fpu_setup */
static void u_sigint(int sig);
static void u_sigalrm(int sig);
static void u_sigany(int sig);

static int u_sigdbg = 0xffff;

void
pl_handler(void (*on_exception)(int signal, char *errmsg))
{
  _pl_u_exception = on_exception;
  if ((u_sigdbg&1) != 0) {
    signal(SIGFPE, &_pl_u_sigfpe);
  }
  if ((u_sigdbg&2) != 0) {
    signal(SIGINT, &u_sigint);
    signal(SIGALRM, &u_sigalrm);
  }
  if ((u_sigdbg&4) != 0) {
    signal(SIGSEGV, &u_sigany);
  }
#ifdef SIGBUS
  if ((u_sigdbg&8) != 0) {
    signal(SIGBUS, &u_sigany); /* not POSIX */
  }
# define MY_SIGBUS SIGBUS
#else
# define MY_SIGBUS 0
#endif
  if ((u_sigdbg&16) != 0) {
    signal(SIGILL, &u_sigany);
  }
  if ((u_sigdbg&32) != 0) {
    signal(SIGPIPE, &u_sigany);  /* not ANSI C */
  }
}

static int sig_table[] = {
  0, 0, SIGINT, SIGFPE, SIGSEGV, SIGILL, MY_SIGBUS, SIGPIPE };

static void
u_sigany(int sig)
{
  int i;
  for (i=1 ; i<PL_SIG_OTHER ; i++) if (sig==sig_table[i]) break;
  pl_signalling = i;
  signal(sig, &u_sigany);
  pl_abort();
}

void
_pl_u_sigfpe(int sig)
{
  pl_signalling = PL_SIG_FPE;
  _pl_u_fpu_setup(1);
  signal(SIGFPE, &_pl_u_sigfpe);
  pl_abort();
}

static void
u_sigint(int sig)
{
  if (pl_signalling == PL_SIG_NONE) {
    /* not all systems have ualarm function for microsecond resolution */
    extern unsigned int alarm(unsigned int);       /* POSIX <unistd.h> */
    pl_signalling = PL_SIG_INT;
    alarm(1);                        /* about 1/2 sec would be better? */
  }
  signal(SIGINT, &u_sigint);
}

static void
u_sigalrm(int sig)
{
  signal(sig, &u_sigalrm);
  if (pl_signalling == PL_SIG_INT) {
    pl_abort();
  }
}

#ifdef NO_XLIB
/* FIXME this is a hack - not quite correct because of prepolling... */
void pl_qclear(void) {}
#endif
