# customized configure.ac
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([flac123], m4_translit(m4_esyscmd_s([grep FLAC123_VERSION src/version.h | awk '{print $3}']), `"'), [https://github.com/flac123])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/flac123.c])
AC_REVISION($Id$)
AC_PREFIX_DEFAULT(/usr/local)

# Checks for specific systems.
system_name=`( uname -s ) 2>&1`
case "$system_name" in
     Linux)
	AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "pulse",[Define the default libao output device.])
	;;
     Darwin)
	AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "macosx",[Define the default libao output device.])
	AC_DEFINE(DARWIN)
	if test -d /sw ; then
	   # Fink needs /sw/include and /sw/lib
	   CFLAGS="$CFLAGS -I/sw/include"
	   LDFLAGS="$LDFLAGS -L/sw/lib"
	   with_ao_prefix="/sw"
	elif test -d /opt/homebrew ; then
           # brew needs /opt/homebrew/include and /opt/homebrew/lib
           CFLAGS="$CFLAGS -I/opt/homebrew/include"
           LDFLAGS="$LDFLAGS -L/opt/homebrew/lib"
           with_ao_prefix="/opt/homebrew"
	elif test -d /usr/local ; then
           # brew needs /usr/local/include and /usr/local/lib
           CFLAGS="$CFLAGS -I/usr/local/include"
           LDFLAGS="$LDFLAGS -L/usr/local/lib"
           with_ao_prefix="/usr/local"
	elif test -d /opt/local ; then
	   # Macports needs /opt/local/include and /opt/local/lib
	   CFLAGS="$CFLAGS -I/opt/local/include"
	   LDFLAGS="$LDFLAGS -L/opt/local/lib"
	   with_ao_prefix="/opt/local"
	fi
	;;
     FreeBSD)
	AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "oss",[Define the default libao output device.])
	# FreeBSD gcc needs /usr/local/
	CFLAGS="$CFLAGS -I/usr/local/include"
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
	;;
     CYGWIN*)
	AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "wmm",[Define the default libao output device.])
	;;
     *)
	AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "oss",[Define the default libao output device.])
	;;
esac

# let the user override the default libao output device
AC_ARG_WITH(default_audio,
[  --with-default-audio=string   Specify default libao output plugin [[pulse, macosx, oss, etc.]] ])

if test "x$with_default_audio" != "x" -a "x$with_default_audio" != "xyes"; then
   AC_DEFINE_UNQUOTED(AUDIO_DEFAULT, "$with_default_audio",[Define the default libao output device.])
fi

AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(FLAC, FLAC__stream_decoder_new, [haveflac=yes], [haveflac=no], -lm)
if test "$haveflac" = "yes"; then
	FLAC_LIBS="-lFLAC -logg -lm"
	AC_SUBST(FLAC_LIBS)
else
	AC_MSG_ERROR(FLAC required!)
fi

AC_CHECK_LIB(popt, poptGetContext, [havepopt=yes])
if test "$havepopt" = "yes"; then
	POPT_LIBS="-lpopt"
	AC_SUBST(POPT_LIBS)
else
	AC_MSG_ERROR(popt required!)
fi

AM_PATH_AO(,AC_MSG_ERROR(libao required!))

# Checks for header files.  None at this time.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# end of checks

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
