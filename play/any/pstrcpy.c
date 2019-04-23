/*
 * $Id: pstrcpy.c,v 1.1 2005-09-18 22:05:43 dhmunro Exp $
 * strcpy that pl_mallocs its destination
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/std.h"
#include <string.h>

char *
pl_strcpy(const char *s)
{
  if (s) {
    char *d = pl_malloc(strlen(s)+1);
    strcpy(d, s);
    return d;
  } else {
    return 0;
  }
}
