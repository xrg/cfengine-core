CFLAGS = @CFLAGS@  @GCC_CFLAG@
YFLAGS = -d
LEXLIB = @LEXLIB@
LDADD = -L../pub -lpub @BERKELEY_DB_LDFLAGS@ @BERKELEY_DB_LIB@ @OPENSSL_LIB_LDFLAGS@ @OPENSSL_LIB_LIB@
INCLUDES = @BERKELEY_DB_CFLAGS@ @OPENSSL_LIB_CPPFLAGS@
LDFLAGS = @BERKELEY_DB_LDFLAGS@ @OPENSSL_LIB_LDFLAGS@ @LDFLAGS@

sbin_PROGRAMS = cfagent cfservd cfrun cfkey cfenvd cfenvgraph cfexecd

cfagent_SOURCES =							\
	cfagent.c							\
	init.c								\
        do.c                                                            \
        wrapper.c                                                       \
        report.c                                                        \
	client.c							\
	ip.c								\
	process.c							\
	ifconf.c							\
	image.c								\
	item.c								\
	item-ext.c							\
	item-file.c							\
	2Dlist.c							\
	globals.c							\
	classes.c							\
	misc.c								\
	parse.c								\
	functions.c							\
	edittools.c							\
	patches.c							\
	install.c							\
	link.c								\
	tidy.c								\
	filedir.c							\
	eval.c								\
	modes.c								\
	chflags.c							\
	locks.c								\
	mount.c								\
	macro.c								\
	filenames.c							\
	varstring.c							\
	wildcard.c							\
	cfparse.y							\
	comparray.c							\
	read.c								\
	checksums.c							\
	proto.c								\
        filters.c                                                       \
	copy.c								\
	repository.c							\
	rotate.c							\
	errors.c							\
	cflex.l								\
	net.c								\
	df.c								\
	log.c								\
	crypto.c							\
	popen.c								\
	popen_def.c							\
	sensible.c							\
	acl.c								\
	dce_acl.c							\
	nameinfo.c							\
	strategies.c


cfagent_LDADD = $(LEXLIB) @BERKELEY_DB_LIB@ @OPENSSL_LIB_LIB@

cfservd_SOURCES =							\
	cfservd.c							\
	parse.c								\
	functions.c							\
	install.c							\
	ip.c								\
	cfparse.c							\
	chflags.c							\
	cflex.l								\
	item.c								\
	item-ext.c							\
	varstring.c							\
	popen.c								\
	popen_def.c							\
	globals.c							\
	filenames.c							\
	macro.c								\
	eval.c								\
	read.c								\
	classes.c							\
	misc.c								\
	2Dlist.c							\
	modes.c								\
	locks.c								\
	init.c								\
	wildcard.c							\
	copy.c								\
	rotate.c							\
	net.c								\
	acl.c								\
	dce_acl.c							\
	log.c								\
	errors.c							\
	crypto.c							\
        filters.c                                                       \
	patches.c							\
	checksums.c							\
	nameinfo.c							\
	strategies.c

cfservd_LDADD = $(LEXLIB)  @BERKELEY_DB_LIB@ @OPENSSL_LIB_LIB@

cfexecd_SOURCES =							\
	cfexecd.c							\
	popen.c								\
	globals.c							\
	read.c								\
	classes.c							\
	nameinfo.c							\
	wildcard.c							\
	errors.c							\
        patches.c                                                       \
	log.c								\
	locks.c								\
	filenames.c							\
	granules.c							\
	eval.c								\
	item-ext.c							\
	item.c

cfexecd_LDADD = $(LEXLIB)  @BERKELEY_DB_LIB@ @OPENSSL_LIB_LIB@

cfrun_SOURCES =								\
	cfrun.c								\
	item.c								\
	item-ext.c							\
	filenames.c							\
	nameinfo.c							\
	eval.c								\
	wildcard.c							\
	proto.c								\
	popen.c								\
	net.c								\
	ip.c								\
	checksums.c							\
	classes.c							\
	read.c								\
	log.c								\
	crypto.c							\
	patches.c							\
	init.c								\
	granules.c							\
	globals.c

cfkey_SOURCES =								\
	cfkey.c								\
	init.c								\
	filenames.c							\
	crypto.c							\
	popen.c								\
	globals.c							\
	read.c								\
	classes.c							\
	errors.c							\
        patches.c                                                       \
	log.c

cfenvd_SOURCES =							\
	cfenvd.c							\
	globals.c							\
	filenames.c							\
	nameinfo.c							\
	classes.c							\
	item.c								\
	item-ext.c							\
	eval.c								\
	wildcard.c							\
	df.c								\
	read.c								\
	granules.c							\
	log.c								\
	popen.c								\
	patches.c							\
	locks.c		

cfenvgraph_SOURCES =							\
	cfenvgraph.c							\
	granules.c

#
# Pickup header files here so Automake knows about them
#
noinst_HEADERS =							\
	cfservd.h								\
	cfparse.h							\
	cf.defs.h							\
	cf.extern.h                                                     \
	prototypes.h

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig

#
# Get everything removed down to where rebuilding requires:
# "make; make install"
# 
CLEANFILES = cflex.c cfparse.c cfparse.h

#
# Get everything removed down to where rebuilding requires:
# "configure; make; make install"
#
DISTCLEANFILES = cflex.c cfparse.c cfparse.h

#
# Get everything removed down to where rebuilding requires:
# "aclocal; autoconf; autoheader; automake --add-missing"
# "configure; make; make install"
#
MAINTAINERCLEANFILES = Makefile.in conf.h.in stamp-h.in
