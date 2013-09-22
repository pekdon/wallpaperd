/*
 * wallpaper.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _RENDER_H_
#define _RENDER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <Imlib2.h>

#include "wallpaperd.h"
#include "x11.h"

extern Imlib_Image render_color (struct geometry *geometry,
                                 const char *color_str);
extern Imlib_Image render_image (struct geometry *geometry,
                                 Imlib_Image image, enum wallpaper_mode mode);
extern Imlib_Image render_centered (struct geometry *geometry, Imlib_Image image);
extern Imlib_Image render_tiled (struct geometry *geometry, Imlib_Image image);
extern Imlib_Image render_fill (struct geometry *geometry, Imlib_Image image);
extern Imlib_Image render_zoom (struct geometry *geometry, Imlib_Image image);
extern Imlib_Image render_scaled (struct geometry *geometry, Imlib_Image image);
extern Imlib_Image render_new_color (unsigned int width, unsigned int height,
                                     struct color *color);

#endif /* _RENDER_H_ */
