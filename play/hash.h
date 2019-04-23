/*
 * $Id: play/hash.h,v 1.1 2005-09-18 22:05:31 dhmunro Exp $
 * portability layer unique key hashing functions
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _PLAY_HASH_H
#define _PLAY_HASH_H 1

#include <play/extern.h>

typedef struct _pl_hashtab pl_hashtab_t;
typedef unsigned long pl_hashkey_t;

/* randomize the low order 32 bits of an address or integer
 *   such that PL_IHASH(x)==PL_IHASH(y) if and only if x==y */
#define PL_IHASH(x) ((x)^pl_hmasks[(((pl_hashkey_t)(x))>>4)&0x3f])
#define PL_PHASH(x) PL_IHASH((char*)(x)-(char*)0)

PL_BEGIN_EXTERN_C

PL_EXTERN pl_hashkey_t pl_hmasks[64];  /* for PL_IHASH, PL_PHASH macros */

/* unique key hash tables are basis for all hashing */
PL_EXTERN pl_hashtab_t *pl_halloc(pl_hashkey_t size);
PL_EXTERN void pl_hfree(pl_hashtab_t *tab, void (*func)(void *));
PL_EXTERN int pl_hinsert(pl_hashtab_t *tab, pl_hashkey_t hkey, void *value);
PL_EXTERN void *pl_hfind(pl_hashtab_t *tab, pl_hashkey_t hkey);
PL_EXTERN void pl_hiter(pl_hashtab_t *tab,
                    void (*func)(void *val, pl_hashkey_t key, void *ctx),
                    void *ctx);

/* global name string to id number correspondence
 *   pl_id returns id number of an existing string, or 0
 *   pl_idmake returns id number valid until matching pl_idfree, never 0
 *   pl_idstatic returns id number for statically allocated input name
 *     - name not copied, subsequent calls to pl_idfree will be ignored
 *   pl_idfree decrements the use counter for the given id number,
 *     freeing the number if there are no more uses
 *     - pl_idmake increments use counter if name already exists
 *   pl_idname returns 0 if no such id number has been made */
PL_EXTERN pl_hashkey_t pl_id(const char *name, int len);
PL_EXTERN pl_hashkey_t pl_idmake(const char *name, int len);
PL_EXTERN pl_hashkey_t pl_idstatic(char *name);
PL_EXTERN void pl_idfree(pl_hashkey_t id);
PL_EXTERN char *pl_idname(pl_hashkey_t id);

/* global pointer-to-pointer correspondence
 *   pl_getctx returns context of a pointer or 0
 *   pl_setctx sets context of a pointer, or deletes it if 0 */
PL_EXTERN void pl_setctx(void *ptr, void *context);
PL_EXTERN void *pl_getctx(void *ptr);

PL_END_EXTERN_C

#endif /* _PLAY_HASH_H */
