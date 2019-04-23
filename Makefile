# $Id: Makefile,v 1.4 2010-02-28 21:32:23 dhmunro Exp $
# see README for usage

SHELL=/bin/sh

ALLDIRS=play gist doc
CONFIGDIRS=play

all: gistexe docs

gistexe: libgist
	@cd gist; $(MAKE) gist

docs:
	@cd doc; $(MAKE) docs

# libraries are built in a fixed order:
# libplay, then libgist, then libyor
#   - the libraries are accumulated, that is,
#     libgist2.a contains libplay2.a

libgist: libplay
	@cd gist; $(MAKE) libgist2.a

libplay: Make.cfg
	@cd play; $(MAKE) libplay

LD_STATIC=
Make.cfg:
	LD_STATIC=$(LD_STATIC) ./configure

config: distclean
	@$(MAKE) "LD_STATIC=$(LD_STATIC)" Make.cfg

prefix=.
GIST_PLATFORM=.

clean::
	@rm -f Make.del exports.def
	@if test ! -r Make.cfg; then touch Make.cfg Make.del; fi
	@for d in $(ALLDIRS); do ( cd $$d; $(MAKE) TGT=exe clean; ); done
	@if test -r Make.del; then rm -f Make.cfg Make.del; fi
	rm -f *~ '#'* *.o cfg* core* *.core a.out
	rm -f i/*~ i0/*~ i-start/*~ g/*~ extend/*~

distclean::
	@touch Make.cfg
	@for d in $(ALLDIRS); do ( cd $$d; $(MAKE) TGT=exe distclean; ); done
	rm -f *~ '#'* *.o cfg* Make.* core* *.core a.out
	rm -f i/*~ i0/*~ i-start/*~ g/*~ extend/*~

check:
	@cd play/any; $(MAKE) check

INSTALL_ROOT=
GIST_BINDIR=
GIST_DOCDIR=
install: gistexe docs
	./instally.sh +both "$(INSTALL_ROOT)" "$(GIST_BINDIR)" "$(GIST_DOCDIR)"

install1: gistexe
	./instally.sh +home "$(INSTALL_ROOT)" "$(GIST_BINDIR)" "$(GIST_DOCDIR)"

uninstall:
	./instally.sh -both "$(INSTALL_ROOT)" "$(GIST_BINDIR)" "$(GIST_DOCDIR)"

uninstall1:
	./instally.sh -home "$(INSTALL_ROOT)" "$(GIST_BINDIR)" "$(GIST_DOCDIR)"

dist: siteclean
	W=`pwd`;N=`basename "$$W"`;R=`tail -n 1 VERSION`;cd ..;\
	tar cvf - $$N|gzip - >$$N.$$R.tgz;

# Usage: make YGROUP=altgrp sharable
# default group is "yorick", affects instally.sh
YGROUP=yorick
sharable:
	@rm -f ysite.grp
	echo "$(YGROUP)" >ysite.grp
	chgrp -R $(YGROUP) .
	chmod -R g+w .
	find . -type d | xargs chmod g+s

dumpconfig:
	@cd yorick; $(MAKE) dumpconfig

# targets for ./configure
echocc:
	echo "$(CC)" >cfg.tmp
echorl:
	echo "$(RANLIB)" >cfg.tmp
echoar:
	echo "$(AR)" >cfg.tmp
echoml:
	echo "$(MATHLIB)" >cfg.tmp
pkgconfig:
	@for d in $(CONFIGDIRS); do ( cd $$d; $(MAKE) config; ); done
