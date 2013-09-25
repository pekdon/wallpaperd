/*
 * x11.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _X11_H_
#define _X11_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <X11/Xlib.h>
#include <stdbool.h>

/**
 * Geometry specification (list).
 */
struct geometry {
    int x;
    int y;
    int width;
    int height;

    struct geometry *next;
};

/**
 * Single RGB color specification.
 */
struct color {
    char r;
    char g;
    char b;
};

extern Atom ATOM_DESKTOP;
extern Atom ATOM_DESKTOP_NAMES;
extern Atom ATOM_ROOTPMAP_ID;
extern Atom ATOM_UTF8_STRING;

extern int x11_open_display (void);
extern void x11_close_display (void);

extern Display *x11_get_display (void);
extern Window x11_get_root_window (void);
extern Display *x11_get_display (void);
extern Visual *x11_get_visual (void);
extern Colormap x11_get_colormap (void);
extern struct geometry *x11_get_geometry (void);
extern struct geometry **x11_get_heads (void);
extern unsigned int x11_get_num_heads (void);

extern bool x11_parse_color (const char *color_str, struct color *color_ret);

extern void x11_set_background_pixmap (Window window, Pixmap pixmap);

extern void x11_init_event_listeners (void);
extern int x11_next_event (XEvent *ev, int timeout);
extern int x11_is_xrandr_event (XEvent *ev);
extern const char *x11_get_desktop_name (int desktop);
extern char **x11_get_desktop_names (int do_refresh);
extern Atom x11_get_atom (const char *atom_name);
extern long x11_get_atom_value_long (Window window, Atom atom);
extern void x11_set_atom_value_long (Window window, Atom atom,
                                     long format, long value);

#endif /* _X11_H_ */
