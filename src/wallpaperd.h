/*
 * wallpaperd.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _WALLPAPERD_H_
#define _WALLPAPERD_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "cfg.h"

/**
 * Command line options structure.
 */
struct options {
    int help;
    int foreground;
    int stop;
};

extern struct options *OPTIONS;
extern struct config *CONFIG;

#endif /* _WALLPAPERD_H_ */
