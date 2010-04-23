/*
 * x11.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include <sys/select.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "util.h"
#include "x11.h"

static Display *DISPLAY = 0;

Atom ATOM_DESKTOP = 0;
Atom ATOM_DESKTOP_NAMES = 0;
Atom ATOM_ROOTPMAP_ID = 0;

static void x11_init_atoms (void);
static void x11_set_geometry_size(struct geometry *geometry, int x, int y,
                                  unsigned int width, unsigned int height);

/**
 * Open a connection to the X11 display if not already open.
 */
int
x11_open_display (void)
{
    if (DISPLAY != 0) {
        return 1;
    }

    DISPLAY = XOpenDisplay (0);
    x11_init_atoms ();

    return 1;
}

/**
 * Initialize atoms after opening the display.
 */
void
x11_init_atoms (void)
{
    ATOM_DESKTOP = x11_get_atom ("_NET_CURRENT_DESKTOP");
    ATOM_DESKTOP_NAMES = x11_get_atom ("_NET_DESKTOP_NAMES");
    ATOM_ROOTPMAP_ID =  x11_get_atom ("_XROOTPMAP_ID");
}

/**
 * Close connection to the X11 display if open.
 */
void
x11_close_display (void)
{
    if (DISPLAY == 0) {
        return;
    }

    XCloseDisplay (DISPLAY);
    DISPLAY = 0;
}

/**
 * Return the X11 display handle.
 */
Display*
x11_get_display (void)
{
    return DISPLAY;
}

/**
 * Return the X11 root Window.
 */
Window
x11_get_root_window (void)
{
    return DefaultRootWindow (DISPLAY);
}

/**
 * Return the Visual for the current display.
 */
Visual*
x11_get_visual (void)
{
    return DefaultVisual (DISPLAY, DefaultScreen (DISPLAY));
}

/**
 * Return the Colormap for the current display.
 */
Window
x11_get_colormap (void)
{
    return DefaultColormap (DISPLAY, DefaultScreen (DISPLAY));
}

/**
 * Return an array with head geometries, the first head is the
 * combined geometry of the display and the last entry is signalled
 * with width/height 0.
 */
struct geometry*
x11_get_geometry (void)
{
    struct geometry *geometry = mem_new (sizeof(struct geometry));

    Screen *screen = ScreenOfDisplay (DISPLAY, DefaultScreen (DISPLAY));

    x11_set_geometry_size (geometry, 0, 0,
                           WidthOfScreen(screen), HeightOfScreen(screen));

    return geometry;
}

/**
 * Update geometry.
 */
void
x11_set_geometry_size(struct geometry *geometry, int x, int y,
                      unsigned int width, unsigned int height)
{
    geometry->x = x;
    geometry->y = y;
    geometry->width = width;
    geometry->height = height;
}

/**
 * Return XRANDR event.
 */
Atom
x11_get_xrandr_event (void)
{
    return 0;
}

/**
 * Set the background Pixmap of Window.
 */
void
x11_set_background_pixmap (Window window, Pixmap pixmap)
{
    XSetWindowBackgroundPixmap (DISPLAY, window, pixmap);
    XClearWindow (DISPLAY, window);
}

/**
 * Select input on the root window.
 */
void
x11_init_event_listeners (void)
{
    XSelectInput (DISPLAY, x11_get_root_window (), PropertyChangeMask);
}

/**
 * Get the next event, return 1 if event was retrived before interrupted.
 */
int
x11_next_event (XEvent *ev)
{
    if (XPending (DISPLAY) > 0) {
        XNextEvent (DISPLAY, ev);
        return 1;
    }
    XFlush (DISPLAY);

    int fd = ConnectionNumber (DISPLAY);
    fd_set rfds;
    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);

    int ret = select (fd + 1, &rfds, 0, 0, 0);
    if (ret > 0) {
        XNextEvent (DISPLAY, ev);
    }

    return ret > 0;
}

/**
 * Get X11 atom from name.
 */
Atom
x11_get_atom (const char *atom_name)
{
    return XInternAtom (DISPLAY, atom_name, False);
}

/**
 * Read atom.
 */
Bool
x11_get_atom_value (Window window, Atom atom, Atom type,
                    unsigned long expected, unsigned char **data,
                    unsigned long *actual)
{
    Atom r_type;
    int r_format;
    unsigned long read, left;
    int status = XGetWindowProperty (DISPLAY, window, atom,
                                     0L, expected, False, type,
                                     &r_type, &r_format, &read, &left, data);

    if (status != Success || type != r_type || read == 0) {
        if (*data != 0) {
            XFree (*data);
            *data = 0;
        }
    }

    return (*data != 0);
}

/**
 * Read long atom.
 */
long
x11_get_atom_value_long (Window window, Atom atom)
{
    long value = -1;
    unsigned char *data = 0;

    if (x11_get_atom_value (window, atom, XA_CARDINAL, 1L, &data, 0)) {
        value = *((long*) data);
        XFree (data);
    }

    return value;
}

/**
 * Set long value atom of format.
 */
void
x11_set_atom_value_long (Window window, Atom atom, long format, long value)
{
    XChangeProperty (DISPLAY, window, atom, format, 32,
                     PropModeReplace, (unsigned char*) &value, 1);
}
