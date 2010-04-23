
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
    const char *path;
    enum wallpaper_mode mode;
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
                                            const char *path,
                                            enum wallpaper_mode mode);
extern struct cache_node *cache_set_pixmap (struct cache *cache,
                                            const char *path,
                                            enum wallpaper_mode mode,
                                            Pixmap pixmap);

#endif /* _CACHE_H_ */
