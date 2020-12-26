/*
 * wallpaper.c for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <Imlib2.h>
#include <X11/Xatom.h>

#include "cache.h"
#include "compat.h"
#include "render.h"
#include "wallpaper.h"
#include "util.h"
#include "x11.h"

static struct cache *CACHE = 0;
static char CACHE_SPEC[4096] = { '\0' };

static char *wallpaper_render_spec (struct wallpaper_filter *filter);
static Imlib_Image wallpaper_render (struct wallpaper_filter *filter);
static void wallpaper_set_x11 (Pixmap pixmap);
static Pixmap wallpaper_create_x11_pixmap (Imlib_Image image);

/**
 * Set wallpaper from image path.
 */
void
wallpaper_set (struct wallpaper_filter *filter)
{
    if (! CACHE) {
        wallpaper_cache_clear (1);
    }

    /* Build specification for filter to check if cache is ok. */
    char *cache_spec = wallpaper_render_spec (filter);
    if (strcmp (CACHE_SPEC, cache_spec) == 0) {
        mem_free (cache_spec);
        return;
    }

    Pixmap pixmap = None;
    struct cache_node *node = cache_get_pixmap (CACHE, cache_spec);
    if (node == NULL) {
        Imlib_Image image = wallpaper_render (filter);
        pixmap = wallpaper_create_x11_pixmap (image);
        imlib_context_set_image (image);
        imlib_free_image ();
        node = cache_set_pixmap (CACHE, cache_spec, pixmap);
    } else {
        pixmap = node->pixmap;
    }

    if (pixmap != None) {
        wallpaper_set_x11 (pixmap);
    }

    snprintf (CACHE_SPEC, sizeof(CACHE_SPEC), "%s", cache_spec);
    mem_free (cache_spec);
}

/**
 * Invalidate all cache data.
 */
void
wallpaper_cache_clear (int do_alloc)
{
    if (CACHE != 0) {
        cache_free (CACHE);
        CACHE = 0;
    }
    if (do_alloc) {
        CACHE = cache_new ();
    }
    CACHE_SPEC[0] = '\0';
}

/**
 * Create spec string for filter.
 */
char*
wallpaper_render_spec (struct wallpaper_filter *filter)
{
    char buf[4096] = {0};

    int heads = x11_get_num_heads ();
    for (int i = 0; i < heads; i++) {
        struct wallpaper_spec *spec = wallpaper_match (filter);
        if (spec == NULL) {
            strlcat(buf, "UNDEFINED", sizeof(buf));
        } else {
            size_t pos = strlen(buf);
            snprintf(buf + pos, sizeof(buf) - pos, "%s-%d-%d",
                     spec->spec, spec->mode, spec->type);
            wallpaper_spec_free (spec);
        }
    }

    return str_dup(buf);
}

/**
 * Render image on all available heads.
 */
static Imlib_Image
wallpaper_render (struct wallpaper_filter *filter)
{
    struct geometry *disp = x11_get_geometry ();
    struct color black = { 0, 0, 0 };
    Imlib_Image image_disp =
        render_new_color (disp->width, disp->height, &black);
    mem_free (disp);

    struct geometry **heads = x11_get_heads ();
    for (int i = 0; heads[i]; i++) {
        filter->head = i;
        struct wallpaper_spec *spec = wallpaper_match (filter);
        if (spec != NULL) {
            Imlib_Image image_head;
            if (spec->type == WALLPAPER_TYPE_COLOR) {
                image_head = render_color (heads[i], spec->spec);
            } else {
                image_head = render_image (heads[i], spec->spec, spec->mode);
            }

            imlib_context_set_image (image_disp);
            imlib_blend_image_onto_image (
                image_head, 0,
                0, 0, heads[i]->width, heads[i]->height,
                heads[i]->x, heads[i]->y, heads[i]->width, heads[i]->height);

            imlib_context_set_image (image_head);
            imlib_free_image ();

            wallpaper_spec_free (spec);
        }
        mem_free (heads[i]);
    }
    mem_free (heads);

    return image_disp;
}

/**
 * Render image as X11 background.
 */
void
wallpaper_set_x11 (Pixmap pixmap)
{
    static Pixmap last_pixmap = 0;
    if (last_pixmap != pixmap) {
        last_pixmap = pixmap;

        x11_set_atom_value_long (x11_get_root_window (), ATOM_ROOTPMAP_ID,
                                 XA_PIXMAP, pixmap);
        x11_set_background_pixmap (x11_get_root_window (), pixmap);
    }
}

/**
 * Create Pixmap from from image.
 */
Pixmap
wallpaper_create_x11_pixmap (Imlib_Image image)
{
    imlib_context_set_display (x11_get_display ());
    imlib_context_set_visual (x11_get_visual ());
    imlib_context_set_colormap (x11_get_colormap ());
    imlib_context_set_image (image);
    imlib_context_set_drawable (x11_get_root_window ());

    Pixmap pixmap = 0, mask = 0;
    imlib_render_pixmaps_for_whole_image (&pixmap, &mask);
    x11_set_background_pixmap (x11_get_root_window (), pixmap);
    return pixmap;
}

