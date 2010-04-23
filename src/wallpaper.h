/*
 * wallpaper.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _WALLPAPER_H_
#define _WALLPAPER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/**
 * Supported background modes.
 */
enum wallpaper_mode {
    CENTERED,
    TILED,
    FILL,
    ZOOM
};

extern void wallpaper_set (const char *path, enum wallpaper_mode mode);
extern enum wallpaper_mode wallpaper_mode_from_str (const char *str);
extern void wallpaper_cache_clear (int do_alloc);

#endif /* _WALLPAPER_H_ */
