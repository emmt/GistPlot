/*
 * $Id: hash.c,v 1.1 2005-09-18 22:05:42 dhmunro Exp $
 * unique key hashing functions
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/hash.h"
#include "play/std.h"

typedef struct _pl_hashent pl_hashent_t;

struct _pl_hashtab {
  pl_hashkey_t mask;       /* nslots-1 */
  pl_hashent_t **slots;
  pl_hashent_t *freelist;
  pl_hashent_t *entries;
  long nitems;          /* informational only */
};

struct _pl_hashent {
  pl_hashent_t *next;
  pl_hashkey_t hkey;   /* could keep key, but pexpand faster with this */
  void *value;
};

static pl_hashent_t *pl_hexpand(pl_hashtab_t *tab);
static int pl_hremove(pl_hashtab_t *tab, pl_hashkey_t hkey);

pl_hashtab_t *
pl_halloc(pl_hashkey_t size)
{
  pl_hashtab_t *tab;
  pl_hashent_t *e;
  pl_hashkey_t i;
  pl_hashkey_t n = 4;
  if (size>100000) size = 100000;
  while (n<size) n<<= 1;
  n<<= 1;
  tab = pl_malloc(sizeof(pl_hashtab_t));
  tab->nitems = 0;
  tab->mask = n-1;
  tab->slots = pl_malloc(sizeof(pl_hashent_t *)*n);
  for (i=0 ; i<n ; i++) tab->slots[i] = 0;
  n>>= 1;
  e = pl_malloc(sizeof(pl_hashent_t)*n);
  for (i=0 ; i<n-1 ; i++) e[i].next = &e[i+1];
  e[i].next = 0;
  tab->entries = tab->freelist = e;
  return tab;
}

void
pl_hfree(pl_hashtab_t *tab, void (*func)(void *))
{
  pl_hashent_t **slots = tab->slots;
  pl_hashent_t *entries = tab->entries;
  if (func) {
    pl_hashkey_t n = tab->mask+1;
    pl_hashent_t *e;
    pl_hashkey_t i;
    for (i=0 ; i<n ; i++)
      for (e=tab->slots[i] ; e ; e=e->next) func(e->value);
  }
  tab->slots = 0;
  tab->freelist = tab->entries = 0;
  pl_free(slots);
  pl_free(entries);
  pl_free(tab);
}

void
pl_hiter(pl_hashtab_t *tab,
        void (*func)(void *val, pl_hashkey_t key, void *ctx), void *ctx)
{
  pl_hashkey_t n = tab->mask+1;
  pl_hashent_t *e;
  pl_hashkey_t i;
  for (i=0 ; i<n ; i++)
    for (e=tab->slots[i] ; e ; e=e->next)
      func(e->value, e->hkey, ctx);
}

int
pl_hinsert(pl_hashtab_t *tab, pl_hashkey_t hkey, void *value)
{
  pl_hashent_t *e;
  if (pl_signalling) return 1;
  if (!value) return pl_hremove(tab, hkey);
  for (e=tab->slots[hkey&tab->mask] ; e && e->hkey!=hkey ; e=e->next);
  if (!e) {
    e = tab->freelist;
    if (!e) {
      e = pl_hexpand(tab);
      if (!e) return 1;
    }
    /* CRITICAL SECTION BEGIN */
    e->hkey = hkey;
    hkey &= tab->mask;
    tab->freelist = e->next;
    e->next = tab->slots[hkey];
    tab->slots[hkey] = e;
    /* CRITICAL SECTION END */
    tab->nitems++;
  }
  e->value = value;
  return 0;
}

void *
pl_hfind(pl_hashtab_t *tab, pl_hashkey_t hkey)
{
  pl_hashent_t *e;
  for (e=tab->slots[hkey&tab->mask] ; e ; e=e->next)
    if (e->hkey==hkey) return e->value;
  return 0;
}

static int
pl_hremove(pl_hashtab_t *tab, pl_hashkey_t hkey)
{
  pl_hashent_t *e, **pe = &tab->slots[hkey & tab->mask];
  for (e=*pe ; e ; e=*pe) {
    if (e->hkey==hkey) {
      /* CRITICAL SECTION BEGIN */
      *pe = e->next;
      e->next = tab->freelist;
      tab->freelist = e;
      /* CRITICAL SECTION END */
      tab->nitems--;
      break;
    }
    pe = &e->next;
  }
  return 0;
}

static pl_hashent_t *
pl_hexpand(pl_hashtab_t *tab)
{
  pl_hashkey_t n = tab->mask + 1;
  pl_hashent_t **slots = pl_malloc(sizeof(pl_hashent_t *)*(n<<1));
  if (slots) {
    pl_hashent_t *e = pl_malloc(sizeof(pl_hashent_t)*n);
    if (e) {
      pl_hashent_t *e0, **pe, **qe;
      pl_hashkey_t i;
      for (i=0 ; i<n ; i++) {
        pe = &slots[i];
        qe = &pe[n];
        for (e0=tab->slots[i] ; e0 ; e0=e0->next,e++) {
          /* exactly n/2 passes through this loop body */
          e->value = e0->value;
          if ((e->hkey = e0->hkey) & n) {
            *qe = e;
            qe = &e->next;
          } else {
            *pe = e;
            pe = &e->next;
          }
        }
        *pe = *qe = 0;
      }
      n>>= 1;
      for (i=0 ; i<n-1 ; i++) e[i].next = &e[i+1];
      e[i].next = 0;
      pe = tab->slots;
      e0 = tab->entries;
      /* CRITICAL SECTION BEGIN */
      tab->mask = (tab->mask<<1) | 1;
      tab->slots = slots;
      tab->entries = e-n;
      tab->freelist = e;
      /* CRITICAL SECTION END */
      pl_free(pe);
      pl_free(e0);
      return e;
    } else {
      pl_free(slots);
    }
  }
  return 0;
}
