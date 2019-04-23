/*
 * $Id: pathnm.c,v 1.1 2005-09-18 22:05:40 dhmunro Exp $
 * pl_getenv and _pl_u_pathname pathname preprocessing for UNIX machines
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#endif

#include "config.h"
#include "play2.h"
#include "playu.h"
#include "play/std.h"

#include <string.h>

#ifndef NO_PASSWD
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#endif

char *
pl_getenv(const char *name)
{
  return getenv(name);
}

char *
_pl_u_pathname(const char *name)
{
  const char *tmp;
  long len = 0;
  long left = PL_WKSIZ;

  pl_wkspc.c[0] = '\0';

  if (name[0]=='$') {
    int delim = *(++name);
    if (delim=='(')      { delim = ')';  name++; }
    else if (delim=='{') { delim = '}';  name++; }
    else                   delim = '/';
    tmp = strchr(name, delim);
    if (!tmp) tmp = name+strlen(name);
    if (tmp>name+left) return pl_wkspc.c;
    if (tmp>name) {
      char *env;
      len = tmp-name;
      strncat(pl_wkspc.c, name, len);
      env = getenv(pl_wkspc.c);
      if (!env) return &pl_wkspc.c[len];
      len = strlen(env);
      left -= len;
      if (left<0) return &pl_wkspc.c[len];
      strcpy(pl_wkspc.c, env);
      if (delim!='/') tmp++;
      name = tmp;
    }

  } else if (name[0]=='~') {
    char *home = 0;
    tmp = strchr(++name, '/');
    if (!tmp) {
      len = strlen(name);
      if (len>left) return pl_wkspc.c;
      strcpy(pl_wkspc.c, name);
      name += len;
    } else {
      len = tmp-name;
      if (tmp>name+left) return pl_wkspc.c;
      if (len) strncat(pl_wkspc.c, name, len);
      name = tmp;
    }
#ifndef NO_PASSWD
    if (!pl_wkspc.c[0]) {
      home = getenv("HOME");
      if (!home) {
        struct passwd *pw = getpwuid(getuid());  /* see also usernm.c */
        if (pw) home = pw->pw_dir;
      }
    } else {
      struct passwd *pw = getpwnam(pl_wkspc.c);
      if (pw) home = pw->pw_dir;
    }
#else
    home = pl_wkspc.c[0]? 0 : getenv("HOME");
#endif
    if (!home) return &pl_wkspc.c[len];
    len = strlen(home);
    left -= len;
    if (left<0) return &pl_wkspc.c[len];
    strcpy(pl_wkspc.c, home);
  }

  if (strlen(name)<=left) strcpy(pl_wkspc.c+len, name);
  else pl_wkspc.c[0] = '\0';
  return pl_wkspc.c;
}
