
#ifndef _X11_H_
#define _X11_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <X11/Xlib.h>

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

extern void x11_open_display (void);
extern void x11_close_display (void);

extern Display *x11_get_display (void);
extern Window x11_get_root_window (void);
extern Display *x11_get_display (void);
extern Visual *x11_get_visual (void);
extern Colormap x11_get_colormap (void);
extern struct geometry *x11_get_geometry (void);

extern void x11_set_background_pixmap (Window window, Pixmap pixmap);

extern void x11_init_event_listeners (void);
extern int x11_next_event (XEvent *ev);
extern Atom x11_get_xrandr_event (void);
extern Atom x11_get_atom (const char *atom_name);
extern long x11_get_atom_value_long (Window window, Atom atom);

#endif /* _X11_H_ */
