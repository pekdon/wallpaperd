/*
 * wallpaper.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <Imlib2.h>
#include <X11/Xatom.h>

#include "cache.h"
#include "render.h"
#include "wallpaper.h"
#include "util.h"
#include "x11.h"

static struct cache *CACHE = 0;
static char CACHE_SPEC[4096] = { '\0' };
static enum wallpaper_type CACHE_TYPE = IMAGE;
static enum wallpaper_mode CACHE_MODE = MODE_UNKNOWN;

static Imlib_Image wallpaper_render (Imlib_Image image,
                                     enum wallpaper_mode mode);
static void wallpaper_set_x11 (struct cache_node *node);
static Pixmap wallpaper_create_x11_pixmap (Imlib_Image image);

/**
 * Set wallpaper from image path.
 */
void
wallpaper_set (const char *spec,
               enum wallpaper_type type, enum wallpaper_mode mode)
{
    if (! CACHE) {
        wallpaper_cache_clear (1);
    } else if (CACHE_TYPE == type && CACHE_MODE == mode
               && strcmp (CACHE_SPEC, spec) == 0) {
        return;
    }

    struct cache_node *node = cache_get_pixmap (CACHE, spec, type, mode);
    if (! node) {
        Imlib_Image image_rendered;
        if (type == IMAGE) {
            Imlib_Image image = imlib_load_image (spec);
            if (! image) {
                fprintf (stderr, "failed to set background from %s\n", spec);
                return;
            }
            image_rendered = wallpaper_render (image, mode);

            imlib_context_set_image (image);
            imlib_free_image ();
        } else {
            struct geometry *disp = x11_get_geometry ();
            image_rendered = render_color (disp, spec);
            mem_free (disp);
        }

        Pixmap pixmap = wallpaper_create_x11_pixmap (image_rendered);
        imlib_context_set_image (image_rendered);
        imlib_free_image ();
        node = cache_set_pixmap (CACHE, spec, type, mode, pixmap);
    }
    wallpaper_set_x11 (node);

    strncpy (CACHE_SPEC, spec, strlen (spec));
    CACHE_TYPE = type;
    CACHE_MODE = mode;
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
    CACHE_TYPE = IMAGE;
    CACHE_MODE = MODE_UNKNOWN;
}

/**
 * Render image on all available heads.
 */
static Imlib_Image
wallpaper_render (Imlib_Image image, enum wallpaper_mode mode)
{
    struct geometry *disp = x11_get_geometry ();
    struct color black = { 0, 0, 0 };
    Imlib_Image image_disp = render_new_color (disp->width, disp->height, &black);
    mem_free (disp);

    struct geometry **heads = x11_get_heads ();
    for (int i = 0; heads[i]; i++) {
        Imlib_Image image_head = render_image (heads[i], image, mode);

        imlib_context_set_image (image_disp);
        imlib_blend_image_onto_image (
            image_head, 0,
            0, 0, heads[i]->width, heads[i]->height,
            heads[i]->x, heads[i]->y, heads[i]->width, heads[i]->height);

        imlib_context_set_image (image_head);
        imlib_free_image ();

        mem_free (heads[i]);
    }
    mem_free (heads);

    return image_disp;
}

/**
 * Render image as X11 background.
 */
void
wallpaper_set_x11 (struct cache_node *node)
{
    static Pixmap last_pixmap = 0;
    if (last_pixmap != node->pixmap) {
        last_pixmap = node->pixmap;

        x11_set_atom_value_long (x11_get_root_window (), ATOM_ROOTPMAP_ID,
                                 XA_PIXMAP, node->pixmap);
        x11_set_background_pixmap (x11_get_root_window (), node->pixmap);
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


