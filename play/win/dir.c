/*
 * $Id: dir.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * MS Windows version of plib directory operations
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "config.h"
#include "play/std.h"
#include "play/io.h"
#include "playw.h"

#include <string.h>

struct _pl_dir {
  HANDLE hd;
  int count;
  WIN32_FIND_DATA data;
};

pl_dir_t *
pl_dopen(const char *unix_name)
{
  char *name = _pl_w_pathname(unix_name);
  pl_dir_t *dir = pl_malloc(sizeof(pl_dir_t));
  int i = 0;
  while (name[i]) i++;
  if (i>0 && name[i-1]!='\\') name[i++] = '\\';
  name[i++] = '*';
  name[i++] = '\0';
  dir->hd = FindFirstFile(name, &dir->data);
  if (dir->hd != INVALID_HANDLE_VALUE) {
    /* even empty directories contain . and .. */
    dir->count = 0;
  } else {
    pl_free(dir);
    dir = 0;
  }
  return dir;
}

int
pl_dclose(pl_dir_t *dir)
{
  int flag = -(!FindClose(dir->hd));
  pl_free(dir);
  return flag;
}

char *
pl_dnext(pl_dir_t *dir, int *is_dir)
{
  for (;;) {
    if (dir->count++) {
      if (!FindNextFile(dir->hd, &dir->data))
        return 0;   /* GetLastError()==ERROR_NO_MORE_FILES or error */
    }
    if (dir->data.cFileName[0]!='.' ||
        (dir->data.cFileName[1] && (dir->data.cFileName[1]!='.' ||
                                    dir->data.cFileName[2]))) break;
  }
  *is_dir = ((dir->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0);
  return (char *)dir->data.cFileName;
}

int
pl_chdir(const char *dirname)
{
  const char *path = _pl_w_pathname(dirname);
  /* is this necessary?
  if (path[0] && path[1]==':' &&
      ((path[0]>='a' && path[0]<='z') || (path[0]>='A' && path[0]<='Z'))) {
    char drive[8];
    drive[0] = *path++;
    drive[1] = *path++;
    drive[2] = '\0';
    if (!SetCurrentDrive(drive)) return -1;
  }
  */
  return -(!SetCurrentDirectory(path));
}

int
pl_rmdir(const char *dirname)
{
  return -(!RemoveDirectory(_pl_w_pathname(dirname)));
}

int
pl_mkdir(const char *dirname)
{
  return -(!CreateDirectory(_pl_w_pathname(dirname), 0));
}

char *
pl_getcwd(void)
{
  DWORD n = GetCurrentDirectory(PL_WKSIZ, pl_wkspc.c);
  if (n>PL_WKSIZ || n==0) return 0;
  return _pl_w_unixpath(pl_wkspc.c);
}
