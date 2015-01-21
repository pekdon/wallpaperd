/*
 * util.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define _GNU_SOURCE

#include <sys/param.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
 * Check if path exists.
 */
bool
file_exists (const char *path)
{
    struct stat st_buf;
    int err = stat(path, &st_buf);
    return err ? false : true;
}

/**
 * Get absolute path.
 */
char*
expand_abs (const char *path)
{
    if (path[0] == '/') {
        return str_dup (path);
    }

    char cwd[MAXPATHLEN];
    getcwd(cwd, MAXPATHLEN);

    char *path_abs;
    asprintf(&path_abs, "%s/%s",
             cwd, str_starts_with (path, "./") ? path + 2 : path);

    return path_abs;
}

/**
 * Expand ~ in path to user home directory.
 */
char*
expand_home (const char *str)
{
    if (str[0] == '~') {
        size_t len;
        char *expanded_str;
        const char *home = getenv ("HOME");
        const char *sep = "/";

        if (home == 0 || strlen(home) == 0) {
            fprintf (stderr, "HOME environment variable not set, using current directory");
            home = ".";
        }

        len = strlen(home);
        if (home[len-1] == '/') {
            sep = "";
        }
        if (str[1] == '/') {
            str += 1;
        }

        if (asprintf (&expanded_str, "%s%s%s", home, sep, str + 1) == -1) {
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
const char*
str_first_of (const char *str, const char *of)
{
    const char *p, *c;
    for (p = str; *p != '\0'; p++) {
        for (c = of; *c != '\0'; c++) {
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
const char*
str_first_not_of (const char *str, const char *not_of)
{
    int found;
    const char *p, *c;
    for (p = str; *p != '\0'; p++) {
        found = 0;
        for (c = not_of; *c != '\0'; c++) {
            if (*p == *c) {
                found = 1;
                break;
            }
        }

        if (! found) {
            return p;
        }
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

#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t dstsize)
{
    size_t dst_len = strlen(dst);
    size_t avail = dstsize - dst_len - 1;
    size_t cpy_len = MIN(avail, dst_len);
    if (cpy_len > 0) {
        memcpy(dst + dst_len, src, cpy_len);
        dst[dst_len + cpy_len] = '\0';
    }
    return dst_len + strlen(src);
}
#endif /* HAVE_STRLCAT */
