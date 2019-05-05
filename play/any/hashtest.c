/*
 * hashtest.c -
 *
 * Test hash functions.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "play/hash.h"
#include "play/std.h"
#include "play/io.h"
#include <stdio.h>
#include <string.h>

static int mxsyms = 50000;
static void test_delete(void *value);
static char nm1[] = "  name 1  ";
static char nm2[] = "  name 2  ";

int
main(int argc, char *argv[])
{
  pl_hashkey_t *id, collist[32];
  pl_hashtab_t *tab;
  int i, ncoll;
  int nsyms = 0;
  char **syms = pl_malloc(sizeof(char *)*mxsyms);

  if (argc>1) {
    char line[120];
    FILE *f = fopen(argv[1], "r");
    if (!f) {
      printf("hashtest: unable to open %s\n", argv[1]);
      exit(1);
    }
    for (nsyms=0 ; nsyms<mxsyms ; nsyms++) {
      if (!fgets(line, 120, f)) break;
      i = strlen(line);
      if (i<1) continue;
      if (line[i-1]=='\n') {
        i--;
        line[i] = '\0';
        if (i<1) continue;
      }
      syms[nsyms] = pl_strcpy(line);
    }
    fclose(f);
  } else {
    syms[0] = pl_strcpy("first");
    syms[1] = pl_strcpy("second");
    syms[2] = pl_strcpy("third");
    syms[3] = pl_strcpy("fourth");
    syms[4] = pl_strcpy("fifth");
    nsyms = 5;
  }

  tab = pl_halloc(4);
  for (i=0 ; i<nsyms ; i++) pl_hinsert(tab, PL_PHASH(syms[i]), syms[i]);
  for (i=0 ; i<nsyms ; i++)
    if (pl_hfind(tab, PL_PHASH(syms[i])) != syms[i]) {
      puts("hashtab: pl_hfind failed first test");
      exit(1);
    }

  for (i=0 ; i<nsyms/2 ; i++) pl_hinsert(tab, PL_PHASH(syms[i]), (void *)0);
  for (i=0 ; i<nsyms/2 ; i++)
    if (pl_hfind(tab, PL_PHASH(syms[i]))) {
      puts("hashtab: pl_hfind failed first removal test");
      exit(1);
    }
  for ( ; i<nsyms ; i++)
    if (pl_hfind(tab, PL_PHASH(syms[i])) != syms[i]) {
      puts("hashtab: pl_hfind failed second removal test");
      exit(1);
    }

  pl_hfree(tab, &test_delete);
  for (i=0 ; i<nsyms ; i++) {
    if (((syms[i][0] & 0x80)==0) == (i<nsyms/2)) syms[i][0] &= 0x7f;
    else {
      puts("hashtab: pl_hfree deletion callback failed");
      exit(1);
    }
  }

  id = pl_malloc(sizeof(pl_hashkey_t)*(nsyms+2));
  for (i=0 ; i<nsyms ; i++) id[i] = pl_idmake(syms[i],
                                             (i&1)? strlen(syms[i]) : 0);
  for (i=0 ; i<nsyms ; i++)
    if (pl_id(syms[i], 0) != id[i]) {
      puts("hashtab: pl_id failed first test");
      exit(1);
    }
  id[nsyms] = pl_idstatic(nm1);
  id[nsyms+1] = pl_idstatic(nm2);
  for (i=0 ; i<nsyms ; i++)
    if (strcmp(pl_idname(id[i]), syms[i])) {
      puts("hashtab: pl_idname failed first test");
      exit(1);
    }
  if (pl_id("#%  #$&\t^",0)) {
    puts("hashtab: pl_id failed second test");
    exit(1);
  }
  if (pl_id(nm1,0)!=id[nsyms] ||
      pl_id(nm2,0)!=id[nsyms+1]) {
    puts("hashtab: pl_id failed third test");
    exit(1);
  }
  if (strcmp(pl_idname(id[nsyms]),nm1) ||
      strcmp(pl_idname(id[nsyms+1]),nm2)) {
    puts("hashtab: pl_idname failed third test");
    exit(1);
  }
  for (i=0 ; i<nsyms ; i+=2)
    if (pl_idmake(syms[i],0)!=id[i]) {
      puts("hashtab: pl_idmake failed repeat test");
      exit(1);
    }
  for (i=0 ; i<nsyms ; i+=3) {
    pl_idfree(id[i]);
    if (i&1) id[i] = 0;
  }
  for (i=0 ; i<nsyms ; i++)
    if (pl_id(syms[i], 0) != id[i]) {
      puts("hashtab: pl_id failed fourth test");
      exit(1);
    }
  for (i=0 ; i<nsyms ; i++) if (id[i]) pl_idfree(id[i]);
  ncoll = 0;
  for (i=0 ; i<nsyms ; i++)
    if ((pl_id(syms[i], 0)!=0) == (i&1 || !((i/2)%3))) {
      if (ncoll<32) collist[ncoll] = id[i];
      ncoll++;
    }
  {
    extern int pl_id_collisions;
    printf("hashtab: pl_id had %d collisions out of %d symbols\n",
           pl_id_collisions, nsyms);
  }

  for (i=0 ; i<nsyms ; i++) pl_setctx(syms[i], syms[i]+1);
  for (i=0 ; i<nsyms ; i++)
    if (pl_getctx(syms[i]) != syms[i]+1) {
      puts("hashtab: pl_getctx failed first test");
      exit(1);
    }
  for (i=0 ; i<nsyms ; i+=2) pl_setctx(syms[i], (void *)0);
  for (i=0 ; i<nsyms ; i++)
    if (pl_getctx(syms[i]) != ((i&1)? syms[i]+1 : 0)) {
      puts("hashtab: pl_getctx failed second test");
      exit(1);
    }

  for (i=0 ; i<nsyms ; i++) pl_free(syms[i]);
  pl_free(syms);
  exit(0);
  return 0;
}

static void
test_delete(void *value)
{
  char *sym = value;
  sym[0] |= 0x80;
}
