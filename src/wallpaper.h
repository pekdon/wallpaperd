/*
 * wallpaper.h for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _WALLPAPER_H_
#define _WALLPAPER_H_

#include "config.h"

#include "wallpaperd.h"
#include "wallpaper_match.h"

extern void wallpaper_set (struct wallpaper_filter *filter);
extern void wallpaper_cache_clear (int do_alloc);

#endif /* _WALLPAPER_H_ */
