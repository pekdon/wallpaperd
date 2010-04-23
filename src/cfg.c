
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "util.h"

static int validate_config (struct config *config);

static void parse_config (FILE *fp, struct config *config);
static char *parse_line (FILE *fp, char *buf, size_t buf_size);
static void parse_key_value (struct config *config, char *line);

static int count_and_add_search_paths(
        struct config *config, const char *search_path_opt, int count_only);

/**
 * Load configuration.
 */
struct config*
cfg_load (const char *path)
{
    struct config *config = mem_new (sizeof (struct config));
    FILE *fp = fopen (path, "r");
    if (fp != 0) {
        config->path = strdup (path);

        parse_config (fp, config);
        if (! validate_config(config)) {
            cfg_free (config);
            config = 0;
        }

        fclose (fp);
    } else {
        die ("failed to open configuration file %s", path);
    }

    return config;
}

/**
 * Free up resources used by configuration.
 */
void
cfg_free (struct config *config)
{
    if (config->search_path) {
        for (int i = 0; config->search_path[i] != 0; i++) {
            mem_free (config->search_path[i]);
        }
        mem_free (config->search_path);
    }

    mem_free (config->path);
    mem_free (config);
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
const char*
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

    return mode_str;
}

/**
 * Get image search path from configuration.
 */
char**
cfg_get_search_path (struct config *config)
{
    if (! config->search_path) {
        const char *search_path_opt = cfg_get (config, "path.search");
        if (! search_path_opt) {
            search_path_opt = ".:~:~/Pictures";
        }

        int num;
        num = count_and_add_search_paths (config, search_path_opt, 1);
        config->search_path = mem_new (sizeof(char*) * (num + 1));
        num = count_and_add_search_paths (config, search_path_opt, 0);
        config->search_path[num] = 0;
    }

    return config->search_path;
}

/**
 * Count path entries in : separted string, if count_only 0 then
 * expand user and add the result to the already allocated search_path
 */
int
count_and_add_search_paths (struct config *config,
                            const char *search_path_opt, int count_only)
{
    
    char *search_path_buf = strdup (search_path_opt);

    int pos;
    char *tok = strtok (search_path_buf, ":");
    for (pos = 0; tok != 0; pos++) {
        if (! count_only) {
            config->search_path[pos] = expand_home (tok);
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
    return expand_home ("~/.wallpaperd.cfg");
}

/**
 * Add config node to configuration.
 */
void
cfg_add_node (struct config *config, const char *key, const char *value)
{
    struct cfg_node *node = mem_new (sizeof (struct cfg_node));
    node->key = strdup (key);
    node->value = strdup (value);
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
            cfg_add_node(config, line, value_start);
        }
    }
}
