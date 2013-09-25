# Without this GNU make is required
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = wallpaperd
wallpaperd_SOURCES = \
	background.c background.h \
	background_xml.c background_xml.h \
	cache.c cache.h \
	compat.c compat.h \
	cfg.c cfg.h \
	main.c wallpaperd.h \
	render.c render.h \
	wallpaper.c wallpaper.h \
	wallpaper_match.c wallpaper_match.h \
	util.c util.h \
	x11.c x11.h

wallpaperd_LDADD = @xlib_LIBS@ @xrandr_LIBS@ @imlib2_LIBS@

AM_CPPFLAGS = @xlib_CFLAGS@ @xrandr_CFLAGS@ @imlib2_CFLAGS@

distclean-local:
	rm -f *\~ .\#*
