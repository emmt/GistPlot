/*
 * $Id: ugetc.h,v 1.1 2005-09-18 22:05:40 dhmunro Exp $
 * play interface for non-event-driven programs
 * -- incompatible with pl_stdinit/pl_stdout functions
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

/* assumes <stdio.h> has been included */

#include <play/extern.h>
PL_BEGIN_EXTERN_C

/* non-play main programs may be able to use these as drop-in
 *   replacements for getc and/or fgets in order to use gist
 * - in particular, _pl_u_getc can be the readline rl_getc_function
 * - _pl_u_waitfor and u_wait_stdin return 0 if input available
 * - such programs must also define _pl_u_abort_hook as appropriate
 * - the code may call u_pending_events() to handle all pending
 *   events and expired alarms without blocking */
PL_EXTERN int _pl_u_getc(FILE *stream);
PL_EXTERN char *_pl_u_fgets(char *s, int size, FILE *stream);
PL_EXTERN int _pl_u_waitfor(FILE *stream);

/* call pl_pending_events() to handle all pending
 *   events and expired alarms without blocking
 * call pl_wait_while to set a flag and wait for play graphics events
 *   until flag is reset to zero
 * call pl_xhandler to set abort_hook and on_exception routines
 *   a less obtrusive alternative to pl_handler */
PL_EXTERN void pl_pending_events(void);
PL_EXTERN void pl_wait_while(int *flag);
PL_EXTERN void pl_xhandler(void (*abort_hook)(void),
                         void (*on_exception)(int signal, char *errmsg));
PL_EXTERN int pl_wait_stdin(void);

PL_END_EXTERN_C
