/*
 * $Id: pstdio.c,v 1.1 2009-06-18 05:19:12 dhmunro Exp $
 *
 * implement virtual play/io.h interface
 */
/* Copyright (c) 2009, David H. Munro
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/io.h"

struct pv_file {
  pl_file_ops_t *ops;
};

unsigned long
pl_fsize(pl_file_t *file)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fsize(file);
}

unsigned long
pl_ftell(pl_file_t *file)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_ftell(file);
}

int
pl_fseek(pl_file_t *file, unsigned long addr)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fseek(file, addr);
}

char *
pl_fgets(pl_file_t *file, char *buf, int buflen)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fgets(file, buf, buflen);
}

int
pl_fputs(pl_file_t *file, const char *buf)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fputs(file, buf);
}

unsigned long
pl_fread(pl_file_t *file, void *buf, unsigned long nbytes)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fread(file, buf, nbytes);
}

unsigned long
pl_fwrite(pl_file_t *file, const void *buf, unsigned long nbytes)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fwrite(file, buf, nbytes);
}

int
pl_feof(pl_file_t *file)
{
  if (file) {
    pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
    return ops->v_feof(file);
  } else {
    return 1;
  }
}

int
pl_ferror(pl_file_t *file)
{
  if (file) {
    pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
    return ops->v_ferror(file);
  } else {
    return 1;
  }
}

int
pl_fflush(pl_file_t *file)
{
  pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
  return ops->v_fflush(file);
}

int
pl_fclose(pl_file_t *file)
{
  if (file) {
    pl_file_ops_t *ops = ((struct pv_file *)file)->ops;
    return ops->v_fclose(file);
  } else {
    return 0;
  }
}
