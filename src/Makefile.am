# Without this GNU make is required
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = wallpaperd
wallpaperd_SOURCES = \
	cache.c cache.h \
	compat.c compat.h \
	cfg.c cfg.h \
	wallpaper.c wallpaper.h \
	wallpaperd.c wallpaperd.h \
	util.c util.h \
	x11.c x11.h

wallpaperd_LDADD = @xlib_LIBS@ @imlib2_LIBS@

INCLUDES = @xlib_CFLAGS@ @imlib2_CFLAGS@

distclean-local:
	rm -f *\~ .\#*
