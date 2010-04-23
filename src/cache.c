
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <Imlib2.h>

#include "cache.h"
#include "util.h"

/**
 * Create new cache node.
 */
struct cache_node*
cache_node_new (const char *path, enum wallpaper_mode mode, Pixmap pixmap)
{
    struct cache_node *node = mem_new (sizeof (struct cache_node));
    node->path = strdup (path);
    node->mode = mode;
    node->pixmap = pixmap;
    node->next = 0;
    return node;
}

/**
 * Free up resources used by cache node.
 */
void
cache_node_free (struct cache_node *node)
{
    imlib_free_pixmap_and_mask (node->pixmap);
    mem_free (node->path);
    mem_free (node);
}

/**
 * Create new cache structure.
 */
struct cache*
cache_new (void)
{
    struct cache *cache = mem_new (sizeof (struct cache));
    cache->first = 0;
    cache->last = 0;
    return cache;
}

/**
 * Free resources used by cache.
 */
void
cache_free (struct cache *cache)
{
    struct cache_node *it = cache->first;
    for (; it; it = it->next) {
        cache_node_free (it);
    }
    mem_free (cache);
}

/**
 * Get pixmap from cache.
 */
struct cache_node*
cache_get_pixmap (struct cache *cache,
                  const char *path, enum wallpaper_mode mode)
{
    struct cache_node *it = cache->first;
    for (; it != 0; it = it->next) {
        if (mode == it->mode && ! strcmp (path, it->path)) {
            return it;
        }
    }
    return 0;
}

/**
 * Add pixmap to cache.
 */
struct cache_node*
cache_set_pixmap (struct cache *cache,
                  const char *path, enum wallpaper_mode mode, Pixmap pixmap)
{
    struct cache_node *node = cache_node_new (path, mode, pixmap);
    if (cache->last) {
        cache->last->next = node;
        cache->last = node;
    } else {
        cache->first = cache->last = node;
    }
    return node;
}
