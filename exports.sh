#!/bin/sh
# $Id: exports.sh,v 1.3 2006-02-02 09:17:06 thiebaut Exp $
# MSWindows and AIX both require lists of all symbols declared as PL_EXTERN
# in order to properly link plugins.
#
# MSWindows - when plugin built, need API list to resolve symbols
#             defined in the yorick executable
# AIX - when yorick built, need API list to mark symbols which the
#       yorick executable exports

PLAY_DIRS=`grep '^PLAY_DIRS=' Make.cfg | sed -e 's/^PLAY_DIRS=//'`
if test "$PLAY_DIRS" = "win"; then
  WIN=win
else
  WIN=x11
fi

rm -f cfg.* exports.def
# header files containing PL_EXTERN declarations
grep -h PL_EXTERN >cfg.00 \
 play/play2.h play/hash.h play/std.h play/io.h play/$WIN/playwin.h \
 gist/gist2.h gist/hlevel.h gist/draw.h gist/engine.h gist/cgm.h \
 gist/ps.h gist/text.h gist/xbasic.h gist/xfancy.h

# splitcmd=`echo one,two,three|sed -e 'y/,/\n/'|wc -l`
# in AIX sed, y recognizes \n but s does not
# splitcmd='y/,/\n/'
# in Linux sed, s recognizes \n but y does not
# splitcmd='s/,/\n/g'
# but both recognize escaped newline in 4th line from bottom in this script:

cat >cfg.01 <<EOF
s/;.*/;/
s/[ 	]*([^*].*)/:/
s/([^*][^)]*/:/
s/)://
s/\[.*\]//
s/;//
s/: *,/:/
s/PL_EXTERN //
s/volatile //
s/const //
s/unsigned //
s/struct //
s/(\*//
s/^[ 	]*[a-zA-Z0-9_]*[ 	]*//
s/\**//g
s/[ 	]*,[ 	]*/,/g
s/,/\\
/g
s/://g
s/[ 	]*//g
EOF

sed -f cfg.01 cfg.00 >exports.def
rm -f cfg.*

# original MSWindows script
# dlltool -z preyor.def libyor.a
# sed -e 's/ @ .*//' -e 's/	//' <preyor.def | tail -n +3 >preyor.def1
# sed -e 's/.*/\0 = yorick.exe.\0/' <preyor.def1 >libyor.def
