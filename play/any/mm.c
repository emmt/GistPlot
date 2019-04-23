/*
 * $Id: mm.c,v 1.1 2005-09-18 22:05:42 dhmunro Exp $
 *
 * load default function pointers for play/std.h interface
 * pl_mminit and pl_mmdebug override these defaults
 * purpose of this file is so if pl_mminit or pl_mmdebug never called,
 *   code should run with system memory manager without loading bmm
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/std.h"

static void *pl__malloc(size_t);
static void  pl__free(void *);
static void *pl__realloc(void *, size_t);

void *(*pl_malloc)(size_t)= &pl__malloc;
void  (*pl_free)(void *)=   &pl__free;
void *(*pl_realloc)(void *, size_t)= &pl__realloc;

static void *pl__mmfail(unsigned long n);
void *(*pl_mmfail)(unsigned long n)= &pl__mmfail;

long pl_nallocs = 0;
long pl_nfrees = 0;
long pl_nsmall = 0;
long pl_asmall = 0;

pl_wkspc_t pl_wkspc;

static void *
pl__malloc(size_t n)
{
  void *p = malloc(n>0? n : 1);
  if (!p) return pl_mmfail(n>0? n : 1);
  pl_nallocs++;
  return p;
}

static void
pl__free(void *p)
{
  if (p) {
    pl_nfrees++;
    free(p);
  }
}

static void *
pl__realloc(void *p, size_t n)
{
  if (n<=0) n = 1;
  p = p? realloc(p,n) : malloc(n);
  return p? p : pl_mmfail(n);
}

/* ARGSUSED */
static void *
pl__mmfail(unsigned long n)
{
  return 0;
}
