dnl init autoconf / automake
AC_INIT(wallpaperd, 0.1.1)
AC_CONFIG_SRCDIR(src/wallpaperd.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl search for programs
AC_PROG_CC_C99
AC_PROG_RANLIB
AC_PROG_INSTALL

dnl search for headers
AC_STDC_HEADERS
AC_HAVE_HEADERS(sys/types.h sys/stat.h unistd.h)

dnl check required libraries
PKG_CHECK_MODULES([imlib2], [imlib2 >= 1.0.0], HAVE_IMLIB2=yes, HAVE_IMLIB2=no)
if test "x$HAVE_IMLIB2" = "xyes"; then
   AC_DEFINE(HAVE_IMLIB2, [1], [define to 1 if you have imlib2])
else
   AC_MSG_ERROR([Could not find imlib2 >= 1.0.0, required dependency.])
fi

dnl output makefiles
AC_OUTPUT(Makefile src/Makefile)
