
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/**
 * Print message and exit with status 1;
 */
void
die (const char *msg)
{
    fprintf (stderr, "%s", msg);
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
        die ("memory allocation failed, aborting!");
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
            die ("failed to expand home directory in path");
        }
        return expanded_str;
    } else {
        return strdup (str);
    }
}
