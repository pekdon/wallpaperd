/*
 * compat.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _COMPAT_H_
#define _COMPAT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

#endif /* _COMPAT_H_ */
