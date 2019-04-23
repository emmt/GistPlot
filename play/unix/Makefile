# $Id: Makefile,v 1.3 2010-02-28 21:32:23 dhmunro Exp $
SHELL=/bin/sh
MAKE=make
include ../../Make.cfg

COPT=$(COPT_DEFAULT)
CFLAGS=$(COPT) $(GIST_CFLAGS) -I. -I.. -I../..
COPTIONS=$(COPT) $(GIST_CFLAGS)
LDOPTIONS=$(COPT) $(GIST_LDFLAGS)

OBJS=dir.o files.o fpuset.o handler.o pathnm.o slinks.o stdinit.o timeu.o \
  timew.o udl.o uevent.o ugetc.o uinbg.o usernm.o umain.o uspawn.o usock.o

all: libplay

libplay: $(OBJS)
	$(AR) rc ../libplay2.a $(OBJS)
	$(RANLIB) ../libplay2.a
	touch $@

config:
	CC="$(CC)" CFLAGS="$(GIST_CFLAGS)" LDFLAGS="$(GIST_LDFLAGS)" MATHLIB="$(MATHLIB)" ./config.sh

CLEANUP_LIST=cfg* config.0* fputest core* *.core a.out udltest.??*
clean::
	rm -f *~ '#'* *.o *.a $(CLEANUP_LIST) libplay

distclean:: clean
	rm -f config.h


dir.o: config.h ../play2.h playu.h ../std.h ../io.h dir.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_DIR1) $(D_DIR2) -c dir.c
files.o: config.h ../io.h ../std.h playu.h files.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_NO_PROCS) -c files.c
fpuset.o: config.h playu.h fpuset.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_FPUSET) -c fpuset.c
handler.o: config.h ../play2.h playu.h ../std.h ../extern.h
pathnm.o: config.h ../play2.h playu.h ../std.h pathnm.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_USERNM) -c pathnm.c
slinks.o: config.h playu.h ../std.h ../io.h ../extern.h
stdinit.o: config.h ../play2.h ../std.h ../io.h playu.h ../extern.h
timeu.o: config.h ../play2.h timeu.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_TIMEU) -c timeu.c
timew.o: config.h ../play2.h timew.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_TIMEW) -c timew.c
udl.o: config.h ../std.h udl.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PLUG_UDL) -c udl.c
uevent.o: config.h upoll.h upoll.c ../play2.h playu.h ../std.h uevent.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_UEVENT) -c uevent.c
ugetc.o: config.h playu.h ugetc.h ../events.h ../extern.h
uinbg.o: config.h playu.h uinbg.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_UINBG1) $(D_UINBG2) -c uinbg.c
usernm.o: config.h ../play2.h usernm.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_USERNM) -c usernm.c
umain.o: config.h ../play2.h playu.h ../std.h ../extern.h
uspawn.o: config.h ../io.h ../std.h ../play2.h playu.h uspawn.c ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_NO_PROCS) -c uspawn.c
usock.o: config.h ../std.h ../play2.h playu.h usock.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_NO_SOCKETS) -c usock.c

# this is the DL config test used in config.sh
YORICK_EXE=./cfg
LDEXE_OPTS=$(LDOPTIONS) $(LDFLAGS) -DTEST_PLUG $(PL_EXPORT) $(PLUG_UDL) -I.
CCSO_OPTS=$(CPPFLAGS) $(CFLAGS) $(PLUG_PIC)
LDSO_OPTS=$(LDOPTIONS) $(LDFLAGS) $(PLUG_SHARED)
udltest: config.c udltest.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDEXE_OPTS) -o cfg config.c $(PLUG_LIB)
	$(CC) $(CCSO_OPTS) -DTEST_SHARED -c udltest.c
	$(CC) $(LDSO_OPTS) -o udltest$(PLUG_SFX) $(PLUG_DEF) udltest.o
	./cfg
