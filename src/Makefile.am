bin_PROGRAMS = flac123

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
