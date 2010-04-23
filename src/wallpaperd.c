
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include <X11/Xlib.h>

#include "cfg.h"
#include "wallpaper.h"
#include "wallpaperd.h"
#include "util.h"
#include "x11.h"

static void main_loop (void);
static void handle_property_event (XEvent *ev);
static void handle_xrandr_event (XEvent *ev);

static void set_wallpaper_for_current_desktop (void);

static Atom DESKTOP_ATOM;
static struct config *CONFIG;

/**
 * Main routine, parse configuration and start listening for desktop
 * changes.
 */
int
main (int argc, char **argv)
{
    char *cfg_path = cfg_get_path ();
    CONFIG = cfg_load (cfg_path);

    x11_open_display ();
    x11_init_event_listeners ();

    DESKTOP_ATOM = x11_get_atom ("_NET_CURRENT_DESKTOP");

    set_wallpaper_for_current_desktop ();
    main_loop ();

    x11_close_display ();
    mem_free (cfg_path);

    return 0;
}

/**
 * Main loop reading X11 events, updating wallpaper on desktop change.
 */
void
main_loop (void)
{
    XEvent ev;
    while (! x11_next_event (&ev)) {
        if (ev.type == PropertyNotify) {
            handle_property_event (&ev);
        } else if (ev.type == x11_get_xrandr_event ()) {
            handle_xrandr_event (&ev);
        }
    }
}

/**
 * Handle property event, detects desktop changes.
 */
void
handle_property_event (XEvent *ev)
{
    if (ev->xproperty.window != x11_get_root_window ()
        || ev->xproperty.atom != DESKTOP_ATOM) {
        return;
    }

    set_wallpaper_for_current_desktop ();
}

/**
 * Handle xrandr events, this invalidates the cache and re-sets the
 * background image.
 */
void
handle_xrandr_event (XEvent *ev)
{
    wallpaper_cache_clear ();
    set_wallpaper_for_current_desktop ();
}

/**
 * Set background image for current desktop.
 */
void
set_wallpaper_for_current_desktop (void)
{
    long current_desktop = x11_get_atom_value_long (x11_get_root_window (),
                                                    DESKTOP_ATOM);

    const char *path = cfg_get_wallpaper (CONFIG, current_desktop);
    const char *mode_str = cfg_get_mode (CONFIG, current_desktop);
    enum wallpaper_mode mode = wallpaper_mode_from_str (mode_str);

    if (path) {
        wallpaper_set (path, mode);
    }
}
