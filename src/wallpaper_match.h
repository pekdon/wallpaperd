/*
 * wallpaper_match.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _WALLPAPER_MATCH_H_
#define _WALLPAPER_MATCH_H_

#include "config.h"

#include "wallpaperd.h"

/**
 * Filter used to select wallpaper from configuration.
 */
struct wallpaper_filter {
    enum bg_select_mode mode; /**< Search mode. */
    int desktop; /**< Workspace number. */
    const char *desktop_name; /**< Name of the workspace. */
    int head; /**< xrandr screen/output. */
};

/**
 * Image specification.
 */
struct wallpaper_spec {
    enum wallpaper_type type;
    enum wallpaper_mode mode;
    char *spec;
};

struct wallpaper_spec *wallpaper_match (struct wallpaper_filter *filter);
void wallpaper_spec_free (struct wallpaper_spec *spec);

#endif /* _WALLPAPER_MATCH_H_ */
