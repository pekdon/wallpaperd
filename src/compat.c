/*
 * compat.c for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compat.h"

#ifndef HAVE_DAEMON
/**
 * daemon compatibility call.
 */
int
daemon (int nochdir, int noclose)
{
    pid_t pid = fork ();
    if (pid == -1) {
        /* Failed fork, abort. */
        return -1;
    } else if (pid == 0) {
        /* Child process. */
        pid_t session = setsid ();
        if (session == -1) {
            perror ("failed to setsid, aborting");
            exit (1);
        }

        if (! nochdir) {
            if (chdir ("/") == -1) {
                perror ("failed to change directory to /");
            }
        }
        if (! noclose) {
            close (0);
            close (1);
            close (2);
        }

    } else {
        /* Parent, exit process. */
        _exit (2);
    }

    return 0;
}
#endif /* HAVE_DAEMON */

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
