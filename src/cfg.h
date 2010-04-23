
#ifndef _CFG_H_
#define _CFG_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

struct cfg_node {
    const char *key;
    const char *value;

    struct cfg_node *next;
};

struct config {
    const char *path;
    struct cfg_node *first;
    struct cfg_node *last;
};

extern struct config *cfg_load (const char *path);
extern char *cfg_get_path (void);

extern const char *cfg_get (struct config *config, const char *key);
extern const char *cfg_get_wallpaper (struct config *config, long desktop);
extern const char *cfg_get_mode (struct config *config, long desktop);

#endif /* _CFG_H_ */
