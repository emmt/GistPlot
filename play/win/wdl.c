/*
 * $Id: wdl.c,v 1.1 2005-09-18 22:05:38 dhmunro Exp $
 * MS Windows version of play dynamic linking operations
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"

#include "play/std.h"
#include "playw.h"
#include <string.h>

#ifdef PLUG_HEADER
#include PLUG_HEADER
#endif

#ifndef PLUG_DISABLE

#ifndef PLUG_SUFFIX
# define PLUG_SUFFIX ".dll"
#endif

void *
pl_dlopen(const char *dlname)
{
  void *handle = 0;
  if (dlname && dlname[0]) {
    char *name = pl_strncat(_pl_w_pathname(dlname), PLUG_SUFFIX, 0);
    handle = LoadLibrary(name);
    pl_free(name);
  }
  return handle;
}

int
pl_dlsym(void *handle, const char *symbol, int type, void *paddr)
{
  void **addr = paddr;
  addr[0] = GetProcAddress(handle, symbol);
  return !addr[0];
}



#else

/* ARGSUSED */
void *
pl_dlopen(const char *dlname)
{
  return 0;
}

/* ARGSUSED */
int
pl_dlsym(void *handle, const char *symbol, int type, void *paddr)
{
  void **addr = paddr;
  addr[0] = 0;
  return 1;
}

#endif
