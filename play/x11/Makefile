# $Id: Makefile,v 1.1 2005-09-18 22:05:33 dhmunro Exp $
SHELL=/bin/sh
MAKE=make
include ../../Make.cfg

COPT=$(COPT_DEFAULT)
CFLAGS=$(COPT) $(GIST_CFLAGS) -I. -I.. -I../.. -I../unix $(XINC)

OBJS=colors.o connect.o cursors.o errors.o events.o feep.o fills.o \
  fonts.o images.o lines.o pals.o pwin.o resource.o rgbread.o textout.o \
  rect.o ellipse.o clips.o points.o

STD_DEPS=../unix/config.h ../play2.h ../hash.h ../win.h playx.h ../extern.h

all: libplay

../win.h: playwin.h
	rm -f '$@'
	cp '$<' '$@'

libplay: $(OBJS) ../win.h
	$(AR) rc ../libplay2.a $(OBJS)
	$(RANLIB) ../libplay2.a
	touch $@

COPTIONS=$(COPT) $(GIST_CFLAGS)
LDOPTIONS=$(COPT) $(GIST_LDFLAGS)
config:
	CC="$(CC)" CFLAGS="$(GIST_CFLAGS)" LDFLAGS="$(GIST_LDFLAGS)" ./xconfig.sh

clean::
	rm -f *~ '#'* *.o *.a core* *.core a.out cfg* libplay

distclean:: clean

connect.o: $(STD_DEPS) ../std.h
resource.o: $(STD_DEPS)
pwin.o: $(STD_DEPS) ../std.h
errors.o: $(STD_DEPS) ../unix/playu.h ../std.h
events.o: $(STD_DEPS) ../unix/playu.h
clips.o: $(STD_DEPS)
feep.o: $(STD_DEPS)

colors.o: $(STD_DEPS)
pals.o: $(STD_DEPS) ../std.h
cursors.o: $(STD_DEPS)
fonts.o: $(STD_DEPS) ../std.h

points.o: $(STD_DEPS)
fills.o: $(STD_DEPS)
rect.o: $(STD_DEPS)
ellipse.o: $(STD_DEPS)
images.o: $(STD_DEPS) ../std.h
lines.o: $(STD_DEPS)
textout.o: $(STD_DEPS) ../std.h
rgbread.o: $(STD_DEPS)
