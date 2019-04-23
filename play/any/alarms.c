/*
 * $Id: alarms.c,v 1.1 2005-09-18 22:05:41 dhmunro Exp $
 * alarm event functions, implemented using play interface
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play2.h"
#include "play/std.h"

typedef struct _pl_alarm pl_alarm_t;
struct _pl_alarm {
  pl_alarm_t *next;
  double time;
  void (*on_alarm)(void *c);
  void *context;
};

static pl_alarm_t *alarm_next = 0;
static pl_alarm_t *alarm_free = 0;
static double alarm_query(void);
static int idle_eligible = 1;

static int pl_dflt_idle(void);
static int (*pl_app_idle)(void)= &pl_dflt_idle;

void
pl_idler(int (*on_idle)(void))
{
  pl_app_idle = on_idle;
}

static int
pl_dflt_idle(void)
{
  return 0;
}

void
pl_on_idle(int reset)
{
  if (!reset) {
    if (alarm_next && !alarm_query()) {
      /* alarm has rung - unlink it and call its on_alarm */
      pl_alarm_t *next = alarm_next;
      alarm_next = next->next;
      next->next = alarm_free;
      alarm_free = next;
      next->on_alarm(next->context);
      idle_eligible = 1;
    } else {
      idle_eligible = pl_app_idle();
    }
  } else {
    idle_eligible = 1;
  }
}

double
pl_timeout(void)
{
  int eligible = idle_eligible;
  idle_eligible = 1;
  return eligible? 0.0 : (alarm_next? alarm_query() : -1.0);
}

void
pl_set_alarm(double secs, void (*on_alarm)(void *c), void *context)
{
  pl_alarm_t *me;
  pl_alarm_t *next = alarm_next;
  pl_alarm_t **prev = &alarm_next;
  double time;
  if (!alarm_free) {
    int n = 8;
    alarm_free = pl_malloc(sizeof(pl_alarm_t)*n);
    alarm_free[--n].next = 0;
    while (n--) alarm_free[n].next = &alarm_free[n+1];
  }
  me = alarm_free;
  me->time = time = pl_wall_secs() + secs;
  me->on_alarm = on_alarm;
  me->context = context;
  /* insert me into alarm_next list, kept in order of time */
  while (next && next->time<=time) {
    prev = &next->next;
    next = next->next;
  }
  alarm_free = alarm_free->next;
  me->next = next;
  *prev = me;
}

void
pl_clr_alarm(void (*on_alarm)(void *c), void *context)
{
  pl_alarm_t *next, **prev = &alarm_next;
  for (next=alarm_next ; next ; next=*prev) {
    if ((!on_alarm || on_alarm==next->on_alarm) &&
        (!context || context==next->context)) {
      *prev = next->next;
      next->next = alarm_free;
      alarm_free = next;
    } else {
      prev = &next->next;
    }
  }
}

static double
alarm_query(void)
{
  if (alarm_next->time != -1.e35) {
    double time = pl_wall_secs();
    pl_alarm_t *next = alarm_next;
    /* if no alarms need to ring yet, return earliest */
    if (next->time > time)
      return next->time - time;
    do {
      next->time = -1.e35;   /* mark all alarms that need to ring */
      next = next->next;
    } while (next && next->time<=time);
  }
  return 0.0;
}
