/*
 * $Id: hashid.c,v 1.1 2005-09-18 22:05:42 dhmunro Exp $
 * global name string to id number correspondence
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/hash.h"
#include "play/std.h"

#include <string.h>

typedef struct id_name id_name;
struct id_name {
  union {
    char *ame;
    id_name *ext;
  } n;
  /* uses >= 0 indicates number of additional uses, ordinary case
   * uses ==-1 indicates idstatic, n.ame will never be freed
   * uses <=-2 indicates locked id_name struct, but
   *           n.ame can still be freed (true uses = -2-uses) */
  long uses;
};

static pl_hashtab_t *id_table = 0;
static id_name *id_freelist = 0;
static id_name id_null;
extern int pl_id_collisions;
int pl_id_collisions = 0;  /* expect to be tiny */

static pl_hashkey_t id_hash(const char *name, int len);
static id_name *id_block(void);
static int idnm_compare(const char *n1, const char *n2, int len);

pl_hashkey_t
pl_id(const char *name, int len)
{
  pl_hashkey_t h = id_hash(name, len);
  if (id_table) {
    id_name *idnm;
    pl_hashkey_t rehash = h&0xfff;

    for (;;) {
      if (!h) h = 1;
      idnm = pl_hfind(id_table, h);
      if (!idnm || !idnm->n.ame) return 0;
      if (!idnm_compare(name, idnm->n.ame, len)) return h;
      if (!rehash) rehash = 3691;
      h += rehash;
    }
  }
  return 0;
}

pl_hashkey_t
pl_idmake(const char *name, int len)
{
  id_name *idnm = 0;
  pl_hashkey_t h = id_hash(name, len);
  pl_hashkey_t rehash = h&0xfff;

  if (!id_table) {
    id_table = pl_halloc(64);
    id_null.n.ame = 0;
    id_null.uses = -1;
    pl_hinsert(id_table, 0, &id_null);
  }

  for (;;) {
    if (!h) h = 1;
    idnm = pl_hfind(id_table, h);
    if (!idnm || !idnm->n.ame) break;
    if (!idnm_compare(name, idnm->n.ame, len)) {
      if (idnm->uses>=0) idnm->uses++;
      else if (idnm->uses<-1) idnm->uses--;
      return h;
    }
    if (!rehash) rehash = 3691;
    h += rehash;
    /* a collision locks the hashkey and id_name struct forever */
    if (idnm->uses>=0) {
      idnm->uses = -2-idnm->uses;
      pl_id_collisions++;
    }
  }

  if (!idnm) {
    idnm = id_freelist;
    if (!idnm) idnm = id_block();
    idnm->uses = 0;
    id_freelist = idnm->n.ext;
    idnm->n.ame = len>0? pl_strncat(0, name, len) : pl_strcpy(name);
    pl_hinsert(id_table, h, idnm);
  } else {
    idnm->n.ame = len>0? pl_strncat(0, name, len) : pl_strcpy(name);
  }

  return h;
}

static int
idnm_compare(const char *n1, const char *n2, int len)
{
  if (len) return strncmp(n1, n2, len) || n2[len];
  else     return strcmp(n1, n2);
}

pl_hashkey_t
pl_idstatic(char *name)
{
  pl_hashkey_t id = pl_idmake(name, 0);
  id_name *idnm = pl_hfind(id_table, id);  /* never 0 or idnm->n.ame==0 */
  if (idnm->uses>=0) {
    char *nm = idnm->n.ame;
    idnm->uses = -1;
    idnm->n.ame = name;
    pl_free(nm);
  }
  return id;
}

void
pl_idfree(pl_hashkey_t id)
{
  if (id_table) {
    id_name *idnm = pl_hfind(id_table, id);
    if (idnm && idnm->n.ame) {
      if (!idnm->uses) {
        char *name = idnm->n.ame;
        pl_hinsert(id_table, id, (void *)0);
        idnm->n.ext = id_freelist;
        id_freelist = idnm;
        pl_free(name);
      } else if (idnm->uses>0) {
        idnm->uses--;
      } else if (idnm->uses==-2) {
        char *name = idnm->n.ame;
        idnm->n.ame = 0;
        pl_free(name);
      } else if (idnm->uses<-2) {
        idnm->uses++;
      }
    }
  }
}

char *
pl_idname(pl_hashkey_t id)
{
  id_name *idnm = id_table? pl_hfind(id_table, id) : 0;
  return idnm? idnm->n.ame : 0;
}

static id_name *
id_block(void)
{
  int i, n = 128;
  id_name *idnm = pl_malloc(sizeof(id_name)*n);
  for (i=0 ; i<n-1 ; i++) idnm[i].n.ext = &idnm[i+1];
  idnm[i].n.ext = 0;
  return id_freelist = idnm;
}

static pl_hashkey_t
id_hash(const char *name, int len)
{
  pl_hashkey_t h = 0x34da3723;  /* random bits */
  if (len) for (; len && name[0] ; len--,name++)
    h = ((h<<8) + (h>>1)) ^ (unsigned char)name[0];
  else while (*name)
    h = ((h<<8) + (h>>1)) ^ (unsigned char)(*name++);
  return h;
}
