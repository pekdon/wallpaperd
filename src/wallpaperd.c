/*
 * wallpaperd.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "cfg.h"
#include "compat.h"
#include "wallpaper.h"
#include "wallpaperd.h"
#include "util.h"
#include "x11.h"

static void parse_options (int argc, char **argv, struct options *options);
static void usage (const char *name);
static void do_start (void);
static void do_stop (int do_output);
static void do_stop_daemon (pid_t pid, int do_output);
static void do_reload (void);
static void do_daemon (void);

static pid_t get_pid_from_pid_file (void);
static void clean_pid_file (void);

static void main_loop (void);
static void handle_property_event (XEvent *ev);
static void handle_xrandr_event (XEvent *ev);

static void set_wallpaper_for_current_desktop (void);

static char *find_wallpaper (const char *name);

static int do_reload_flag = 0;
static int do_shutdown_flag = 0;

struct options *OPTIONS = 0;
struct config *CONFIG = 0;

/**
 * Handler for HUP signal, set reload flag.
 */
void
sighandler_hup (int signal)
{
    do_reload_flag = 1;
}

/**
 * Handler for INT signal, set shutdown flag.
 */
void
sighandler_int (int signal)
{
    do_shutdown_flag = 1;
}

/**
 * Parse command line options.
 */
void
parse_options (int argc, char **argv, struct options *options)
{
    options->help = 0;
    options->foreground = 0;
    options->stop = 0;

    int opt;
    while ((opt = getopt (argc, argv, "hfs")) != -1) {
        switch (opt) {

        case 'f':
            options->foreground = 1;
            break;
        case 's':
            options->stop = 1;
            break;
        case 'h':
        case '?':
        default:
            options->help = 1;
            break;
        }
    }
}

/**
 * Print usage information and exit.
 */
void
usage (const char *name)
{
    fprintf (stderr, "usage: %s [-fsh]\n", name);
    fprintf (stderr, "\n");
    fprintf (stderr, "  -f foreground    do not go into background\n");
    fprintf (stderr, "  -h help          print help information\n");
    fprintf (stderr, "  -s stop          stop running daemon\n");
    fprintf (stderr, "\n");
    exit (1);
}

/**
 * Main routine, parse configuration and start listening for desktop
 * changes.
 */
int
main (int argc, char **argv)
{
    struct options options;
    OPTIONS = &options;
    CONFIG = cfg_new ();

    parse_options (argc, argv, OPTIONS);
    if (OPTIONS->help) {
        usage (argv[0]);
    }

    if (OPTIONS->stop) {
        do_stop (1);
    } else {
        do_start ();
    }

    cfg_free (CONFIG);

    return 0;
}

/**
 * Start wallpaper daemon.
 */
void
do_start (void)
{
    char *cfg_path = cfg_get_path ();
    if (! cfg_load (CONFIG, cfg_path)) {
        die ("failed to load configuration from %s, aborting!", cfg_path);
    }

    if (x11_open_display ()) {
        do_stop (0);

        /* Setup signal handlers, INT for controlled shutdown and
           HUP for reload */
        signal (SIGINT, &sighandler_int);
        signal (SIGHUP, &sighandler_hup);

        /* Go into background */
        if (! OPTIONS->foreground) {
            do_daemon ();
        }

        x11_init_event_listeners ();

        set_wallpaper_for_current_desktop ();
        main_loop ();

        wallpaper_cache_clear (0);
        x11_close_display ();

        clean_pid_file ();
    }

    mem_free (cfg_path);
}

/**
 * Stop wallpaper daemon.
 */
void
do_stop (int do_output)
{
    pid_t pid = get_pid_from_pid_file ();

    if (pid != -1) {
        do_stop_daemon (pid, do_output);
    } else if (do_output) {
        fprintf (stderr, "no pid read from %s, not stopping daemon\n",
                 CONFIG->pid_path);
    }
}

/**
 * Stop the daemon, if fails check errno and if it was caused by "no
 * such process" remove the pid file.
 */
void
do_stop_daemon (pid_t pid, int do_output)
{
    if (kill (pid, SIGINT) == -1) {
        if (errno == ESRCH) {
            if (unlink (CONFIG->pid_path)) {
                perror ("failed to clean out pid file");
            } else if (do_output) {
                fprintf (stderr, "cleaned out stale pid file %s\n",
                         CONFIG->pid_path);
            }
        } else {
            perror ("failed to signal wallpaperd stop");
        }
    } else if (do_output) {
        printf ("wallpaper daemon with pid %d stopped\n", pid);
    }
}

/**
 * Reload configuration and clean cache.
 */
void
do_reload (void)
{
    char *cfg_path = cfg_get_path ();
    struct config *config = cfg_new ();

    if (cfg_load (config, cfg_path)) {
        /* Configuration successfully loaded, replace current configuration
           and reset the background image. */
        if (CONFIG) {
            wallpaper_cache_clear(1);
            cfg_free (CONFIG);
        }
        CONFIG = config;
        
        set_wallpaper_for_current_desktop ();
    } else {
        fprintf (stderr, "reload of configuration from %s, keeping current.\n",
                 cfg_path);
        cfg_free (config);
    }
}

/**
 * Daemonize application and write pid in temporary file.
 */
void
do_daemon (void)
{
    if (daemon (0, 0) == -1) {
        perror ("failed to daemonize");
        return;
    }

    FILE *fp = fopen (CONFIG->pid_path, "w");
    if (fp) {
        pid_t pid = getpid ();
        fprintf (fp, "%d", pid);
        fclose (fp);
    } else {
        perror ("failed to open pid file for writing");
    }
}

/**
 * Get pid of currently running wallpaperd, return -1 if failed.
 */
pid_t
get_pid_from_pid_file (void)
{
    pid_t pid = -1;

    FILE *fp = fopen (CONFIG->pid_path, "r");
    if (fp)  {
        if (! fscanf (fp, "%d", &pid) == 1) {
            fprintf (stderr, "failed to read pid from %s\n", CONFIG->pid_path);
        }
    }

    return pid;
}

/**
 * Remove pid file if it contains current pid.
 */
void
clean_pid_file (void)
{
    pid_t pid_current = getpid();
    pid_t pid_recorded = get_pid_from_pid_file();

    if (pid_current == pid_recorded) {
        if (unlink (CONFIG->pid_path)) {
            perror ("failed to clean pid file");
        }
    }
}

/**
 * Main loop reading X11 events, updating wallpaper on desktop change.
 */
void
main_loop (void)
{
    XEvent ev;
    int ev_status;
    while (! do_shutdown_flag) {
        ev_status = x11_next_event (&ev);

        if (do_reload_flag) {
            do_reload ();
            do_reload_flag = 1;
        }

        if (ev_status) {
            if (ev.type == PropertyNotify) {
                handle_property_event (&ev);
            } else if (ev.type == x11_get_xrandr_event ()) {
                handle_xrandr_event (&ev);
            }
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
        || ev->xproperty.atom != ATOM_DESKTOP) {
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
    wallpaper_cache_clear (1);
    set_wallpaper_for_current_desktop ();
}

/**
 * Set background image for current desktop.
 */
void
set_wallpaper_for_current_desktop (void)
{
    long current_desktop = x11_get_atom_value_long (x11_get_root_window (),
                                                    ATOM_DESKTOP) + 1;

    const char *name = cfg_get_wallpaper (CONFIG, current_desktop);
    const char *mode_str = cfg_get_mode (CONFIG, current_desktop);
    enum wallpaper_mode mode = wallpaper_mode_from_str (mode_str);

    if (name) {
        char *path = find_wallpaper (name);
        if (path) {
            wallpaper_set (path, mode);
            mem_free (path);
        }
    }
}

/**
 * Find wallpaper in search path.
 */
char*
find_wallpaper (const char *name)
{
    if (name[0] == '/') {
        return strdup (name);
    }

    char **search_path = cfg_get_search_path (CONFIG);

    char *path;
    struct stat buf;
    for (int i = 0; search_path[i] != 0; i++) {
        if (asprintf (&path, "%s/%s", search_path[i], name) != -1
            && ! stat (path, &buf)) {
            return path;
        }
        mem_free (path);
    }

    return 0;
}
