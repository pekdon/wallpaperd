/*
 * util.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#define _GNU_SOURCE

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/**
 * Print message and exit with status 1;
 */
void
die (const char *msg, ...)
{
    va_list ap;
    va_start (ap, msg);
    vfprintf (stderr, msg, ap);
    va_end (ap);
    
    fprintf (stderr, "\n");
    
    exit (1);
}

/**
 * Wrapper for malloc, aborts if memory allocation fails.
 */
void*
mem_new (size_t size)
{
    void *ptr = malloc (size);
    if (ptr == 0) {
        die ("memory allocation of %d bytes failed, aborting!", size);
    }
    return ptr;
}

/**
 * Wrapper for free, sets the pointer to null.
 */
void
mem_free (void *data)
{
    if (data != 0) {
        free (data);
    }
}

/**
 * Expand ~ in path to user home directory.
 */
char*
expand_home (const char *str)
{
    if (str[0] == '~') {
        const char *home = getenv ("HOME");
        char *expanded_str;
        if (asprintf(&expanded_str, "%s/%s", home, str + 1) == -1) {
            die ("failed to expand home directory in path %s", str);
        }
        return expanded_str;
    } else {
        return str_dup (str);
    }
}

/**
 * Wrapper for strdup, aborts if memory allocation fails.
 */
char*
str_dup (const char *str_in)
{
    char *str = strdup (str_in);
    if (str == 0) {
        die ("strdup of %d bytes string failed, aborting!", strlen (str_in));
    }
    return str;
}

/**
 * Find first occurance of character in of.
 */
char*
str_first_of (char *str, const char *of)
{
    char *p, *c;
    for (p = str; *p != '\0'; p++) {
        for (c = (char*) of; *c != '\0'; c++) {
            if (*p == *c) {
                return p;
            }
        }
    }
    return 0;
}

/**
 * Return pointer to first position in string not being one of the
 * specified characters.
 */
char*
str_first_not_of (char *str, const char *not_of)
{
    char *p, *c;
    for (p = str; *p != '\0'; p++) {
        for (c = (char*) not_of; *c != '\0'; c++) {
            if (*p == *c) {
                continue;
            }
        }
        return p;
    }
    return 0;
}

/**
 * Check if str starts with start.
 */
int
str_starts_with (const char *str, const char *start)
{
    size_t start_len = strlen (start);
    return strncmp (str, start, start_len) == 0;
}

/**
 * Check if str ends with end.
 */
int
str_ends_with (const char *str, const char *end)
{
    size_t str_len = strlen (str);
    size_t end_len = strlen (end);

    if (str_len < end_len) {
        return 0;
    } else {
        return strcmp (str + str_len - end_len, end) == 0;
    }

}
