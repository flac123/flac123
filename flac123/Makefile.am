bin_PROGRAMS = flac123

flac123_SOURCES = \
	flac123.h \
	flac123.c \
	remote.c \
	version.h \
	vorbiscomment.c

flac123_LDADD = @FLAC_LIBS@ @POPT_LIBS@ @AO_LIBS@

EXTRA_DIST = README.remote BUGS reconf

##man_MANS = flac123.1

dist-hook:
	-for i in `find $(distdir) -name ".svn" -print`; do rm -r $$i; done
