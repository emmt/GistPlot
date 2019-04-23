/*
 * $Id: files.c,v 1.6 2009-05-22 04:02:26 dhmunro Exp $
 * UNIX version of play file operations
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _POSIX_SOURCE
/* to get fileno declared */
#define _POSIX_SOURCE 1
#endif
#ifndef _XOPEN_SOURCE
/* to get popen declared */
#define _XOPEN_SOURCE 500
#endif

#include "config.h"
#include "play/io.h"
#include "play/std.h"
#include "playu.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

struct _pl_file {
  pl_file_ops_t *ops;
  FILE *fp;  /* use FILE* for text file operations */
  int fd;    /* use file descriptor for binary file operations */
  int binary;    /* 1 to use read/write/lseek, 2 if pipe */
  int errflag;   /* bit 1 is eof, bit 2 is error for binary==1 */
};

static unsigned long pv_fsize(pl_file_t *file);
static unsigned long pv_ftell(pl_file_t *file);
static int pv_fseek(pl_file_t *file, unsigned long addr);

static char *pv_fgets(pl_file_t *file, char *buf, int buflen);
static int pv_fputs(pl_file_t *file, const char *buf);
static unsigned long pv_fread(pl_file_t *file,
                              void *buf, unsigned long nbytes);
static unsigned long pv_fwrite(pl_file_t *file,
                               const void *buf, unsigned long nbytes);

static int pv_feof(pl_file_t *file);
static int pv_ferror(pl_file_t *file);
static int pv_fflush(pl_file_t *file);
static int pv_fclose(pl_file_t *file);

static pl_file_ops_t u_file_ops = {
  &pv_fsize, &pv_ftell, &pv_fseek,
  &pv_fgets, &pv_fputs, &pv_fread, &pv_fwrite,
  &pv_feof, &pv_ferror, &pv_fflush, &pv_fclose };

pl_file_t *
pl_fopen(const char *unix_name, const char *mode)
{
  FILE *fp = fopen(_pl_u_pathname(unix_name), mode);
  pl_file_t *f = fp? pl_malloc(sizeof(pl_file_t)) : 0;
  if (f) {
    f->ops = &u_file_ops;
    f->fp = fp;
    f->fd = fileno(fp);
    for (; mode[0] ; mode++) if (mode[0]=='b') break;
    f->binary = (mode[0]=='b');
    f->errflag = 0;
    _pl_u_fdwatch(f->fd, 1);
  } else if (fp) {
    fclose(fp);
  }
  return f;
}

pl_file_t *
pl_popen(const char *command, const char *mode)
{
#ifndef NO_PROCS
  FILE *fp = popen(command, mode[0]=='w'? "w" : "r");
  pl_file_t *f = fp? pl_malloc(sizeof(pl_file_t)) : 0;
  if (f) {
    f->ops = &u_file_ops;
    f->fp = fp;
    f->fd = fileno(fp);
    f->binary = 2;
    f->errflag = 0;
    _pl_u_fdwatch(f->fd, 1);
  } else if (fp) {
    pclose(fp);
  }
  return f;
#else
  return (pl_file_t *)0;
#endif
}

pl_file_t *
pl_fd_raw(int fd)
{
  pl_file_t *f = pl_malloc(sizeof(pl_file_t));
  if (f) {
    f->ops = &u_file_ops;
    f->fp = 0;
    f->fd = fd;
    f->binary = 5;
    _pl_u_fdwatch(f->fd, 1);
  }
  return f;
}

static int
pv_fclose(pl_file_t *file)
{
  int flag = (file->binary&2)? pclose(file->fp) : fclose(file->fp);
  _pl_u_fdwatch(file->fd, 0);
  pl_free(file);
  return flag;
}

/* could make closeall/fdwatch ultra-clever,
 * but dumb-and-simple seems to work just as well
 */
static int u_fd_max = 16;  /* minimum possible OPEN_MAX in POSIX limits.h */
void
_pl_u_closeall(void)
{
  int i;
  for (i=3 ; i<u_fd_max ; i++) close(i);
}
void
_pl_u_fdwatch(int fd, int on)
{
  if (on && fd>=u_fd_max) u_fd_max = fd+1;
}

static char *
pv_fgets(pl_file_t *file, char *buf, int buflen)
{
  return fgets(buf, buflen, file->fp);
}

static int
pv_fputs(pl_file_t *file, const char *buf)
{
  return fputs(buf, file->fp);
}

static unsigned long
pv_fread(pl_file_t *file, void *buf, unsigned long nbytes)
{
  if (file->binary & 1) {
    long n = read(file->fd, buf, nbytes);
    if (n!=nbytes && n!=-1L) {
      /* cope with filesystems which permit read/write to return before
       * full request completes, even in blocking mode (NFS, Lustre)
       * - note that read behavior undefined if signed nbytes is < 0
       * - assume (POSIX standard) that read/write cannot return zero
       *   when O_NONBLOCK is not set - they will not return until
       *   at least one byte has been transferred
       *   UNLESS we are positioned at end of file on read
       * - apparently read/write timeout after about 1 second even if
       *   the requested number of bytes has not been transferred
       */
      unsigned long nb = n;
      char *cbuf = buf;
      for (;;) {
        n = read(file->fd, cbuf+nb, nbytes-nb);
        if (n == 0) { file->errflag |= 1;  return nb; }
        if ((nb+=n) == nbytes) return nbytes;
        if (n < 0) { file->errflag |= 2;  return -1L; }
      }
    }
    return n;
  } else {
    return fread(buf, 1, nbytes, file->fp);
  }
}

static unsigned long
pv_fwrite(pl_file_t *file, const void *buf, unsigned long nbytes)
{
  if (file->binary & 1) {
    long n = write(file->fd, buf, nbytes);
    if (n!=nbytes && n!=-1L) {
      /* see comment in pl_fread above */
      unsigned long nb = n;
      const char *cbuf = buf;
      for (;;) {
        n = write(file->fd, cbuf+nb, nbytes-nb);
        if ((nb+=n) == nbytes) return nbytes;
        if (n <= 0) { file->errflag |= 2;  return -1L; }
      }
    }
    return n;
  } else {
    return fwrite(buf, 1, nbytes, file->fp);
  }
}

static unsigned long
pv_ftell(pl_file_t *file)
{
  if (file->binary & 1)
    return lseek(file->fd, 0L, SEEK_CUR);
  else if (!(file->binary & 2))
    return ftell(file->fp);
  else
    return -1L;
}

static int
pv_fseek(pl_file_t *file, unsigned long addr)
{
  file->errflag &= ~1;
  if (file->binary & 1)
    return -(lseek(file->fd, addr, SEEK_SET)==-1L);
  else if (!(file->binary & 2))
    return fseek(file->fp, addr, SEEK_SET);
  else
    return -1;
}

static int
pv_fflush(pl_file_t *file)
{
  return (file->binary & 1)? 0 : fflush(file->fp);
}

static int
pv_feof(pl_file_t *file)
{
  return (file->binary&1)? ((file->errflag&1) != 0) : feof(file->fp);
}

static int
pv_ferror(pl_file_t *file)
{
  int flag = (file->binary&1)? ((file->errflag&2) != 0) : ferror(file->fp);
  clearerr(file->fp);
  file->errflag = 0;
  return flag;
}

static unsigned long
pv_fsize(pl_file_t *file)
{
  struct stat buf;
  if (fstat(file->fd, &buf)) return 0;
  return buf.st_size;
}

int
pl_remove(const char *unix_name)
{
  return remove(_pl_u_pathname(unix_name));
}

int
pl_rename(const char *unix_old, const char *unix_new)
{
  char old[PL_WKSIZ+1];
  old[0] = '\0';
  strncat(old, _pl_u_pathname(unix_old), PL_WKSIZ);
  return rename(old, _pl_u_pathname(unix_new));
}

char *
pl_native(const char *unix_name)
{
  return pl_strcpy(_pl_u_pathname(unix_name));
}
