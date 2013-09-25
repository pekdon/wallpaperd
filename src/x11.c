/*
 * x11.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/select.h>
#include <stdio.h>
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
static int XRANDR_EVENT_BASE = 0;
static int XRANDR_ERROR_EVENT_BASE = 0;
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

static struct geometry **x11_get_fake_heads (void);

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
    if (DISPLAY == 0) {
        die ("failed to open display, shutting down!");
    }

    x11_init_atoms ();
    x11_get_desktop_names (1);

#ifdef HAVE_XRANDR
    XRRQueryExtension (DISPLAY, &XRANDR_EVENT_BASE, &XRANDR_ERROR_EVENT_BASE);
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
 * Get number of heads.
 */
unsigned int
x11_get_num_heads (void)
{
    unsigned int num = 1;

#ifdef HAVE_XRANDR
    XRRScreenResources *res = XRRGetScreenResources (DISPLAY,
                                                     x11_get_root_window ());
    if (res) {
        for (int i = 0, num = 0; i < res->noutput; i++) {
            XRROutputInfo *output =
                XRRGetOutputInfo(DISPLAY, res, res->outputs[i]);
            if (output->crtc) {
                num++;
            }
            XRRFreeOutputInfo (output);
        }
        XRRFreeScreenResources (res);
    }
#endif /* HAVE_XRANDR */ 

    return num;
}

/**
 * Get null terminated list of heads, all elements AND the list needs
 * to be freed by the caller.
 */
struct geometry**
x11_get_heads (void)
{
#ifdef HAVE_XRANDR
    XRRScreenResources *res = XRRGetScreenResources (DISPLAY,
                                                     x11_get_root_window ());
    if (! res) {
        fprintf (stderr, "unable to read xrandr screen information.");
        return x11_get_fake_heads ();
    }

    unsigned int num = x11_get_num_heads ();
    unsigned int head = 0;
    struct geometry **heads = mem_new (sizeof (struct geometry*) * (num + 1));

    for (int i = 0; i < res->noutput; ++i) {
        XRROutputInfo *output = XRRGetOutputInfo(DISPLAY, res, res->outputs[i]);
        if (output->crtc) {
            XRRCrtcInfo *crtc = XRRGetCrtcInfo(DISPLAY, res, output->crtc);
            heads[head] = mem_new (sizeof (struct geometry));
            x11_set_geometry_size (heads[head++],
                                   crtc->x, crtc->y, crtc->width, crtc->height);
            XRRFreeCrtcInfo (crtc);
        }
        XRRFreeOutputInfo (output);
    }

    heads[num] = 0;

    XRRFreeScreenResources (res);

    return heads;
#else /* ! HAVE_XRANDR */
    return x11_get_fake_heads ();
#endif /* HAVE_XRANDR */
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
x11_is_xrandr_event (XEvent *ev)
{
#ifdef HAVE_XRANDR
    switch (ev->type - XRANDR_EVENT_BASE) {
    case RRNotify:
        return RRNotify;
    case RRScreenChangeNotify:
        return RRScreenChangeNotify;
    default:
        return 0;
    }
#else /* ! HAVE_XRANDR */
    return 0;
#endif /* HAVE_XRANDR */
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
 * Parse color string and fill in color_ret with color values.
 *
 * Color can be in any format supported by XParseColor which includes
 * #rrggbb, rgb:rr/gg/bb and color names such as white.
 *
 * @return true if color was parsed successfully, else false.
 */
bool
x11_parse_color (const char *color_str, struct color *color_ret)
{
    bool parse_ok;

    XColor color;
    if (XParseColor (DISPLAY, x11_get_colormap (), color_str, &color)) {
        parse_ok = true;
        color_ret->r = color.red >> 8;
        color_ret->g = color.green >> 8;
        color_ret->b = color.blue >> 8;
    } else {
        parse_ok = false;
        color_ret->r = 0;
        color_ret->g = 0;
        color_ret->b = 0;
    }

    return parse_ok;
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
    XSelectInput (DISPLAY, x11_get_root_window (),
                  PropertyChangeMask|
                  StructureNotifyMask|SubstructureNotifyMask);
#ifdef HAVE_XRANDR
    XRRSelectInput (DISPLAY, x11_get_root_window (),
                    RRCrtcChangeNotifyMask|RRScreenChangeNotifyMask);
#endif // HAVE_XRANDR
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

/**
 * Get head information filled in from X11 screen information, used as
 * fallback if XRANDR is missing or fails.
 */
struct geometry**
x11_get_fake_heads (void)
{
    struct geometry **heads = mem_new (sizeof (struct geometry*) * 2);
    heads[0] = x11_get_geometry ();
    heads[1] = 0;
    return heads;
}
