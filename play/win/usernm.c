/*
 * $Id: usernm.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * pl_getuser for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "play/std.h"
#include "play2.h"

/* GetUserName in advapi32.lib */
#include <windows.h>

char *
pl_getuser(void)
{
  DWORD sz = PL_WKSIZ;
  if (GetUserName(pl_wkspc.c, &sz))
    return pl_wkspc.c;
  else
    return 0;
}
