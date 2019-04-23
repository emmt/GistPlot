/*
 * $Id: play/events.h,v 1.1 2005-09-18 22:05:31 dhmunro Exp $
 * minimally intrusive play event handling (no stdio)
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _PLAY_EVENTS_H
#define _PLAY_EVENTS_H 1

#include <play/extern.h>

PL_BEGIN_EXTERN_C

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

#endif /* _PLAY_EVENTS_H */
