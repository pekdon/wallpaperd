
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <strings.h>
#include <Imlib2.h>

#include "cache.h"
#include "wallpaper.h"
#include "util.h"
#include "x11.h"

static struct cache *CACHE;

static void wallpaper_set_x11 (struct cache_node *node);

static Imlib_Image render_image (Imlib_Image image, enum wallpaper_mode mode);
static Imlib_Image render_centered (struct geometry *geometry, Imlib_Image image);
static Imlib_Image render_tiled (struct geometry *geometry, Imlib_Image image);
static Imlib_Image render_fill (struct geometry *geometry, Imlib_Image image);
static Imlib_Image render_zoom (struct geometry *geometry, Imlib_Image image);
static Pixmap render_x11_pixmap (Imlib_Image image);

/**
 * Set wallpaper from image path.
 */
void
wallpaper_set (const char *path, enum wallpaper_mode mode)
{
    if (! CACHE) {
        CACHE = cache_new (); /* FIXME: Initialize elsewhere? */
    }

    struct cache_node *node = cache_get_pixmap (CACHE, path, mode);
    if (! node) {
        Imlib_Image image = imlib_load_image (path);
        Imlib_Image image_rendered = render_image (image, mode);
        Pixmap pixmap = render_x11_pixmap (image_rendered);
        imlib_context_set_image (image);
        imlib_free_image ();
        imlib_context_set_image (image_rendered);
        imlib_free_image ();
        node = cache_set_pixmap (CACHE, path, mode, pixmap);
    }
    wallpaper_set_x11 (node);
}

/**
 * Return wallpaper mode from string, defaults to CENTERED if parsing
 * fails.
 */
enum wallpaper_mode
wallpaper_mode_from_str (const char *str)
{
    enum wallpaper_mode mode = CENTERED;

    if (! str) {
    } else if (! strcasecmp (str, "CENTERED")) {
        mode = CENTERED;
    } else if (! strcasecmp (str, "TILED")) {
        mode = TILED;
    } else if (! strcasecmp (str, "FILL")) {
        mode = FILL;
    } else if (! strcasecmp (str, "ZOOM")) {
        mode = ZOOM;
    }

    return mode;
}

/**
 * Invalidate all cache data.
 */
void
wallpaper_cache_clear (void)
{
    cache_free (CACHE);
    CACHE = cache_new ();
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
        x11_set_background_pixmap (x11_get_root_window (), node->pixmap);
    }
}

Imlib_Image
render_image (Imlib_Image image, enum wallpaper_mode mode)
{
    Imlib_Image image_rendered;

    struct geometry *geometry = x11_get_geometry ();

    switch (mode) {
    case CENTERED:
        image_rendered = render_centered (geometry, image);
        break;
    case TILED:
        image_rendered = render_tiled (geometry, image);
        break;
    case FILL:
        image_rendered = render_fill (geometry, image);
        break;
    case ZOOM:
        image_rendered = render_zoom (geometry, image);
        break;

    }

    mem_free (geometry);

    return image_rendered;
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
    
    Imlib_Image image_dest = imlib_create_cropped_scaled_image (0, 0, 
                                                                s_width, s_height,
                                                                geometry->width, geometry->height);

    return image_dest;
}

/**
 * Fill image on new image keeping aspect ratio.
 */
Imlib_Image
render_zoom (struct geometry *geometry, Imlib_Image image)
{
    imlib_context_set_image (image);
    int s_width = imlib_image_get_width ();
    int s_height = imlib_image_get_height ();

    float s_aspect = (float) s_width / s_height;
    float d_aspect = (float) geometry->width / geometry->height;

    int d_width, d_height;
    if (s_aspect > d_aspect) {
        d_width = geometry->width / s_aspect * d_aspect;
        d_height = geometry->height;
    } else {
        d_width = geometry->width;
        d_height = geometry->height / s_aspect * d_aspect;
    }

    Imlib_Image image_zoom = imlib_create_cropped_scaled_image (0, 0, 
                                                                s_width, s_height,
                                                                d_width, d_height);

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

    Imlib_Image image_dest = imlib_create_image (geometry->width,
                                                 geometry->height);
    imlib_context_set_image (image_dest);
    imlib_context_set_color (0, 0, 0, 255);
    imlib_image_fill_rectangle (0, 0,
                                imlib_image_get_width (),
                                imlib_image_get_height ());


    int dest_x = (geometry->width - s_width) / 2;
    int dest_y = (geometry->height - s_height) / 2;
    imlib_blend_image_onto_image (image, 0,
                                  0, 0, s_width, s_height,
                                  dest_x, dest_y, s_width, s_height);

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
            imlib_blend_image_onto_image (image, 0,
                                          0, 0, s_width, s_height,
                                          x, y, s_width, s_height);

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
