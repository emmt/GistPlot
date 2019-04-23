# $Id: Makefile,v 1.1 2005-09-18 22:05:38 dhmunro Exp $
# for cygwin mingw

SHELL=/bin/sh
MAKE=make
include ../../Make.cfg

CFLAGS=$(COPTIONS) -I. -I.. -I../..
COPTIONS=$(COPT) $(GIST_CFLAGS)
LDOPTIONS=$(COPT) $(GIST_LDFLAGS)
COPT=$(COPT_DEFAULT)

OBJS=dir.o files.o handler.o sigansi.o pathnm.o conterm.o timeu.o \
  timew.o usernm.o wpoll.o wdl.o wstdio.o cygapp.o cygmain.o \
  clips.o cursors.o ellipse.o feep.o getdc.o pals.o pcell.o pfill.o plines.o \
  points.o prect.o pscr.o ptext.o pwin.o wspawn.o

all: libplay

../win.h: playwin.h
	rm -f '$@'
	cp '$<' '$@'

libplay: $(OBJS) ../win.h
	$(AR) rc ../libplay2.a $(OBJS)
	$(RANLIB) ../libplay2.a
	touch $@

config:
	cp Make.win ../../Make.cfg

CLEANUP_LIST=cfg* config.0* fputest core* *.core a.out wdltest.??*
clean::
	rm -f *~ '#'* *.o *.a $(CLEANUP_LIST) libplay

distclean:: clean


conterm.o: playw.h ../play2.h ../extern.h
cygapp.o: playw.h ../std.h ../play2.h ../extern.h
#cygmain.o:
dir.o: config.h playw.h ../io.h ../std.h ../play2.h ../extern.h
files.o: config.h playw.h ../io.h ../std.h ../play2.h ../extern.h
handler.o: playw.h ../std.h ../play2.h ../extern.h
pathnm.o: playw.h ../std.h ../play2.h ../extern.h
sigansi.o: playw.h ../std.h ../play2.h ../extern.h
timeu.o: ../play2.h ../extern.h
timew.o: ../play2.h ../extern.h
usernm.o: ../play2.h ../std.h ../extern.h
wdl.o: config.h playw.h ../std.h ../play2.h ../extern.h
wpoll.o: playw.h ../std.h ../play2.h ../extern.h
wstdio.o: playw.h ../std.h ../play2.h ../extern.h

clips: playw.h ../play2.h ../extern.h
cursors: playw.h ../play2.h ../extern.h
ellipse.o: playw.h ../play2.h ../extern.h
feep.o: playw.h ../play2.h ../extern.h
getdc.o: playw.h ../std.h ../play2.h ../extern.h
pals.o: playw.h ../std.h ../play2.h ../extern.h
pcell.o: playw.h ../play2.h ../extern.h
pfill.o: playw.h ../play2.h ../extern.h
plines.o: playw.h ../play2.h ../extern.h
points.o: playw.h ../play2.h ../extern.h
prect.o: playw.h ../play2.h ../extern.h
pscr.o: playw.h ../std.h ../play2.h ../extern.h
ptext.o: playw.h ../std.h ../play2.h ../extern.h
pwin.o: playw.h ../std.h ../play2.h ../extern.h

#	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_UEVENT) -c uevent.c

# this is the DL config test used in config.sh
YORICK_EXE=./cfg
YORICK_DEF=wdltest.def
LDEXE_OPTS=$(LDOPTIONS) $(LDFLAGS) $(PL_EXPORT) -I.
CCSO_OPTS=$(CPPFLAGS) $(CFLAGS) $(PLUG_PIC)
LDSO_OPTS=$(LDOPTIONS) $(LDFLAGS) $(PLUG_SHARED)
wdltest: wdlmain.c wdltest.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c wdlmain.c
	$(CC) $(LDEXE_OPTS) -o cfg.exe wdlmain.o wdlmain.def
	$(CC) $(CCSO_OPTS) -c wdltest.c
	$(CC) $(LDSO_OPTS) -o wdltest$(PLUG_SFX) wdltest.o $(PLUG_DEF)
	./cfg
