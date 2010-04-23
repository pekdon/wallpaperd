
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "util.h"
#include "x11.h"

static Display *DISPLAY = 0;

static void x11_set_geometry_size(struct geometry *geometry, int x, int y,
                                  unsigned int width, unsigned int height);

/**
 * Open a connection to the X11 display if not already open.
 */
void
x11_open_display (void)
{
    if (DISPLAY != 0) {
        return;
    }

    DISPLAY = XOpenDisplay (0);
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
 * Get the next event.
 */
int
x11_next_event (XEvent *ev)
{
    return XNextEvent (DISPLAY, ev);
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
