# $Id: Makefile,v 1.1 2005-09-18 22:04:19 dhmunro Exp $
SHELL=/bin/sh
MAKE=make
X11OBJS=xfancy.o xbasic.o
include ../Make.cfg

CFLAGS=$(COPTIONS) -I. -I.. -I../play
CCLOAD=$(CC) $(LDOPTIONS) $(LDFLAGS)
SYS_LIBS=$(X11LIB) $(MATHLIB) $(FPELIB)
COPTIONS=$(COPT) $(GIST_CFLAGS)
LDOPTIONS=$(COPT) $(GIST_LDFLAGS)
COPT=$(COPT_DEFAULT)

D_GISTPATH='-DGISTPATH="~/gist:~/Gist:$(GIST_SITE)/g"'

OBJS=gist.o tick.o tick60.o engine.o text.o draw.o draw0.o clip.o \
  gread.o gcntr.o hlevel.o ps.o cgm.o $(X11OBJS)
BOBJS=browser.o cgmin.o eps.o

all: gist

# always check to see whether ../play/libplay2.a needs to be rebuilt
libgist:
	@cd ../play; $(MAKE) "COPT=$(COPT)"
	@$(MAKE) "COPT=$(COPT)" libgist2.a

../play/libplay2.a:
	@cd ../play; $(MAKE) "COPT=$(COPT)" libplay

libgist2.a: ../play/libplay2.a $(OBJS)
	cp ../play/libplay2.a $@
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@

symbols.lst: libgist2.a
	nm '$<' | sed -r -e '/ [BT] /!d;s/.* [BT] //' | LANG=C sort >'$@'

gist: libgist2.a $(BOBJS) main.o
	$(CCLOAD) -o $@ main.o $(BOBJS) -L. -lgist2 $(SYS_LIBS)

bench: libgist bench.o main.o
	$(CCLOAD) -o $@ main.o bench.o -L. -lgist2 $(SYS_LIBS)

main.c: ../play/$(GIST_MAIN_C)
	cp ../play/$(GIST_MAIN_C) main.c

clean::
	rm -f *~ '#'* *.o *.a core* *.core a.out main.c gist bench symbols.lst

distclean:: clean
	rm -f .gdb*

config:
	@:

# ------------- dependencies -------------

# cgm.h: gist2.h engine.h
# draw.h: gist2.h
# engine.h: gist2.h
# text.h: gist2.h
# hlevel.h: gist2.h
# ps.h: gist2.h engine.h
# xbasic.h: gist2.h engine.h play2.h
# xfancy.h: xbasic.h

PLAY_H=../play/play2.h ../play/extern.h
PSLIB=../play/std.h ../play/extern.h
PSLIO=../play/std.h ../play/io.h ../play/extern.h
PLAY_IO=../play/play2.h ../play/io.h ../play/extern.h
PLAY_STD=../play/std.h ../play/play2.h ../play/extern.h
PLAY_ALL=../play/play2.h ../play/std.h ../play/io.h ../play/extern.h

cgm.o: cgm.h text.h $(PLAY_ALL) gist2.h engine.h
clip.o: clip.h $(PSLIB)
draw.o: draw.c draw.h text.h $(PSLIB) gist2.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NO_EXP10) -c draw.c
draw0.o: draw.h text.h $(PLAY_STD) gist2.h
engine.o: engine.h gist2.h draw.h $(PSLIB)
gcntr.o: gist2.h
gist.o: gist2.h engine.h clip.h $(PLAY_STD)
gread.o: gread.c gist2.h $(PLAY_ALL)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_GISTPATH) -c gread.c
text.o: text.h gist2.h
hlevel.o: hlevel.h xbasic.h engine.h $(PLAY_IO) gist2.h
ps.o: ps.h text.h $(PLAY_IO) gist2.h engine.h
tick.o: tick.c gist2.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NO_EXP10) -c tick.c
tick60.o: gist2.h
xbasic.o: xbasic.h text.h $(PLAY_IO) gist2.h engine.h
xfancy.o: xfancy.c xfancy.h draw.h gist2.h xbasic.h engine.h $(PLAY_H)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NO_EXP10) -c xfancy.c

browser.o: xbasic.h cgm.h cgmin.h eps.h gist2.h engine.h $(PLAY_ALL)
cgmin.o: cgmin.h engine.h $(PLAY_ALL) gist2.h
eps.o: eps.h ps.h text.h $(PSLIO) gist2.h engine.h

bench.o: hlevel.h xfancy.h $(PLAY_ALL) gist2.h xbasic.h engine.h

# Always compile main.c with -g so new packages can be debugged.
main.o: main.c
	$(CC) $(CPPFLAGS) -I. -I.. -I../play -g -c main.c
