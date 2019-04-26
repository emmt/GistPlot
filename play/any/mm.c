/*
 * mm.c -
 *
 * Load default function pointers for <play/std.h> interface.
 * pl_mminit and pl_mmdebug override these defaults.
 * Purpose of this file is so if pl_mminit or pl_mmdebug never called,
 * code should run with system memory manager without loading bmm.
 *
 *------------------------------------------------------------------------------
 *
 * Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/std.h"

static void *default_malloc(size_t);
static void  default_free(void *);
static void *default_realloc(void *, size_t);
static void *default_mmfail(unsigned long n);

void *(*pl_malloc)(size_t)          = &default_malloc;
void  (*pl_free)(void *)            = &default_free;
void *(*pl_realloc)(void *, size_t) = &default_realloc;
void *(*pl_mmfail)(unsigned long n) = &default_mmfail;

long pl_nallocs = 0;
long pl_nfrees = 0;
long pl_nsmall = 0;
long pl_asmall = 0;

pl_wkspc_t pl_wkspc;

static void*
default_malloc(size_t n)
{
  size_t m = (n > 0 ? n : 1);
  void *p = malloc(m);
  if (p == NULL) return pl_mmfail(m);
  ++pl_nallocs;
  return p;
}

static void
default_free(void *p)
{
  if (p != NULL) {
    ++pl_nfrees;
    free(p);
  }
}

static void*
default_realloc(void *p, size_t n)
{
  size_t m = (n > 0 ? n : 1);
  p = (p != NULL ? realloc(p,m) : malloc(m));
  return (p != NULL ? p : pl_mmfail(n));
}

/* ARGSUSED */
static void*
default_mmfail(unsigned long n)
{
  return NULL;
}
