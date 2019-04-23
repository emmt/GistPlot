/*
 * $Id: pathnm.c,v 1.1 2005-09-18 22:05:35 dhmunro Exp $
 * pl_getenv and _pl_w_pathname pathname preprocessing for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"
#include "play/std.h"

#include <string.h>

/* GetEnvironmentVariable is in kernel32.lib */

char *
pl_getenv(const char *name)
{
  DWORD flag = GetEnvironmentVariable(name, pl_wkspc.c, PL_WKSIZ);
  return (flag>0 && flag<PL_WKSIZ)? pl_wkspc.c : 0;
}

char *
_pl_w_pathname(const char *name)
{
  const char *tmp = name;
  long len = 0;
  long left = PL_WKSIZ;

  if (name[0]=='$') {
    int delim = *(++name);
    if (delim=='(')      { delim = ')';  name++; }
    else if (delim=='{') { delim = '}';  name++; }
    else                   delim = '\0';
    for (tmp=name ; tmp[0] ; tmp++)
      if (delim? (tmp[0]==delim) : (tmp[0]=='/' || tmp[0]=='\\')) break;
    if (tmp>name+1024) {
      pl_wkspc.c[0] = '\0';
      return pl_wkspc.c;
    }
    len = tmp-name;
    if (len && delim) tmp++;
  } else if (name[0]=='~' && (!name[1] || name[1]=='/' || name[1]=='\\')) {
    tmp++;
    len = -1;
  }
  if (len) {
    char env_name[1024];
    if (len < 0) {
      strcpy(env_name, "HOME");
    } else {
      env_name[0] = '\0';
      strncat(env_name, name, len);
    }
    len = GetEnvironmentVariable(env_name, pl_wkspc.c, PL_WKSIZ);
    if (len>PL_WKSIZ) len = left+1;
    left -= len;
    if (left<0) {
      pl_wkspc.c[0] = '\0';
      return pl_wkspc.c;
    }
    name = tmp;
  }

  if ((long)strlen(name)<=left) strcpy(pl_wkspc.c+len, name);
  else pl_wkspc.c[0] = '\0';

  for (left=0 ; pl_wkspc.c[left] ; left++)
    if (pl_wkspc.c[left]=='/') pl_wkspc.c[left] = '\\';
  return pl_wkspc.c;
}

char *
_pl_w_unixpath(const char *wname)
{
  int i;
  for (i=0 ; i<PL_WKSIZ && wname[i] ; i++)
    pl_wkspc.c[i] = (wname[i]=='\\')? '/' : wname[i];
  return pl_wkspc.c;
}
