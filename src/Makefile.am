AM_CFLAGS = $(GCC_CFLAG) @CFLAGS@  
AM_YFLAGS = -d
LEXLIB = @LEXLIB@
LDADD = -L../pub -lpub -L. -lcfengine $(BERKELEY_DB_LDFLAGSD) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LDFLAGS) $(OPENSSL_LIB_LIB)
INCLUDES = $(BERKELEY_DB_CFLAGS) $(OPENSSL_LIB_CPPFLAGS)
AM_LDFLAGS = $(BERKELEY_DB_LDFLAGS) $(OPENSSL_LIB_LDFLAGS) @LDFLAGS@

sbin_PROGRAMS = cfagent cfservd cfrun cfkey cfenvd cfenvgraph cfexecd cfshow cfetool cfetoolgraph


#
# Fix this generically
#

inst_LIBRARIES = libcfengine.a
instdir = /usr/local/lib

# Separate out a library

libcfengine_a_SOURCES =							\
	init.c								\
	modules.c							\
	alerts.c							\
        wrapper.c                                                       \
        report.c                                                        \
	client.c							\
	ip.c								\
	ipname.c							\
	instrument.c							\
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
	sockaddr.c							\
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
	strategies.c                         				\
	granules.c							\
	methods.c							\
	dtypes.c							\
	state.c								\
	expand-files.c							\
	expand-tidy.c							\
	expand-image.c							\
	expand-editfiles.c						\
	checksum_db.c							\
	package.c							\
	scli.c

cfshow_SOURCES =							\
	cfshow.c

#cfshow_LDADD = $(LEXLIB) $(LEXLIB) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LIB)

cfagent_SOURCES =							\
	cfagent.c							\
        do.c

#cfagent_LDADD = $(LEXLIB) $(LEXLIB) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LIB)


cfservd_SOURCES =							\
	cfservd.c

#cfservd_LDADD = $(LEXLIB) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LIB)

cfexecd_SOURCES =							\
	cfexecd.c

#cfexecd_LDADD = $(LEXLIB) $(BERKELEY_DB_LIB) $(OPENSSL_LIB_LIB)

cfrun_SOURCES =								\
	cfrun.c

cfkey_SOURCES =								\
	cfkey.c

cfenvd_SOURCES =							\
	cfenvd.c

cfenvgraph_SOURCES =							\
	cfenvgraph.c

cfetool_SOURCES =							\
	cfetool.c							\
	globals.c							\
	filenames.c							\
	nameinfo.c							\
	sockaddr.c							\
	classes.c							\
	item.c								\
	item-ext.c							\
	item-file.c							\
	eval.c								\
	wildcard.c							\
	df.c								\
	read.c								\
	granules.c							\
	log.c								\
	popen.c								\
	patches.c							\
	locks.c								\
	dtypes.c							\
	state.c								\
	init.c

cfetoolgraph_SOURCES =							\
	cfetoolgraph.c							\
	granules.c

#
# Pickup header files here so Automake knows about them
#
noinst_HEADERS =							\
	cfservd.h							\
	cfparse.h							\
	cf.defs.h							\
	cf.extern.h                                                     \
	prototypes.h							\
	cfetooldefs.h

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig *.rej

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
