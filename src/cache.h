/*
 * cache.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _CACHE_H_
#define _CACHE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <X11/Xlib.h>

#include "wallpaper.h"

/**
 * Single node in the cache structure.
 */
struct cache_node {
    char *spec;
    Pixmap pixmap;

    struct cache_node *next;
};

/**
 * Cache structure.
 */
struct cache {
    struct cache_node *first;
    struct cache_node *last;
};


extern struct cache *cache_new (void);
extern void cache_free (struct cache *cache);

extern struct cache_node *cache_get_pixmap (struct cache *cache,
                                            const char *spec);
extern struct cache_node *cache_set_pixmap (struct cache *cache,
                                            const char *spec,
                                            Pixmap pixmap);

#endif /* _CACHE_H_ */
