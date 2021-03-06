/*
 * util.h for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include "config.h"

#include <string.h>
#include <stdbool.h>

extern void die (const char *msg, ...);

extern void *mem_new (size_t size);
extern void mem_free (void *data);

extern bool file_exists (const char *path);

extern char *expand_abs (const char *path);
extern char *expand_home (const char *str);

extern char *str_dup (const char *str);
extern const char *str_first_of (const char *str, const char *of);
extern const char *str_first_not_of (const char *str, const char *not_of);
extern int str_starts_with (const char *str, const char *start);
extern int str_ends_with (const char *str, const char *end);

#endif /* _UTIL_H_ */
