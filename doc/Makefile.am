man_MANS = cfengine.8
info_TEXINFOS = cfengine-Reference.texinfo cfengine-Tutorial.texinfo cfengine-Anomalies.texinfo

docdir = $(prefix)/doc/$(PACKAGE)-$(VERSION)
doc_DATA =
# Info used in building and installing HTML files
htmldir = $(pkgdatadir)/html
htmlfiles = cfengine-Reference.html cfengine-Tutorial.html 
html_DATA = $(htmlfiles)

# Info used in building and installing Postscript files
ps1 = 
ps2 = cfengine-Tutorial.ps cfengine-Reference.ps

# When the Texi2DVI problem with cfengine-Reference is corrected, 
# then uncomment the reference to ps2
psfiles = $(ps1)	# $(ps2)

psdir = $(pkgdatadir)/doc
ps_DATA = $(psfiles)

# Make sure these get distributed with everything else.
EXTRA_DIST = cfengine.8 texinfo.tex $(htmlfiles)
SUFFIXES = html latex

%.html : %.texinfo
	${MAKEINFO} -I $(srcdir) --html $<

%.dvi : %.latex
	${LATEX} $<

%.dvi : %.texinfo
	${TEXI2DVI} $<

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
