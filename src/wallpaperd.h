/*
 * wallpaperd.h for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _WALLPAPERD_H_
#define _WALLPAPERD_H_

#include "config.h"

/**
 * Background image selection mode
 */
enum bg_select_mode {
    MODE_NUMBER,
    MODE_NAME,
    MODE_RANDOM,
    MODE_SET,
    MODE_STATIC
};

/**
 * Supported background types.
 */
enum wallpaper_type {
    WALLPAPER_TYPE_UNKNOWN,
    WALLPAPER_TYPE_COLOR,
    WALLPAPER_TYPE_IMAGE
};

/**
 * Supported background modes.
 */
enum wallpaper_mode {
    MODE_UNKNOWN,
    MODE_CENTERED,
    MODE_TILED,
    MODE_FILL,
    MODE_ZOOM,
    MODE_SCALED
};

/**
 * Command line options structure.
 */
struct options {
    int help;
    int foreground;
    int stop;
    char *image;
    enum wallpaper_mode mode;
    const char *workspace;
};

#include "cfg.h"

extern struct options *OPTIONS;
extern struct config *CONFIG;

#ifdef HAVE_ARC4RANDOM
#define rand_init(seed) { }
#define rand_next arc4random
#else /* !HAVE_ARC4RANDOM */
#define rand_init(seed) srandom((seed))
#define rand_next random
#endif /* HAVE_ARC4RANDOM */

#endif /* _WALLPAPERD_H_ */
