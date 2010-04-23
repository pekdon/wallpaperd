
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
static Imlib_Image render_zoom (Imlib_Image image);
static Imlib_Image render_centered (Imlib_Image image);
static Imlib_Image render_tiled (Imlib_Image image);

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
        /* FIXME: Add free for image_rendered when render actually
                  does something. */
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
    } else if (! strcasecmp (str, "ZOOM")) {
        mode = ZOOM;
    } else if (! strcasecmp (str, "TILED")) {
        mode = TILED;
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

    switch (mode) {
    case CENTERED:
        image_rendered = render_centered (image);
        break;
    case ZOOM:
        image_rendered = render_zoom (image);
        break;
    case TILED:
        image_rendered = render_tiled (image);
        break;
    }

    return image_rendered;
}

Imlib_Image
render_zoom (Imlib_Image image)
{
    return image;
}

Imlib_Image
render_centered (Imlib_Image image)
{
    return image;
}

Imlib_Image
render_tiled (Imlib_Image image)
{
    return image;
}

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
