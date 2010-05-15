/*
 * background.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _BACKGROUND_H_
#define _BACKGROUND_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/**
 * Single background, part of a background set.
 */
struct background {
    char *path;
    unsigned int duration;

    struct background *next;
};

/**
 * Background set, used to display a series of images with a duration
 * relative to the current time.
 */
struct background_set {
    unsigned int time;
    unsigned int duration;

    unsigned int start_hour;
    unsigned int start_minute;
    unsigned int start_second;

    struct background *bg_curr;
    struct background *bg_first;
};

extern struct background_set *background_set_new (void);
extern void background_set_free (struct background_set *bg_set);

extern struct background *background_set_get_now (struct background_set *bg_set);

extern void background_set_add_background (struct background_set *bg_set,
                                           const char *path,
                                           unsigned int duration);

#endif /* _BACKGROUND_H_ */
