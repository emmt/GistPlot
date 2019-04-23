/*
 * $Id: play/extern.h,v 1.1 2005-09-18 22:05:31 dhmunro Exp $
 * provide extern attribute for MSWindows DLLs
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _PLAY_EXTERN_H
#define _PLAY_EXTERN_H 1

# if !defined(_WIN32) && !defined(__CYGWIN__)
/* non-MSWindows platforms have more sophistocated dynamic linkers */
#define PL_EXPORT extern
#define PL_IMPORT extern
# else
#  ifdef __GNUC__
#define PL_EXPORT extern __attribute__((dllexport))
#define PL_IMPORT extern __attribute__((dllimport))
#  else
#define PL_EXPORT extern __declspec(dllexport)
#define PL_IMPORT extern __declspec(dllimport)
#  endif
# endif

# ifndef PL_PLUGIN
/* gist/play source code uses this branch */
#define PL_EXTERN PL_EXPORT
# else
/* gist/play plugins (MSWindows DLLs) use this branch */
#define PL_EXTERN PL_IMPORT
# endif

/* the following alias is for some functions to not be selected by the
   export.sh script */
#define PL__EXTERN PL_EXTERN

/* get this ugliness out of important header files */
#ifndef PL_BEGIN_EXTERN_C
# if defined(__cplusplus) || defined(c_plusplus)
#  define PL_BEGIN_EXTERN_C extern "C" {
#  define PL_END_EXTERN_C }
# else
#  define PL_BEGIN_EXTERN_C
#  define PL_END_EXTERN_C
# endif
#endif

#endif /* _PLAY_EXTERN_H */
