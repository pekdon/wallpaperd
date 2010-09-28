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

#include "wallpaperd.h"

extern void wallpaper_set (const char *spec,
                           enum wallpaper_type type, enum wallpaper_mode mode);
extern void wallpaper_cache_clear (int do_alloc);

#endif /* _WALLPAPER_H_ */
