#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cfg.h"
#include "wallpaper_match.h"
#include "util.h"
#include "x11.h"

static const char *IMAGE_EXTS[] = {"png", "jpg", 0};

static struct wallpaper_spec *wallpaper_spec_new (void);
static struct wallpaper_spec *wallpaper_match_name (
        struct wallpaper_filter *filter);
static struct wallpaper_spec *wallpaper_match_number (
        struct wallpaper_filter *filter);
static struct wallpaper_spec *wallpaper_match_random (
        struct wallpaper_filter *filter);
static struct wallpaper_spec *wallpaper_match_set (
        struct wallpaper_filter *filter);
static struct wallpaper_spec *wallpaper_match_default (
        struct wallpaper_filter *filter);

static char *find_wallpaper (const char *name);
static char *find_wallpaper_by_name (const char *name);
static char *find_wallpaper_random (void);
static int count_and_select_image_in_search_path (int image_select,
                                                  char **path_ret);
static void select_image (int num, int image_select,
                          const char *path, const char *name, char **path_ret);
static int is_image_file_ext (const char *name);

/**
 * Find matching wallpaper specification from filter.
 */
struct wallpaper_spec*
wallpaper_match (struct wallpaper_filter *filter)
{
    struct wallpaper_spec *spec;

    switch (filter->mode) {
    case MODE_NAME:
        spec = wallpaper_match_name (filter);
        break;
    case MODE_NUMBER:
        spec = wallpaper_match_number (filter);
        break;
    case MODE_RANDOM:
        spec = wallpaper_match_random (filter);
        break;
    case MODE_SET:
        spec = wallpaper_match_set (filter);
        break;
    case MODE_STATIC:
    default:
        spec = wallpaper_match_default (filter);
        break;
    }

    /* Handle missing spec, convenient to catch it here instead of
     * in all modes. */
    if (spec != NULL && spec->spec == NULL) {
        wallpaper_spec_free(spec);
        spec = NULL;
    }

    return spec;
}

static struct wallpaper_spec*
wallpaper_spec_new (void)
{
    struct wallpaper_spec *spec = mem_new(sizeof(struct wallpaper_spec));
    spec->type = WALLPAPER_TYPE_UNKNOWN;
    spec->mode = MODE_UNKNOWN;
    spec->spec = NULL;
    return spec;
}

/**
 * Free resources used by spec.
 */
void
wallpaper_spec_free (struct wallpaper_spec *spec)
{
    if (spec->spec != NULL) {
        mem_free (spec->spec);
    }
    mem_free (spec);
}

/**
 * Get wallpaper based on desktop name.
 */
struct wallpaper_spec*
wallpaper_match_name (struct wallpaper_filter *filter)
{
    struct wallpaper_spec *spec = wallpaper_spec_new ();
    spec->type = cfg_get_type (CONFIG, filter->desktop);
    spec->mode = cfg_get_mode (CONFIG, filter->desktop);

    if (spec->type == WALLPAPER_TYPE_COLOR) {
        const char *color = cfg_get_color (CONFIG, -1);
        spec->spec = str_dup (color);
    } else {
        if (filter->desktop_name) {
            spec->spec = find_wallpaper_by_name (filter->desktop_name);
        }
        if (! spec->spec) {
            const char *name = cfg_get_wallpaper (CONFIG, -1);
            spec->spec = find_wallpaper (name);
        }
    }

    return spec;
}

/**
 * Get wallpaper based on desktop number.
 */
struct wallpaper_spec*
wallpaper_match_number (struct wallpaper_filter *filter)
{
    struct wallpaper_spec *spec = wallpaper_spec_new ();
    spec->type = cfg_get_type (CONFIG, filter->desktop);
    spec->mode = cfg_get_mode (CONFIG, filter->desktop);

    if (spec->type == WALLPAPER_TYPE_COLOR) {
        const char *color = cfg_get_color (CONFIG, filter->desktop);
        spec->spec = str_dup (color);
    } else {
        const char *name = cfg_get_wallpaper (CONFIG, filter->desktop);
        spec->spec = find_wallpaper (name);
    }

    return spec;
}

/**
 * Get wallpaper selecting random from the search path.
 */
struct wallpaper_spec*
wallpaper_match_random (struct wallpaper_filter *filter)
{
    struct wallpaper_spec *spec = wallpaper_spec_new ();
    spec->type = cfg_get_type (CONFIG, -1);
    spec->mode = cfg_get_mode (CONFIG, -1);
    spec->spec = find_wallpaper_random ();
    return spec;
}

/**
 * Set wallpaper using background from wallpaper set.
 */
struct wallpaper_spec*
wallpaper_match_set (struct wallpaper_filter *filter)
{
    struct wallpaper_spec *spec = wallpaper_spec_new ();

    struct background *bg = background_set_get_now (CONFIG->bg_set);
    if (bg) {
        spec->type = cfg_get_type (CONFIG, -1);
        spec->mode = cfg_get_mode (CONFIG, -1);
        spec->spec = str_dup (bg->path);
    }

    return spec;
}

/**
 * Set wallpaper using the default configuration.
 */
struct wallpaper_spec*
wallpaper_match_default (struct wallpaper_filter *filter)
{
    filter->desktop = -1;
    return wallpaper_match_number (filter);
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

