/*
 * $Id: play/std.h,v 1.1 2005-09-18 22:05:31 dhmunro Exp $
 * portability layer basic memory management interface
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include <stdlib.h>

#ifndef _PLAY_STD_H
#define _PLAY_STD_H 1

#include <play/extern.h>

PL_BEGIN_EXTERN_C

PL_EXTERN void *(*pl_malloc)(size_t);
PL_EXTERN void  (*pl_free)(void *);
PL_EXTERN void *(*pl_realloc)(void *, size_t);

/* above data loaded to system malloc, free, and realloc
 * -- call pl_mminit to get mm version
 */
#ifdef PL_DEBUG
#define pl_mminit pl_mmdebug
PL__EXTERN int pl_mmcheck(void *p);
PL__EXTERN void pl_mmguard(void *b, unsigned long n);
PL__EXTERN long pl_mmextra, pl_mmoffset;
#endif
PL_EXTERN void pl_mminit(void);

/* make trivial memory statistics globally available
 * -- counts total number of allocations, frees, and
 *    current number of large blocks */
PL_EXTERN long pl_nallocs;
PL_EXTERN long pl_nfrees;
PL_EXTERN long pl_nsmall;
PL_EXTERN long pl_asmall;

/* define this to get control when mm functions fail
 * -- if it returns, must return 0 */
PL_EXTERN void *(*pl_mmfail)(unsigned long n);

/* temporary space */
#define PL_WKSIZ 2048
typedef union {
  char c[PL_WKSIZ+8];
  int i[PL_WKSIZ/8];
  long l[PL_WKSIZ/8];
  double d[PL_WKSIZ/8];
} pl_wkspc_t;
PL_EXTERN pl_wkspc_t pl_wkspc;

/* similar to the string.h functions, but pl_malloc destination
 * - 0 src is acceptable */
PL_EXTERN void *pl_memcpy(const void *, size_t);
PL_EXTERN char *pl_strcpy(const char *);
PL_EXTERN char *pl_strncat(const char *, const char *, size_t);

/* expand leading env var, ~, set / or \ separators, free with pl_free */
PL_EXTERN char *pl_native(const char *unix_name);

/* dont do anything critical if this is set -- signal an error */
PL__EXTERN volatile int pl_signalling;

/* dynamic linking interface
 * dlname is filename not including .so, .dll, .dylib, etc. suffix
 * symbol is name in a C program
 * type is 0 if expecting a function, 1 if expecting data
 * paddr is &addr where addr is void* or void(*)(),
 * pl_dlsym return value is 0 on success, 1 on failure */
PL_EXTERN void *pl_dlopen(const char *dlname);
PL_EXTERN int pl_dlsym(void *handle, const char *symbol, int type, void *paddr);

/* interface for synchronous subprocess
 * (pl_popen, pl_spawn in play/io.h for asynchronous)
 */
PL_EXTERN int pl_system(const char *cmdline);

PL_END_EXTERN_C

#endif /* _PLAY_STD_H */

