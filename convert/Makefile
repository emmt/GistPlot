srcdir=.
DEST_DIR=..
GIST2_DIR=$HOME/tao/NeoGist
TOOLS_DIR=$GIST2_DIR/_tools
MASTER=$(srcdir)/yorick-master

LIST_SOURCES=./$(srcdir)/list-sources.sh
LIST_SYMBOLS=./$(srcdir)/list-symbols.sh

CONVERT_SOURCES = conv-play.lst conv-gist.lst

TARGETS = \
    $(DEST_DIR)/play \
    $(DEST_DIR)/gist \
    $(DEST_DIR)/etc \
    $(DEST_DIR)/configure \
    $(DEST_DIR)/exports.sh \
    $(DEST_DIR)/LICENSE.md \
    $(DEST_DIR)/Makedll \
    $(DEST_DIR)/Makeexe \
    $(DEST_DIR)/Makefile \
    $(DEST_DIR)/Makepkg \
    $(DEST_DIR)/README.md

REFILTER = \
    $(DEST_DIR)/play/unix/Makefile \
    $(DEST_DIR)/play/unix/config.sh \
    $(DEST_DIR)/play/unix/PLUGIN.txt \
    $(DEST_DIR)/play/win/Make.win \
    $(DEST_DIR)/play/win/Makefile

default:

all: $(MASTER) $(TARGETS)

clean:
	rm -f *~

yorick-master: yorick-master.zip
	unzip -a '$<'

yorick-master.zip:
	wget -O '$@' 'https://github.com/dhmunro/yorick/archive/master.zip'

$(DEST_DIR)/configure: $(MASTER)/configure convert.pl Makefile
	perl convert.pl <'$<' >'$@'
	chmod 755 '$@'

$(DEST_DIR)/exports.sh: $(MASTER)/yorapi.sh convert.pl Makefile
	perl convert.pl <'$<' >'$@'
	chmod 755 '$@'

$(DEST_DIR)/LICENSE.md: $(MASTER)/LICENSE.md Makefile
	cp -a '$<' '$@'

$(DEST_DIR)/Make.cfg: $(MASTER)/Make.cfg convert.pl Makefile
	perl convert.pl <'$<' >'$@'

$(DEST_DIR)/Makedll: $(MASTER)/Makedll convert.pl Makefile
	perl convert.pl <'$<' >'$@'

$(DEST_DIR)/Makeexe: $(MASTER)/Makeexe convert.pl Makefile
	perl convert.pl <'$<' >'$@'

$(DEST_DIR)/Makefile: $(MASTER)/Makefile convert.pl Makefile
	perl convert.pl <'$<' >'$@'

$(DEST_DIR)/Makepkg: $(MASTER)/Makepkg convert.pl Makefile
	perl convert.pl <'$<' >'$@'

$(DEST_DIR)/README.md: $(MASTER)/README.md convert.pl Makefile
	perl convert.pl <'$<' >'$@'

$(DEST_DIR)/etc: $(MASTER)/g Makefile
	rm -rf '$@'
	cp -a '$<' '$@'

$(DEST_DIR)/play: $(MASTER)/play convert.pl Makefile
	rm -rf '$@'
	cp -a '$<' '$@'
	mv $@/play.h	 $@/play2.h
	mv $@/pstdlib.h	 $@/std.h
	mv $@/pstdio.h	 $@/io.h
	mv $@/pmin.h	 $@/events.h
	mv $@/phash.h	 $@/hash.h
	mv $@/plugin.h	 $@/extern.h
	mv $@/yversion.h $@/version.h
	for file in `find '$@' -type f`; do \
	    rm -f "$$file.orig"; \
	    mv "$$file" "$$file.orig"; \
	    perl convert.pl <"$$file.orig" >"$$file"; \
	    rm -f "$$file.orig"; \
	done
	for file in `find '$@' -type f -name '*.h'`; do \
	    perl -pi.orig \
	        -e 's,"((play|gist)(2|/[_A-Za-z0-9]+)\.h)",<$$1>,g;' \
	        "$$file"; \
	    rm -f "$$file.orig"; \
	done
	for file in $(REFILTER); do \
	    perl -pi.orig -e 's/\\b(D|)PL_EXPORT\\b/$$1PLUG_API/g;' "$$file"; \
	    rm -f "$$file.orig"; \
	done

$(DEST_DIR)/gist: $(MASTER)/gist convert.pl Makefile
	rm -rf '$@'
	cp -a '$<' '$@'
	mv $@/gist.h $@/gist2.h
	mv $@/gtext.h $@/text.h
	mv $@/gtext.c $@/text.c
	for file in `find '$@' -type f`; do \
	    rm -f "$$file.orig"; \
	    mv "$$file" "$$file.orig"; \
	    perl convert.pl <"$$file.orig" >"$$file"; \
	    rm -f "$$file.orig"; \
	done
	for file in `find '$@' -type f -name '*.h'`; do \
	    perl -pi.orig \
	        -e 's,"((play|gist)(2|/[_A-Za-z0-9]+)\.h)",<$$1>,g;' \
	        "$$file"; \
	    rm -f "$$file.orig"; \
	done

play2-and-gist2-sources.lst.gz: $(LIST_SOURCES)
	(cd "$(GIST_DIR)"; $(LIST_SOURCES) configure Make* play2 gist2 etc) | gzip -9 > '$@'

play2-and-gist2-symbols.lst.gz: $(LIST_SYMBOLS) play2-and-gist2-sources.lst.gz
	(cd "$(GIST_DIR)"; $(LIST_SYMBOLS) `zcat "$(srcdir)/play2-and-gist2-sources.lst.gz"`) | gzip -9 > '$@'

play-and-gist-sources.lst.gz: $(LIST_SOURCES)
	(cd "$(MASTER)"; $(LIST_SOURCES) configure *.sh Make* play gist g) | gzip -9 > '$@'

play-and-gist-symbols.lst.gz: $(LIST_SYMBOLS) play-and-gist-sources.lst.gz
	(cd "$(MASTER)"; $(LIST_SYMBOLS) `zcat "$(srcdir)/play-and-gist-sources.lst.gz"`) | gzip -9 > '$@'

yorick-sources.lst.gz: $(LIST_SOURCES)
	(cd "$(MASTER)"; $(LIST_SOURCES) .) | gzip -9 > '$@'

yorick-symbols.lst.gz: $(LIST_SYMBOLS) yorick-sources.lst.gz
	(cd "$(MASTER)"; $(LIST_SYMBOLS) `zcat "$(srcdir)/yorick-sources.lst.gz"`) | gzip -9 > '$@'

play2_to_play.h: play2_to_play_defines.sed play-and-gist-symbols.lst.gz
	zcat play-and-gist-symbols.lst.gz \
	| sed -f play2_to_play_defines.sed >'$@'

play_to_play2.h: play_to_play2_defines.sed play-and-gist-symbols.lst.gz
	zcat play-and-gist-symbols.lst.gz \
	| sed -f play_to_play2_defines.sed >'$@'

convert.pl: $(CONVERT_SOURCES) Makefile
	echo  >'$@' '#! /usr/bin/perl'
	echo >>'$@' 'use strict;'
	echo >>'$@' 'use warnings;'
	echo >>'$@' 'while (<>) {'
	cat $(CONVERT_SOURCES) \
	| sed -rn \
	    -e 's/#.*//;s/[ \t]+/ /g;s/^ //;s/ $$//' \
	    -e '/^[^ ]+ [^ ]+$$/p' \
	| sort -r \
	| sed -r >>'$@' \
	    -e 's/([^ ]+) ([^ ]+)$$/    s,\\b\1\\b,\2,g;/'
	echo >>'$@' '    s,\\b((play|gist)/)\1,$$1,g;'
	echo >>'$@' '    s/\\bp_([_A-Za-z0-9])/pl_$$1/g;'
	echo >>'$@' '    s/\\bP_([_A-Za-z0-9])/PL_$$1/g;'
	echo >>'$@' '    s/\\bstruct +([_A-Za-z0-9]+)_t\\b/struct _$$1/g;'
	echo >>'$@' '    s/\\bgtext\\./text./g;'
	echo >>'$@' '    s/ +$$//;'
	echo >>'$@' '    print $$_;'
	echo >>'$@' '}'
	chmod 755 '$@'
