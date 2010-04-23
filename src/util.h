/*
 * util.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

extern void die (const char *msg, ...);

extern void *mem_new (size_t size);
extern void mem_free (void *data);

extern char *expand_home (const char *str);

extern char *str_first_of (char *str, const char *of);
extern char *str_first_not_of (char *str, const char *not_of);
extern int str_starts_with (const char *str, const char *start);
extern int str_ends_with (const char *str, const char *end);

#endif /* _UTIL_H_ */
