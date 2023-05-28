bin_PROGRAMS = flac123

dist_man_MANS = flac123.1

flac123_SOURCES = \
	flac123.h \
	flac123.c \
	remote.c \
	version.h \
	vorbiscomment.c

flac123_LDADD = @FLAC_LIBS@ @POPT_LIBS@ @AO_LIBS@

clobber: distclean
	rm -fr autom4te.cache *~

spotless: clobber
	rm -fr configure Makefile.in
