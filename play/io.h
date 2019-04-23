/*
 * $Id: play/io.h,v 1.2 2009-05-22 04:02:26 dhmunro Exp $
 * portability layer I/O wrappers
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

/* filesystem services (mostly ANSI or POSIX)
 * - necessary to implement UNIX-like filenaming semantics universally
 * - pl_free frees result of pl_getcwd, pl_frall for pl_lsdir result */

#ifndef _PLAY_IO_H
#define _PLAY_IO_H 1

#include <play/extern.h>

typedef struct _pl_file pl_file_t;
typedef struct _pl_dir pl_dir_t;

PL_BEGIN_EXTERN_C

/* support virtual file objects:
 * first member of struct _pl_file is pl_file_ops_t*
 * other functions besides pl_fopen, pl_popen may create pl_file_t* objects
 */
typedef struct _pl_file_ops pl_file_ops_t;
struct _pl_file_ops {
  unsigned long (*v_fsize)(pl_file_t *file);
  unsigned long (*v_ftell)(pl_file_t *file);
  int (*v_fseek)(pl_file_t *file, unsigned long addr);

  char *(*v_fgets)(pl_file_t *file, char *buf, int buflen);
  int (*v_fputs)(pl_file_t *file, const char *buf);
  unsigned long (*v_fread)(pl_file_t *file,
                           void *buf, unsigned long nbytes);
  unsigned long (*v_fwrite)(pl_file_t *file,
                            const void *buf, unsigned long nbytes);

  int (*v_feof)(pl_file_t *file);
  int (*v_ferror)(pl_file_t *file);
  int (*v_fflush)(pl_file_t *file);
  int (*v_fclose)(pl_file_t *file);
};

PL_EXTERN pl_file_t *pl_fopen(const char *unix_name, const char *mode);
PL_EXTERN pl_file_t *pl_popen(const char *command, const char *mode);
PL_EXTERN pl_file_t *pl_fd_raw(int fd);

PL_EXTERN unsigned long pl_fsize(pl_file_t *file);
PL_EXTERN unsigned long pl_ftell(pl_file_t *file);
PL_EXTERN int pl_fseek(pl_file_t *file, unsigned long addr);

PL_EXTERN char *pl_fgets(pl_file_t *file, char *buf, int buflen);
PL_EXTERN int pl_fputs(pl_file_t *file, const char *buf);
PL_EXTERN unsigned long pl_fread(pl_file_t *file,
                               void *buf, unsigned long nbytes);
PL_EXTERN unsigned long pl_fwrite(pl_file_t *file,
                                const void *buf, unsigned long nbytes);

PL_EXTERN int pl_feof(pl_file_t *file);
PL_EXTERN int pl_ferror(pl_file_t *file);
PL_EXTERN int pl_fflush(pl_file_t *file);
PL_EXTERN int pl_fclose(pl_file_t *file);

PL_EXTERN int pl_remove(const char *unix_name);
PL_EXTERN int pl_rename(const char *unix_old, const char *unix_new);

PL_EXTERN int pl_chdir(const char *unix_name);
PL_EXTERN int pl_rmdir(const char *unix_name);
PL_EXTERN int pl_mkdir(const char *unix_name);
PL_EXTERN char *pl_getcwd(void);

PL_EXTERN pl_dir_t *pl_dopen(const char *unix_name);
PL_EXTERN int pl_dclose(pl_dir_t *dir);
/* returned filename does not need to be freed, but
 * value may be clobbered by dclose, next dnext, or pl_wkspc use
 * . and .. do not appear in returned list */
PL_EXTERN char *pl_dnext(pl_dir_t *dir, int *is_dir);

PL_END_EXTERN_C

#endif /* _PLAY_IO_H */
