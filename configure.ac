dnl init autoconf / automake
AC_INIT(wallpaperd, 0.2.2)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl search for programs
AC_PROG_CC_C99
AC_PROG_RANLIB
AC_PROG_INSTALL

dnl search for headers
AC_STDC_HEADERS
AC_HAVE_HEADERS(sys/types.h sys/stat.h unistd.h)

dnl add x11 to the env
AC_PATH_X
AC_PATH_XTRA
if test "x$no_x" = "xyes"; then
    AC_MSG_ERROR([Could not find Xlib, required dependency.])
fi

if ! test -z "$x_includes"; then
    xlib_CFLAGS="-I$x_includes"
else
    xlib_CFLAGS=""
fi
if ! test -z "$x_libraries"; then
    xlib_LIBS="-L$x_libraries -lX11"
else
    xlib_LIBS="-lX11"
fi

AC_SUBST(xlib_CFLAGS)
AC_SUBST(xlib_LIBS)

dnl check for xrandr
PKG_CHECK_MODULES([xrandr], [xrandr >= 1.2.0], HAVE_XRANDR=yes, HAVE_XRANDR=no)
if test "x$HAVE_XRANDR" = "xyes"; then
    AC_DEFINE(HAVE_XRANDR, [1], [Define to 1 if you have an XRANDR capable client])
fi

dnl check for daemon call
AC_CHECK_FUNC(daemon, [AC_DEFINE(HAVE_DAEMON, [1], [define to 1 if you daemon call in the C library])])

dnl check required libraries
PKG_CHECK_MODULES([imlib2], [imlib2 >= 1.0.0], HAVE_IMLIB2=yes, HAVE_IMLIB2=no)
if test "x$HAVE_IMLIB2" = "xyes"; then
   AC_DEFINE(HAVE_IMLIB2, [1], [define to 1 if you have imlib2])
else
   AC_MSG_ERROR([Could not find imlib2 >= 1.0.0, required dependency.])
fi

dnl output makefiles
AC_OUTPUT(Makefile src/Makefile)
