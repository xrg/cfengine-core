man_MANS = cfengine.8 cfagent.8  cfenvd.8  cfenvgraph.8  cfexecd.8  cfkey.8  cfrun.8  cfservd.8 cfshow.8

info_TEXINFOS = cfengine-Reference.texinfo cfengine-Tutorial.texinfo cfengine-Anomalies.texinfo

docdir = $(prefix)/doc/$(PACKAGE)-$(VERSION)
doc_DATA =

# Info used in building and installing HTML files
htmldir = $(pkgdatadir)/html
htmlfiles = cfengine-Reference.html cfengine-Tutorial.html cfengine-Anomalies.html
html_DATA = $(htmlfiles)

# Info used in building and installing Postscript files

psfiles = cfengine-Tutorial.ps cfengine-Reference.ps cfengine-Anomalies.ps
pdffiles = cfengine-Tutorial.pdf cfengine-Reference.pdf cfengine-Anomalies.pdf

psdir = $(pkgdatadir)/doc
ps_DATA = $(psfiles) $(pdffiles)

MAKEINFOHTML = $(MAKEINFO) --html --no-split

# Make sure these get distributed with everything else.

EXTRA_DIST = cfagent.8 cfenvd.8 cfenvgraph.8 cfexecd.8 cfkey.8 cfrun.8 cfservd.8 cfengine.8 cfshow.8 texinfo.tex $(htmlfiles) $(infofiles)  $(psfiles) $(dvifiles) $(pdffiles)

SUFFIXES = html latex pdf ps dvi

#%.html : %.texinfo
#	${MAKEINFO} -I $(srcdir) --html --no-split $<
#
#%.dvi : %.latex
#	${LATEX} $<

#%.dvi : %.texinfo
#	${TEXI2DVI} $<

#%.pdf : %.ps
#	${PS2PDF} $<

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig

#
# Get everything removed down to where rebuilding requires:
# "make; make install"
# 
CLEANFILES = $(htmlfiles) $(psfiles)

#
# Get everything removed down to where rebuilding requires:
# "configure; make; make install"
#
DISTCLEANFILES = *.log *.aux *.mbs *.mb *.info*

#
# Get everything removed down to where rebuilding requires:
# "aclocal; autoconf; autoheader; automake --add-missing"
# "configure; make; make install"
#
MAINTAINERCLEANFILES = Makefile.in mdate-sh
