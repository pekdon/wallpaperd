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
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <X11/Xlib.h>
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif /* HAVE_XRANDR */

#include "cfg.h"
#include "compat.h"
#include "wallpaper.h"
#include "wallpaperd.h"
#include "util.h"
#include "x11.h"

#define IS_CONFIG_TIMED_MODE() \
    (CONFIG->bg_select_mode == MODE_SET \
     || (CONFIG->bg_select_mode == MODE_RANDOM && CONFIG->bg_interval > 0))

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
static void main_loop_set_interval (time_t *next_interval);
static void main_loop_check_change_interval (time_t *next_interval);
static int get_next_event_wait (time_t next_interval);
static void handle_property_event (XEvent *ev);
static void handle_xrandr_event (XEvent *ev, int ev_xrandr);

static void set_wallpaper_for_current_desktop (void);
static void set_wallpaper_name (long desktop);
static void set_wallpaper_number (long desktop);
static void set_wallpaper_random (void);
static void set_wallpaper_set (void);
static void set_wallpaper_default (void);
static void set_wallpaper_and_free (char *spec,
                                    enum wallpaper_type type,
                                    enum wallpaper_mode mode);

static char *find_wallpaper (const char *name);
static char *find_wallpaper_by_name (const char *name);
static char *find_wallpaper_random (void);
static int count_and_select_image_in_search_path (int image_select,
                                                  char **path_ret);
static void select_image (int num, int image_select,
                          const char *path, const char *name, char **path_ret);
static int is_image_file_ext (const char *name);

static const char *IMAGE_EXTS[] = {"png", "jpg", 0};

static int do_reload_flag = 0;
static int do_next_flag = 0;
static int do_shutdown_flag = 0;

struct options *OPTIONS = 0;
struct config *CONFIG = 0;

/**
 * Handler for HUP signal, set reload flag.
 */
static void
sighandler_hup_int_usr1 (int signal)
{
    if (signal == SIGHUP) {
        do_reload_flag = 1;
    } else if (signal == SIGINT) {
        do_shutdown_flag = 1;
    } else if (signal == SIGUSR1) {
        do_next_flag = 1;
    }
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
    options->image = NULL;
    options->mode = MODE_UNKNOWN;
    options->workspace = "default";

    int opt;
    while ((opt = getopt (argc, argv, "fhi:m:sw:")) != -1) {
        switch (opt) {

        case 'f':
            options->foreground = 1;
            break;
        case 'i':
            options->image = expand_abs (optarg);
            break;
        case 'm':
            options->mode = cfg_get_mode_from_str (optarg);
            break;
        case 's':
            options->stop = 1;
            break;
        case 'w':
            options->workspace = optarg;
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
    fprintf (stderr, "usage: %s [-fhimsw]\n", name);
    fprintf (stderr, "\n");
    fprintf (stderr, "  -f foreground    do not go into background\n");
    fprintf (stderr, "  -h help          print help information\n");
    fprintf (stderr, "  -i image         set image for workspace\n");
    fprintf (stderr, "  -m mode          set image mode for workspace\n");
    fprintf (stderr, "  -s stop          stop running daemon\n");
    fprintf (stderr, "  -w workspace     workspace image applies on, defaults"
             " to default\n");
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
    if (OPTIONS->image) {
        mem_free (OPTIONS->image);
    }

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

    srandom(time(NULL));

    if (OPTIONS->image || OPTIONS->mode != MODE_UNKNOWN) {
        if (OPTIONS->image) {
            char *image_key;
            asprintf(&image_key, "wallpaper.%s.image", OPTIONS->workspace);
            cfg_set (CONFIG, image_key, OPTIONS->image);
            mem_free (image_key);
        }

        if (OPTIONS->mode != MODE_UNKNOWN) {
            char *mode_key;
            asprintf(&mode_key, "wallpaper.%s.mode", OPTIONS->workspace);
            cfg_set (CONFIG, mode_key, cfg_get_str_from_mode(OPTIONS->mode));
            mem_free (mode_key);
        }

        cfg_save (CONFIG, cfg_path);
    }

    if (x11_open_display ()) {
        do_stop (0);

        /* Setup signal handlers, INT for controlled shutdown and
           HUP for reload */
        signal (SIGINT, &sighandler_hup_int_usr1);
        signal (SIGHUP, &sighandler_hup_int_usr1);
        signal (SIGUSR1, &sighandler_hup_int_usr1);

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
    time_t next_interval = time (0);
    main_loop_set_interval (&next_interval);

    XEvent ev;
    int ev_status, ev_xrandr;
    while (! do_shutdown_flag) {        
        ev_status = x11_next_event (&ev, get_next_event_wait (next_interval));

        if (do_reload_flag) {
            do_reload ();
            do_reload_flag = 0;
        }
        if (do_next_flag) {
            do_next_flag = next_interval = 0;
        }

        main_loop_check_change_interval (&next_interval);

        if (ev_status) {
            if (ev.type == PropertyNotify) {
                handle_property_event (&ev);
            } else if (ev.type == ConfigureNotify
                       && ev.xconfigurerequest.window == x11_get_root_window ()) {
                handle_xrandr_event (&ev, ConfigureNotify);
            } else if ((ev_xrandr = x11_is_xrandr_event (&ev)) != 0) {
                handle_xrandr_event (&ev, ev_xrandr);
            }
        }
    }
}

/**
 * Get number of seconds to wait for next event, -1 if bg_select_mode
 * does not change at timed intervals.
 */
int
get_next_event_wait (time_t next_interval)
{
    if (IS_CONFIG_TIMED_MODE()) {
        time_t next = next_interval - time (0);
        return next > 0 ? next : 1;
    } else {
        return -1;
    }
}


void
main_loop_set_interval (time_t *next_interval)
{
    if (CONFIG->bg_select_mode == MODE_RANDOM) {
        *next_interval = time (0) + CONFIG->bg_interval;
    } else if (CONFIG->bg_select_mode == MODE_SET) {
        *next_interval += CONFIG->bg_set->duration;
    }
}

/**
 * Check if background change interval has been reached, if so set a
 * new background and update next_interval.
 */
void
main_loop_check_change_interval (time_t *next_interval)
{
    if (IS_CONFIG_TIMED_MODE() && (time (0) > *next_interval)) {
        set_wallpaper_for_current_desktop ();
        main_loop_set_interval (next_interval);
    }
}


/**
 * Handle property event, detects desktop changes.
 */
void
handle_property_event (XEvent *ev)
{
    if (ev->xproperty.window != x11_get_root_window ()) {
        return;
    }

    int do_update = 0;
    if (ev->xproperty.atom == ATOM_DESKTOP) {
        do_update = 1;
    } else if (ev->xproperty.atom == ATOM_DESKTOP_NAMES) {
        x11_get_desktop_names (1);
        do_update = 1;
    }

    if (do_update && CONFIG->bg_select_mode != MODE_RANDOM && CONFIG->bg_select_mode != MODE_STATIC) {
        set_wallpaper_for_current_desktop ();
    }
}

/**
 * Handle xrandr events, this invalidates the cache and re-sets the
 * background image.
 */
void
handle_xrandr_event (XEvent *ev, int ev_xrandr)
{
#ifdef HAVE_XRANDR
    if (ev_xrandr == RRNotify) {
        XRRNotifyEvent *ev_notify = (XRRNotifyEvent*) ev;
        if (ev_notify->subtype == RRNotify_OutputChange) {
        } else if (ev_notify->subtype == RRNotify_CrtcChange) {
        } else if (ev_notify->subtype == RRNotify_OutputProperty) {
        }
    } else if (ev_xrandr == RRScreenChangeNotify) {
        XRRUpdateConfiguration (ev);
    } else if (ev_xrandr == ConfigureNotify) {
        XRRUpdateConfiguration (ev);
    }
#endif /* HAVE_XRANDR */

    wallpaper_cache_clear (0);
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
    switch (CONFIG->bg_select_mode) {
    case MODE_NAME:
        set_wallpaper_name (current_desktop);
        break;
    case MODE_NUMBER:
        set_wallpaper_number (current_desktop);
        break;
    case MODE_RANDOM:
        set_wallpaper_random ();
        break;
    case MODE_SET:
        set_wallpaper_set ();
        break;
    case MODE_STATIC:
        set_wallpaper_default ();
        break;
    }
}

/**
 * Set wallpaper based on desktop name.
 */
void
set_wallpaper_name (long desktop)
{
    const char *name = x11_get_desktop_name (desktop - 1);

    enum wallpaper_type type = cfg_get_type (CONFIG, desktop);
    enum wallpaper_mode mode = cfg_get_mode (CONFIG, desktop);

    if (type == COLOR) {
        const char *spec = cfg_get_color (CONFIG, -1);
        wallpaper_set (spec, type, mode);
    } else {
        char *spec = NULL;
        if (name) {
            spec = find_wallpaper_by_name (name);
        }
        if (! spec) {
            name = cfg_get_wallpaper (CONFIG, -1);
            spec = find_wallpaper (name);
        }
        set_wallpaper_and_free (spec, type, mode);
    }
}

/**
 * Set wallpaper based on desktop number.
 */
void
set_wallpaper_number (long desktop)
{
    enum wallpaper_type type = cfg_get_type (CONFIG, desktop);
    enum wallpaper_mode mode = cfg_get_mode (CONFIG, desktop);

    if (type == COLOR) {
        const char *spec = cfg_get_color (CONFIG, desktop);
        wallpaper_set (spec, type, mode);
    } else {
        const char *name = cfg_get_wallpaper (CONFIG, desktop);
        char *spec = find_wallpaper (name);
        set_wallpaper_and_free (spec, type, cfg_get_mode (CONFIG, desktop));
    }
}

/**
 * Set wallpaper selecting random from the search path.
 */
void
set_wallpaper_random (void)
{
    char *path = find_wallpaper_random ();
    set_wallpaper_and_free (path,
                            cfg_get_type (CONFIG, -1),
                            cfg_get_mode (CONFIG, -1));
}

/**
 * Set wallpaper using background from wallpaper set.
 */
void
set_wallpaper_set (void)
{
    struct background *bg = background_set_get_now (CONFIG->bg_set);
    if (bg) {
        wallpaper_set (bg->path,
                       cfg_get_type (CONFIG, -1), cfg_get_mode (CONFIG, -1));
    }
}

/**
 * Set wallpaper using the default configuration.
 */
static
void set_wallpaper_default (void)
{
    set_wallpaper_number (-1);
}

/**
 * If path != 0, set wallpaper and free path.
 */
void
set_wallpaper_and_free (char *spec,
                        enum wallpaper_type type, enum wallpaper_mode mode)
{
    if (spec) {
        wallpaper_set (spec, type, mode);
        mem_free (spec);
    }
}

/**
 * Find wallpaper in search path.
 */
char*
find_wallpaper (const char *name)
{
    if (name[0] == '/') {
        return str_dup (name);
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

/**
 * Find wallpaper by name.
 */
char*
find_wallpaper_by_name (const char *name)
{
    char *path, *test_name;
    for (int i = 0; IMAGE_EXTS[i] != 0; i++) {
        if (asprintf (&test_name, "%s.%s", name, IMAGE_EXTS[i]) != 1) {
            path = find_wallpaper (test_name);
            mem_free (test_name);

            if (path != 0) {
                return path;
            }
        }
    }

    return 0;
}

/**
 * Select a random image from image search directories.
 */
char*
find_wallpaper_random (void)
{
    static int image_select_last = -1;
    char *path = 0;

    /* Count and select random image. */
    int num = count_and_select_image_in_search_path (-1, &path);
    if (num > 0) {
        int image_select;
        do {
            image_select = random () % num;
        } while (num > 1 && image_select == image_select_last);
        image_select_last = image_select;

        image_select = count_and_select_image_in_search_path (
            image_select, &path);
    }

    return path;
}


/**
 * Count images in search path, if image_select >= 0 select that
 * image and store full path in path_ret.
 */
int
count_and_select_image_in_search_path (int image_select, char **path_ret)
{
    char **search_path = cfg_get_search_path (CONFIG);

    /* Count and select random directory. */
    int num = 0;
    *path_ret = 0;
    for (int i = 0; search_path[i] != 0; i++) {
        DIR *dirp = opendir (search_path[i]);
        if (! dirp) {
            continue;
        }

        struct dirent *entry;
        while (*path_ret == 0 && (entry = readdir (dirp)) != 0) {
            if (is_image_file_ext (entry->d_name)) {
                select_image (num++, image_select,
                              search_path[i], entry->d_name, path_ret);
            }
        }
 
        closedir (dirp);
    }

    return num;
}

/**
 * If num and image_select are equal, construct full path in path_ret.
 */
void
select_image (int num, int image_select,
              const char *path, const char *name, char **path_ret)
{
    if (image_select > -1 && num == image_select) {
        if (asprintf (path_ret, "%s/%s", path, name) == -1) {
            fprintf (stderr, "failed to construct full path for %s", name);
        }
    }
}

/**
 * Check if image has one of the valid image file extensions.
 */
int
is_image_file_ext (const char *name)
{
    for (int i = 0; IMAGE_EXTS[i] != 0; i++) {
        if (str_ends_with (name, IMAGE_EXTS[i])) {
            return 1;
        }
    }
    return 0;
}
