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
#include "wallpaper.h"
#include "util.h"
#include "x11.h"

static struct cache *CACHE = 0;
static char CACHE_SPEC[4096] = { '\0' };
static enum wallpaper_type CACHE_TYPE = IMAGE;
static enum wallpaper_mode CACHE_MODE = CENTERED;

static void wallpaper_set_x11 (struct cache_node *node);

static Imlib_Image render_color (const char *color_str);
static Imlib_Image render_image (Imlib_Image image, enum wallpaper_mode mode);
static Imlib_Image render_centered (struct geometry *geometry, Imlib_Image image);
static Imlib_Image render_tiled (struct geometry *geometry, Imlib_Image image);
static Imlib_Image render_fill (struct geometry *geometry, Imlib_Image image);
static Imlib_Image render_zoom (struct geometry *geometry, Imlib_Image image);
static Pixmap render_x11_pixmap (Imlib_Image image);

static Imlib_Image render_new_color (unsigned int width, unsigned int height,
                                     struct color *color);
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
            image_rendered = render_image (image, mode);

            imlib_context_set_image (image);
            imlib_free_image ();
        } else {
            image_rendered = render_color (spec);
        }

        Pixmap pixmap = render_x11_pixmap (image_rendered);
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
    CACHE_MODE = CENTERED;
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
 * Render image for current screen using a single color.
 */
Imlib_Image
render_color (const char *color_str)
{
    struct color color;
    x11_parse_color (color_str, &color);

    struct geometry *disp = x11_get_geometry ();
    Imlib_Image image = render_new_color (disp->width, disp->height, &color);
    mem_free (disp);

    return image;
}

/**
 * Render image for current screen with specified mode.
 */
Imlib_Image
render_image (Imlib_Image image, enum wallpaper_mode mode)
{
    struct geometry *disp = x11_get_geometry ();
    struct color black = { 0, 0, 0 };
    Imlib_Image image_disp = render_new_color (disp->width, disp->height, &black);
    mem_free (disp);

    struct geometry **heads = x11_get_heads ();
    for (int i = 0; heads[i]; i++) {
        Imlib_Image image_head;

        switch (mode) {
        case TILED:
            image_head = render_tiled (heads[i], image);
            break;
        case FILL:
            image_head = render_fill (heads[i], image);
            break;
        case ZOOM:
            image_head = render_zoom (heads[i], image);
            break;
        case CENTERED:
        default:
            image_head = render_centered (heads[i], image);
            break;
        }
       
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
 * Fill image on new image without keeping aspect ratio.
 */
Imlib_Image
render_fill (struct geometry *geometry, Imlib_Image image)
{
    imlib_context_set_image (image);
    int s_width = imlib_image_get_width ();
    int s_height = imlib_image_get_height ();
    
    Imlib_Image image_dest = imlib_create_cropped_scaled_image (
            0, 0, s_width, s_height, geometry->width, geometry->height);

    return image_dest;
}

/**
 * Fill image on new image keeping aspect ratio.
 */
Imlib_Image
render_zoom (struct geometry *geometry, Imlib_Image image)
{
    imlib_context_set_image (image);
    float s_width = imlib_image_get_width ();
    float s_height = imlib_image_get_height ();

    float s_aspect = s_width / s_height;
    float d_aspect = (float) geometry->width / geometry->height;

    int d_width, d_height;
    if (s_aspect > d_aspect) {
        d_width = geometry->height * (s_width / s_height);
        d_height = geometry->height;
    } else {
        d_width = geometry->width;
        d_height = geometry->width * (s_height / s_width);
    }

    Imlib_Image image_zoom = imlib_create_cropped_scaled_image (
            0, 0, s_width, s_height, d_width, d_height);

    Imlib_Image image_dest = render_centered (geometry, image_zoom);

    imlib_context_set_image (image_zoom);
    imlib_free_image ();

    return image_dest;
}

/**
 * Render image centered on geometry sized image.
 */
Imlib_Image
render_centered (struct geometry *geometry, Imlib_Image image)
{
    imlib_context_set_image (image);
    int s_width = imlib_image_get_width ();
    int s_height = imlib_image_get_height ();

    Imlib_Image image_dest = imlib_create_image (
            geometry->width, geometry->height);
    imlib_context_set_image (image_dest);
    imlib_context_set_color (0, 0, 0, 255);
    imlib_image_fill_rectangle (
            0, 0, imlib_image_get_width (), imlib_image_get_height ());

    int dest_x = (geometry->width - s_width) / 2;
    int dest_y = (geometry->height - s_height) / 2;
    imlib_blend_image_onto_image (
            image, 0,
            0, 0, s_width, s_height, dest_x, dest_y, s_width, s_height);

    return image_dest;
}

/**
 * Tile image onto new image.
 */
Imlib_Image
render_tiled (struct geometry *geometry, Imlib_Image image)
{
    imlib_context_set_image (image);
    int s_width = imlib_image_get_width ();
    int s_height = imlib_image_get_height ();

    Imlib_Image image_dest = imlib_create_image (geometry->width,
                                                 geometry->height);
    imlib_context_set_image (image_dest);

    for (int x = 0; x < geometry->width; x += s_width) {
        for (int y = 0; y < geometry->height; y += s_height) {
            imlib_blend_image_onto_image (
                    image, 0, 0, 0, s_width, s_height, x, y, s_width, s_height);
        }
    }

    return image_dest;
}

/**
 * Create Pixmap from from image.
 */
Pixmap
render_x11_pixmap (Imlib_Image image)
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

/**
 * Create new image filled with black of width/height dimensions.
 */
Imlib_Image
render_new_color (unsigned int width, unsigned int height, struct color *color)
{
    Imlib_Image image = imlib_create_image (width, height);

    imlib_context_set_image (image);
    imlib_context_set_color (color->r, color->g, color->b, 255);
    imlib_image_fill_rectangle (0, 0, width, height);

    return image;
}
