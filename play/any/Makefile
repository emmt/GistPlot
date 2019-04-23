# $Id: Makefile,v 1.2 2009-05-22 04:02:26 dhmunro Exp $
SHELL=/bin/sh
MAKE=make
include ../../Make.cfg

COPT=$(COPT_DEFAULT)
CFLAGS=$(COPT) $(GIST_CFLAGS) -I. -I.. -I../.. $(PLAY_INCS) $(HACK_FLAGS)
CCLOAD=$(CC) $(COPT) $(GIST_LDFLAGS)

OBJS=hash.o hash0.o hashctx.o hashid.o mm.o mminit.o \
  alarms.o pmemcpy.o pstrcpy.o pstrncat.o p595.o \
  bitrev.o bitlrot.o bitmrot.o pstdio.o psoftfpe.o

all: libplay

libplay: $(OBJS)
	$(AR) rc ../libplay2.a $(OBJS)
	$(RANLIB) ../libplay2.a
	touch $@

EXE_FILES=numfmt test2d hashtest

clean::
	rm -f *~ '#'* *.o *.a main.c core* *.core a.out $(EXE_FILES) libplay

distclean:: clean
	rm -f .gdb*



numfmt: numfmt.c
	$(CC) -g -o $@ numfmt.c

test2d: main.o test2d.o ../libplay2.a
	$(CCLOAD) -o $@ main.o test2d.o -L.. -lplay2 $(X11LIB) -lm

test: test2d

hashtest: hashtest.o ../libplay2.a
	$(CCLOAD) -o $@ hashtest.o -L.. -lplay2

hash.o: $(PLAY_CFG)/config.h ../hash.h ../std.h ../extern.h
hash0.o: $(PLAY_CFG)/config.h ../hash.h ../extern.h
hashctx.o: $(PLAY_CFG)/config.h ../hash.h ../extern.h
hashid.o: $(PLAY_CFG)/config.h ../hash.h ../std.h ../extern.h

mm.o: $(PLAY_CFG)/config.h ../std.h ../extern.h
mminit.o: $(PLAY_CFG)/config.h ../std.h ../extern.h
pfrall.o: $(PLAY_CFG)/config.h ../std.h ../extern.h

alarms.o: $(PLAY_CFG)/config.h ../play2.h ../std.h ../extern.h

pmemcpy.o: $(PLAY_CFG)/config.h ../std.h ../extern.h
pstrcpy.o: $(PLAY_CFG)/config.h ../std.h ../extern.h
pstrncat.o: $(PLAY_CFG)/config.h ../std.h ../extern.h

bitrev.o: $(PLAY_CFG)/config.h ../play2.h ../extern.h
bitlrot.o: $(PLAY_CFG)/config.h ../play2.h ../extern.h
bitmrot.o: $(PLAY_CFG)/config.h ../play2.h ../extern.h
p595.o: ../play2.h ../extern.h

pstdio.o: $(PLAY_CFG)/config.h ../io.h ../extern.h

psoftfpe.o: $(PLAY_CFG)/config.h ../play2.h ../extern.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_HAS_FENV_H) -c psoftfpe.c

test2d.o: ../play2.h ../std.h ../io.h ../extern.h
hashtest.o: ../hash.h ../std.h ../io.h ../extern.h

main.o: main.c ../play2.h ../extern.h
main.c: ../$(GIST_MAIN_C)
	cp ../$(GIST_MAIN_C) main.c
