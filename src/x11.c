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

#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif /* HAVE_XRANDR */

#include "util.h"
#include "x11.h"
#include "cache.h"

static Display *DISPLAY = 0;
static int XRANDR_EVENT = 0;
static int XRANDR_ERROR_EVENT = 0;
static char **DESKTOP_NAMES = 0;

Atom ATOM_DESKTOP = 0;
Atom ATOM_DESKTOP_NAMES = 0;
Atom ATOM_ROOTPMAP_ID = 0;
Atom ATOM_UTF8_STRING = 0;

static void x11_init_atoms (void);
static void x11_set_geometry_size(struct geometry *geometry, int x, int y,
                                  unsigned int width, unsigned int height);
static Bool x11_get_atom_value (Window window, Atom atom, Atom type,
                                unsigned long expected, unsigned char **data,
                                unsigned long *actual);

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
    x11_get_desktop_names (1);

#ifdef HAVE_XRANDR
    XRRQueryExtension (DISPLAY, &XRANDR_EVENT, &XRANDR_ERROR_EVENT);
#endif /* HAVE_XRANDR */
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
    ATOM_ROOTPMAP_ID = x11_get_atom ("_XROOTPMAP_ID");
    ATOM_UTF8_STRING = x11_get_atom("UTF8_STRING");
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
 * Free resources used by desktop names.
 */
void
x11_free_desktop_names ()
{
    if (! DESKTOP_NAMES) {
        return;
    }

    for (int i = 0; DESKTOP_NAMES[i] != 0; i++) {
        mem_free (DESKTOP_NAMES[i]);
    }
    mem_free (DESKTOP_NAMES);
    DESKTOP_NAMES = 0;
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
 * combined geometry of the display and the last entry is identified
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
int
x11_get_xrandr_event (void)
{
    return XRANDR_EVENT;
}

const char*
x11_get_desktop_name (int desktop)
{
    char **desktop_names = x11_get_desktop_names (0);
    if (desktop_names) {
        for (int i = 0; desktop_names[i] != 0; i++) {
            if (i == desktop) {
                return desktop_names[i];
            }
        }
    }
    return 0;
}

/**
 * Return null terminated array of desktop names.
 */
char**
x11_get_desktop_names (int do_refresh)
{
    if (! DESKTOP_NAMES || do_refresh) {
        x11_free_desktop_names ();

        unsigned char *data;
        unsigned long data_length;
        if (x11_get_atom_value (x11_get_root_window(), ATOM_DESKTOP_NAMES,
                                ATOM_UTF8_STRING, 256, &data, &data_length)) {
            char *p;
            unsigned long i, j, num;
            for (p = (char*) data, i = 0, num = 0; i < data_length; num++) {
                i += strlen (p) + 1;
                p += strlen (p) + 1;
            }

            DESKTOP_NAMES = mem_new (sizeof (char*) * (num + 1));
            for (p = (char*) data, i = 0, j = 0; i < data_length; j++) {
                DESKTOP_NAMES[j] = str_dup (p);
                i += strlen (p) + 1;
                p += strlen (p) + 1;
            }
            DESKTOP_NAMES[num] = 0;
        }
    }
    
    return DESKTOP_NAMES;
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
x11_next_event (XEvent *ev, int timeout)
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

    struct timeval timeout_tv = { timeout, 0 };

    int ret = select (fd + 1, &rfds, 0, 0, timeout > 0 ? &timeout_tv : 0);
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
 * XFree data and set data pointer to 0.
 */
void
x11_free (unsigned char **data)
{
    if (*data) {
        XFree (*data);
        *data = 0;
    }
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
    int r_format, status;
    unsigned long left = 0;

    *data = 0;
    do {
        x11_free (data);
        expected += left;

        status = XGetWindowProperty(DISPLAY, window, atom,
                                    0L, expected, False, type,
                                    &r_type, &r_format, actual, &left, data);

        if (status != Success || type != r_type || *actual == 0) {
            x11_free (data);
            left = 0;
        }
    } while (left);

    return (*data != 0);
}

/**
 * Read long atom.
 */
long
x11_get_atom_value_long (Window window, Atom atom)
{
    long value = -1;
    unsigned long actual;
    unsigned char *data = 0;

    if (x11_get_atom_value (window, atom, XA_CARDINAL, 1L, &data, &actual)) {
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
