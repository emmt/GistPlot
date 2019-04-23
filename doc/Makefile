# you need the makeinfo and texi2dvi programs from gnu.org
# and the tex typesetting system from tug.org
# For the html-doc target, you need a recent texi2html that accepts
# "--split node". Mac users: The one in fink (1.64) is too old, get one from
# http://www.nongnu.org/texi2html/
include ../Make.cfg
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi
#TEXI2PDF = texi2pdf
TEXI2PDF = texi2dvi --pdf
TEX = tex
PDFTEX = pdftex

all: docs

docs:

ngist.info: ngist.tex
	$(MAKEINFO) ngist.tex

ngist.dvi: ngist.tex
	$(TEXI2DVI) ngist.tex

ngist.pdf: ngist.tex
	$(TEXI2PDF) ngist.tex

refs.dvi: refs.tex
	$(TEX) refs.tex

refs.pdf: refs.tex
	$(PDFTEX) refs.tex

html-doc:
	cd html; make html-doc

TEX_AUX=*.aux *.log *.toc *.cp *.fn *.ky *.pg *.tp *.vr *.tmp
clean::
	rm -f *~ '#'* *.inf* *.dvi *.doc $(TEX_AUX)
	rm -Rf ngist/
	if test -d html; then cd html; make clean; fi

distclean:: clean

.PHOONY: all docs clean distclean
