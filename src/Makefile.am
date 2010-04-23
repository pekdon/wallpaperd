# Without this GNU make is required
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = wallpaperd
wallpaperd_SOURCES = \
	cache.c cache.h \
	cfg.c cfg.h \
	wallpaper.c wallpaper.h \
	wallpaperd.c wallpaperd.h \
	util.c util.h \
	x11.c x11.h

wallpaperd_LDADD = @imlib2_LIBS@

INCLUDES = @imlib2_CFLAGS@

distclean-local:
	rm -f *\~ .\#*
