/*
 * cfg.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _CFG_H_
#define _CFG_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "background.h"
#include "wallpaperd.h"

/**
 * Single node used internally in the configuration struct.
 */
struct cfg_node {
    const char *key;
    const char *value;

    struct cfg_node *next;
};

/**
 * Parsed configuration structure.
 */
struct config {
    char *path;
    char *pid_path;

    enum bg_select_mode bg_select_mode;
    struct background_set *bg_set;
    
    long bg_interval;
    char **_search_path;

    struct cfg_node *first;
    struct cfg_node *last;
};

extern struct config *cfg_new (void);
extern void cfg_free (struct config *config);
extern int cfg_load (struct config *config, const char *path);
extern char *cfg_get_path (void);

extern const char *cfg_get (struct config *config, const char *key);
extern char **cfg_get_search_path (struct config *config);
extern const char *cfg_get_wallpaper (struct config *config, long desktop);
extern enum wallpaper_mode cfg_get_mode (struct config *config, long desktop);

#endif /* _CFG_H_ */
