/*
 * cfg.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "background_xml.h"
#include "cfg.h"
#include "util.h"

static char *create_pid_path (void);

static void cfg_unload (struct config *config);
static void read_config (struct config *config);
static enum bg_select_mode read_bg_select_mode (struct config *config);
static long read_interval (struct config *config);
static void read_bg_set (struct config *config);
static int validate_config (struct config *config);

static enum wallpaper_type cfg_get_type_from_str (const char *str);
static enum wallpaper_mode cfg_get_mode_from_str (const char *str);

static void parse_config (FILE *fp, struct config *config);
static char *parse_line (FILE *fp, char *buf, size_t buf_size);
static void parse_key_value (struct config *config, char *line);

static int count_and_add_search_paths(
        struct config *config, const char *search_path_opt, int count_only);


/**
 * Create new empty config structure.
 */
struct config*
cfg_new (void)
{
    struct config *config = mem_new (sizeof (struct config));

    config->path = 0;
    config->pid_path = create_pid_path();

    config->bg_select_mode = NUMBER;
    config->bg_set = 0;
    config->bg_interval = 0;
    config->_search_path = 0;

    config->first = 0;
    config->last = 0;

    return config;
}

/**
 * Create new string with the pid file path.
 */
char*
create_pid_path (void)
{
    char *pid_path;
    uid_t uid = getuid ();
    if (asprintf (&pid_path, "/tmp/.wallpaperd.%d.pid", uid) == -1) {
        die ("failed to construct pid path, aborting");
    }
    return pid_path;
}

/**
 * Free up resources used by configuration.
 */
void
cfg_free (struct config *config)
{
    cfg_unload (config);

    mem_free (config->pid_path);
    mem_free (config->path);
    mem_free (config);
}

/**
 * Load configuration.
 */
int
cfg_load (struct config *config, const char *path)
{
    cfg_unload (config);

    int status = 1;

    FILE *fp = fopen (path, "r");
    if (fp != 0) {
        config->path = str_dup (path);

        parse_config (fp, config);
        read_config (config);
        status = validate_config(config);

        fclose (fp);
    } else {
        die ("failed to open configuration file %s", path);
    }

    return status;
}

/**
 * Unload configuration.
 */
void
cfg_unload (struct config *config)
{
    if (config->_search_path) {
        for (int i = 0; config->_search_path[i] != 0; i++) {
            mem_free (config->_search_path[i]);
        }
        mem_free (config->_search_path);
        config->_search_path = 0;
    }

    if (config->bg_set) {
        background_set_free (config->bg_set);
        config->bg_set = 0;
    }

    struct cfg_node *it, *it_next;
    for (it = config->first; it; it = it_next) {
        it_next = it->next;
        mem_free (it->key);
        mem_free (it->value);
        mem_free (it);
    }
    config->first = 0;
    config->last = 0;
}

/**
 * Read configuration parameters.
 */
void
read_config (struct config *config)
{
    config->bg_select_mode = read_bg_select_mode (config);
    config->bg_interval = read_interval (config);

    if (config->bg_select_mode == SET) {
        read_bg_set (config);
    }
}

/**
 * Read bg select mode from configuration and convert mode string into
 * bg_select_mode enum.
 */
enum bg_select_mode
read_bg_select_mode (struct config *config)
{
    const char *mode_str = cfg_get (config, "config.mode");
    enum bg_select_mode mode = NUMBER;

    if (! mode_str || ! strcmp (mode_str, "NUMBER")) {
    } else if (! strcmp (mode_str, "NAME")) {
        mode = NAME;
    } else if (! strcmp (mode_str, "RANDOM")) {
        mode = RANDOM;
    } else if (! strcmp (mode_str, "SET")) {
        mode = SET;
    } else {
        fprintf (stderr, "unknown selection mode %s, setting to NUMBER",
                 mode_str);
    }

    return mode;
}

/**
 * Read config.interval as long.
 */
long
read_interval (struct config *config)
{
    long interval = -1;
    const char *interval_str = cfg_get (config, "config.interval");
    if (interval_str) {
        interval = strtol (interval_str, 0, 10);
    }
    return interval;
}

/**
 * Read background set from configuration file.
 */
void
read_bg_set (struct config *config)
{
    const char *path = cfg_get (config, "config.set");
    if (path) {
        char *path_expanded = expand_home (path);

        FILE *fp = fopen (path_expanded, "r");
        if (fp) {
            config->bg_set = background_xml_parse (fp);
            fclose (fp);
        } else {
            die ("failed to open config.set configuration at %s",
                 path_expanded);
        }
    } else {
        die ("no config.set option available in configuration.");
    }
}

/**
 * Validate configuration, return 1 if all required options are set.
 */
int
validate_config (struct config *config)
{
    const char *required_options[] = {
        "wallpaper.default.image",
        "wallpaper.default.mode",
        0
        };

    int config_ok = 1;
    for (int i = 0; required_options[i] != 0; i++) {
        const char *key = required_options[i];
        const char *value = cfg_get (config, key);
        if (! value) {
            fprintf (stderr, "required option %s not set\n", key);
            config_ok = 0;
        } else if (! strlen (value)) {
            fprintf (stderr, "required option %s empty\n", key);
            config_ok = 0;
        }
    }

    return config_ok;
}

/**
 * Get configuration option.
 */
const char*
cfg_get (struct config *config, const char *key)
{
    struct cfg_node *it = config->first;
    for (; it != 0; it = it->next) {
        if (! strcmp (key, it->key)) {
            return it->value;
        }
    }
    return 0;
}

/**
 * Get color configured for desktop.
 */
const char*
cfg_get_color (struct config *config, long desktop)
{
    const char *color = 0;

    char *color_key;
    if (asprintf (&color_key, "wallpaper.%ld.color", desktop) != -1) {
        color = cfg_get (config, color_key);
        mem_free (color_key);
    }

    if (! color) {
        color = cfg_get (config, "wallpaper.default.color");
    }

    return color;
}

/**
 * Get wallpaper configured for desktop.
 */
const char*
cfg_get_wallpaper (struct config *config, long desktop)
{
    const char *wallpaper_path = 0;

    char *wallpaper_key;
    if (asprintf (&wallpaper_key, "wallpaper.%ld.image", desktop) != -1) {
        wallpaper_path = cfg_get (config, wallpaper_key);
        mem_free (wallpaper_key);
    }

    if (! wallpaper_path) {
        wallpaper_path = cfg_get (config, "wallpaper.default.image");
    }

    return wallpaper_path;
}

/**
 * Get wallpaper configured for desktop.
 */
enum wallpaper_type
cfg_get_type (struct config *config, long desktop)
{
    const char *type_str = 0;

    char *type_key;
    if (asprintf (&type_key, "wallpaper.%ld.type", desktop) != -1) {
        type_str = cfg_get (config, type_key);
        mem_free (type_key);
    }

    if (! type_str) {
        type_str = cfg_get (config, "wallpaper.default.type");
    }

    return cfg_get_type_from_str (type_str);
}

/**
 * Return wallpaper type from string, defaults to IMAGE if parsing
 * fails.
 */
enum wallpaper_type
cfg_get_type_from_str (const char *str)
{
    enum wallpaper_type type = IMAGE;

    if (! str) {
    } else if (! strcasecmp (str, "IMAGE")) {
        type = IMAGE;
    } else if (! strcasecmp (str, "COLOR")) {
        type = COLOR;
    }

    return type;
}

/**
 * Get wallpaper configured for desktop.
 */
enum wallpaper_mode
cfg_get_mode (struct config *config, long desktop)
{
    const char *mode_str = 0;

    char *mode_key;
    if (asprintf (&mode_key, "wallpaper.%ld.mode", desktop) != -1) {
        mode_str = cfg_get (config, mode_key);
        mem_free (mode_key);
    }

    if (! mode_str) {
        mode_str = cfg_get (config, "wallpaper.default.mode");
    }

    return cfg_get_mode_from_str (mode_str);
}

/**
 * Return wallpaper mode from string, defaults to CENTERED if parsing
 * fails.
 */
enum wallpaper_mode
cfg_get_mode_from_str (const char *str)
{
    enum wallpaper_mode mode = CENTERED;

    if (! str) {
    } else if (! strcasecmp (str, "CENTERED")) {
        mode = CENTERED;
    } else if (! strcasecmp (str, "TILED")) {
        mode = TILED;
    } else if (! strcasecmp (str, "FILLED")) {
        mode = FILL;
    } else if (! strcasecmp (str, "ZOOMED")) {
        mode = ZOOM;
    }

    return mode;
}

/**
 * Get image search path from configuration.
 */
char**
cfg_get_search_path (struct config *config)
{
    if (! config->_search_path) {
        const char *search_path_opt = cfg_get (config, "path.search");
        if (! search_path_opt) {
            search_path_opt = ".:~:~/Pictures";
        }

        int num;
        num = count_and_add_search_paths (config, search_path_opt, 1);
        config->_search_path = mem_new (sizeof(char*) * (num + 1));
        num = count_and_add_search_paths (config, search_path_opt, 0);
        config->_search_path[num] = 0;
    }

    return config->_search_path;
}

/**
 * Count path entries in : separted string, if count_only 0 then
 * expand user and add the result to the already allocated search_path
 */
int
count_and_add_search_paths (struct config *config,
                            const char *search_path_opt, int count_only)
{
    
    char *search_path_buf = str_dup (search_path_opt);

    int pos;
    char *tok = strtok (search_path_buf, ":");
    for (pos = 0; tok != 0; pos++) {
        if (! count_only) {
            config->_search_path[pos] = expand_home (tok);
        }
        tok = strtok (0, ":");
    }

    mem_free (search_path_buf);

    return pos;
}

/**
 * Return path to wallpaperd configuration file.
 */
char*
cfg_get_path (void)
{
    char *path;

    path = expand_home ("~/.wallpaperd.cfg");
    if (! file_exists (path)) {
        char *path_alt = expand_home("~/.config/wallpaperd/wallpaperd.cfg");
        if (file_exists (path_alt)) {
            mem_free (path);
            path = path_alt;
        }
    }

    return path;
}

/**
 * Add config node to configuration.
 */
void
cfg_add_node (struct config *config, const char *key, const char *value)
{
    struct cfg_node *node = mem_new (sizeof (struct cfg_node));
    node->key = str_dup (key);
    node->value = str_dup (value);
    node->next = 0;

    if (config->last) {
        config->last->next = node;
        config->last = node;
    } else {
        config->first = config->last = node;
    }
}

/**
 * Parse file into configuration file.
 */
void
parse_config (FILE *fp, struct config *config)
{
    size_t buf_size = 4096;
    char *buf = mem_new (sizeof(char) * buf_size);

    char *line;
    while ((line = parse_line (fp, buf, buf_size)) != 0) {
        if (line[0] == '#') {
            /* Filter out comment lines */
            continue;
        } else if (! str_first_not_of (line, " \t")) {
            /* Filter out empty lines */
        } else {
            parse_key_value (config, line);

        }
    }

    mem_free (buf);
}

/**
 * Read single line, return 0.
 */
char*
parse_line (FILE *fp, char *buf, size_t buf_size)
{
    int c, pos = 0;
    while ((c = fgetc  (fp)) != EOF && pos < buf_size) {
        if (c == '\n') {
            break;
        }
        buf[pos++] = c;
    }
    buf[pos] = '\0';

    return c == EOF ? 0 : buf;
}

/**
 * Parse key = value pair and add to configuration.
 */
void
parse_key_value (struct config *config, char *line)
{
    char *key_end = str_first_of(line, " =");
    if (key_end != 0) {
        *key_end = '\0';
        
        char *value_start = str_first_not_of(key_end + 1, " =");
        if (value_start != 0) {
            cfg_add_node (config, line, value_start);
        }
    }
}
