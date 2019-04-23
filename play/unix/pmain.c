/*
 * $Id: pmain.c,v 1.1 2005-09-18 22:05:40 dhmunro Exp $
 * play main function for UNIX with raw X11 (no X toolkit)
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "play2.h"

PL_BEGIN_EXTERN_C
extern void pl_mminit(void);
extern int _pl_u_main_loop(int (*on_launch)(int,char**), int, char **);
PL_END_EXTERN_C

int
main(int argc, char *argv[])
{
  pl_mminit();
  return _pl_u_main_loop(on_launch, argc, argv);
}
