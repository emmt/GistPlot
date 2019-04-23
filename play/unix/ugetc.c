/*
 * $Id: ugetc.c,v 1.1 2005-09-18 22:05:40 dhmunro Exp $
 * play interface for non-event-driven programs
 * -- incompatible with stdinit.c functions
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _POSIX_SOURCE
/* to get fileno declared */
#define _POSIX_SOURCE 1
#endif

#include "config.h"

#include "playu.h"
#include <stdio.h>
#include "ugetc.h"
#include "play/events.h"

static void u_fd0_ready(void *c);
static void u_nowait(void);

static FILE *u_stream = 0;
static FILE *u_fd0_init = 0;
/* ARGSUSED */
static
void u_fd0_ready(void *c)
{
  u_stream = c;
}

int
_pl_u_getc(FILE *stream)
{
  _pl_u_waitfor(stream);
  return getc(stream);
}

char *
_pl_u_fgets(char *s, int size, FILE *stream)
{
  _pl_u_waitfor(stream);
  return fgets(s, size, stream);
}

int
_pl_u_waitfor(FILE *stream)
{
  if (stream != u_fd0_init) {
    u_nowait();
    _pl_u_event_src(fileno(stream), &u_fd0_ready, stream);
    u_fd0_init = stream;
  }

  u_stream = 0;
  while (!u_stream) _pl_u_waiter(1);
  stream = u_stream;
  u_stream = 0;
  return (stream != u_fd0_init);  /* 0 on success */
}

static void
u_nowait(void)
{
  if (u_fd0_init) {
    _pl_u_event_src(fileno(u_fd0_init), (void (*)(void *c))0, u_fd0_init);
    u_fd0_init = 0;
  }
}

int
pl_wait_stdin(void)
{
  return _pl_u_waitfor(stdin);
}

void
pl_pending_events(void)
{
  /* do not handle events on u_fd0_init -- just everything else */
  u_nowait();
  while (_pl_u_waiter(0));
}

void
pl_wait_while(int *flag)
{
  pl_pending_events();
  while (*flag) _pl_u_waiter(1);
}

void
pl_xhandler(void (*abort_hook)(void),
           void (*on_exception)(int signal, char *errmsg))
{
  u_abort_hook = abort_hook;   /* replaces pl_abort */
  u_exception = on_exception;  /* when _pl_u_waiter detects pl_signalling */
}
