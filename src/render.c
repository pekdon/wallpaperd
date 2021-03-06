/*
 * wallpaper.c for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include "config.h"

#include <stdio.h>
#include <Imlib2.h>

#include "render.h"
#include "util.h"

/**
 * Render image for current screen using a single color.
 */
Imlib_Image
render_color (struct geometry *geometry, const char *color_str)
{
    struct color color;
    x11_parse_color (color_str, &color);

    Imlib_Image image =
        render_new_color (geometry->width, geometry->height, &color);

    return image;
}

/**
 * Render image for current screen with specified mode.
 */
Imlib_Image
render_image (struct geometry *geometry,
              const char *path, enum wallpaper_mode mode)
{
    Imlib_Image image = imlib_load_image (path);
    if (! image) {
        fprintf (stderr, "failed to load %s\n", path);
        return NULL;
    }

    Imlib_Image image_rendered;
    switch (mode) {
    case MODE_TILED:
        image_rendered = render_tiled (geometry, image);
        break;
    case MODE_FILL:
        image_rendered = render_fill (geometry, image);
        break;
    case MODE_ZOOM:
        image_rendered = render_zoom (geometry, image);
        break;
    case MODE_SCALED:
        image_rendered = render_scaled (geometry, image);
        break;
    case MODE_CENTERED:
    default:
        image_rendered = render_centered (geometry, image);
        break;
    }

    imlib_context_set_image (image);
    imlib_free_image ();

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
 * Scale image on new image keeping aspect ratio.
 */
Imlib_Image
render_scaled (struct geometry *geometry, Imlib_Image image)
{
    imlib_context_set_image (image);
    float s_width = imlib_image_get_width ();
    float s_height = imlib_image_get_height ();

    float s_aspect = s_width / s_height;
    float d_aspect = (float) geometry->width / geometry->height;

    int d_width, d_height;
    if (s_aspect < d_aspect) {
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
