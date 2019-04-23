/*
 * $Id: hashctx.c,v 1.1 2005-09-18 22:05:44 dhmunro Exp $
 * generic pointer<->context association functions
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/hash.h"

static pl_hashtab_t *ctx_table = 0;

void
pl_setctx(void *ptr, void *context)
{
  if (!ctx_table) ctx_table = pl_halloc(64);
  pl_hinsert(ctx_table, PL_PHASH(ptr), context);
}

void *
pl_getctx(void *ptr)
{
  return ctx_table? pl_hfind(ctx_table, PL_PHASH(ptr)) : 0;
}
