/*
 * $Id: config.c,v 1.1 2005-09-18 22:05:39 dhmunro Exp $
 * configuration tester for UNIX machines
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#define MAIN_DECLARE int main(int argc, char *argv[])
#define MAIN_RETURN(value) return value

#ifdef TEST_GCC
# ifdef __GNUC__
MAIN_DECLARE {
  MAIN_RETURN(0); }
# else
#error not gcc
# endif
#endif

#ifdef TEST_UTIME
/* check settings of: USE_GETRUSAGE USE_TIMES */
#include "timeu.c"
MAIN_DECLARE {
  double s;
  double t = pl_cpu_secs(&s);
  MAIN_RETURN(t!=0.); }
#endif

#ifdef TEST_WTIME
/* check settings of: USE_GETTIMEOFDAY */
#include "timew.c"
MAIN_DECLARE {
  double t = pl_wall_secs();
  MAIN_RETURN(t!=0.); }
#endif

#ifdef TEST_USERNM
/* check settings of: NO_PASSWD */
#include "usernm.c"
MAIN_DECLARE {
  char *u = pl_getuser();
  MAIN_RETURN(u==0); }
#endif

#ifdef TEST_TIOCGPGRP
/* check settings of: USE_TIOCGPGRP_IOCTL */
#include "uinbg.c"
MAIN_DECLARE {
  MAIN_RETURN(_pl_u_in_background()); }
#endif

#ifdef TEST_GETCWD
/* check settings of: USE_GETWD */
#include <unistd.h>
static char dirbuf[1024];
#ifdef USE_GETWD
#define getcwd(x,y) getwd(x)
#endif
MAIN_DECLARE {
  char *u = getcwd(dirbuf, 1024);
  MAIN_RETURN(u==0); }
#endif

#ifdef TEST_DIRENT
/* check settings of: DIRENT_HEADER USE_GETWD */
#include "dir.c"
pl_wkspc_t pl_wkspc;
MAIN_DECLARE {
  pl_dir_t *d = pl_dopen("no/such/thing");
  int value = 0;
  char *l = pl_dnext(d, &value);
  MAIN_RETURN(pl_chdir(l) || pl_rmdir(l) || pl_mkdir(l)); }
#endif

#ifdef TEST_POLL
/* check settings of: USE_SYS_POLL_H USE_SELECT HAVE_SYS_SELECT_H
                      NO_SYS_TIME_H NEED_SELECT_PROTO */
#include "uevent.c"
MAIN_DECLARE {
  int p = _pl_u_poll(1000);
  MAIN_RETURN(p!=0); }
#endif

#ifdef TEST_PLUG
/* check settings of: PLUG_LIBDL PLUG_HPUX PLUG_MACOSX */
/* also check that udltest.c can call function in main executable */
extern int testcall(int check);
int testcall(int check) {
  return (check == 13579);
}
#include "udltest.c"
MAIN_DECLARE {
  union {
    void *data;
    void (*function)(int);
  } addr;
  void *h = test_dlopen();
  if (!h) {
    return 1;
  } else if (!(test_dlsym(h, 1, &addr) & 1)) {
    return 2;
  } else {
    int *pdat = addr.data;
    if (!pdat || pdat[0]!=-1 || pdat[1]!=-2) {
      return 3;
    } else if (!(test_dlsym(h, 0, &addr) & 2)) {
      return 4;
    } else {
      void (*pfun)(int) = addr.function;
      pdat[0] = 24680;
      pfun(13579);
      if (pdat[0]!=13579 || pdat[1]!=24680) {
        return 5;
      }
    }
  }
  MAIN_RETURN(0); }
#endif

#ifdef TEST_SOCKETS
/* check if IEEE 1003.1 sockets present, seek usock.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
MAIN_DECLARE {
  static struct addrinfo hints, *ai;
  char text[1025];
  int fd;
  hints.ai_family = AF_UNIX;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;  /* will call bind, not connect */
  getaddrinfo(0, text, &hints, &ai);
  fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (fd != -1) {
    struct sockaddr_storage sad;
    socklen_t lsad = sizeof(struct sockaddr_storage);
    struct sockaddr *psad = (struct sockaddr *)&sad;
    getsockname(fd, psad, &lsad);
    getnameinfo(psad, lsad, 0, 0, text, 1025, NI_NUMERICSERV);
  }
  MAIN_RETURN(0); }
#endif

#ifdef TEST_FENV_H
#include <fenv.h>
#include <signal.h>
MAIN_DECLARE {
  int except = FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW;
  fenv_t env;
  if (fetestexcept(except)) {
    feclearexcept(except);
  }
  fegetenv(&env);
  fesetenv(FE_DFL_ENV);
  MAIN_RETURN(0); }
#endif
