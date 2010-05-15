/*
 * background.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include <time.h>

#include "background.h"
#include "util.h"

/**
 * Create new background_set.
 */
extern struct background_set*
background_set_new (void)
{
    struct background_set *bg_set = mem_new (sizeof (struct background_set));

    bg_set->time = time (0);
    bg_set->duration = 0;
    // FIXME: Calculate offset

    bg_set->start_hour = 0;
    bg_set->start_minute = 0;
    bg_set->start_second = 0;

    bg_set->bg_curr = 0;
    bg_set->bg_first = 0;

    return bg_set;
}

/**
 * Free resources used by background_set including backgrounds.
 */
void
background_set_free (struct background_set *bg_set)
{
    struct background *bg_it = bg_set->bg_first, *bg_it_next;
    for (; bg_it != 0; bg_it = bg_it_next) {
        bg_it_next = bg_it->next;
        mem_free (bg_it->path);
        mem_free (bg_it);
    }
    mem_free (bg_set);
}

/**
 * Get image from background set to be used at this point in time.
 */
struct background*
background_set_get_now (struct background_set *bg_set)
{
    struct background *bg = 0;

    if (! bg_set->bg_first) {
        /* Nothing to do, no backgrounds. */
    } else if (! bg_set->bg_first->next) {
        bg = bg_set->bg_first;
    } else {
        int time_elapsed = time (0) - bg_set->time;
        while (time_elapsed >= 0)  {
            if (bg) {
                bg = bg->next;
            }
            if (! bg) {
                bg = bg_set->bg_first;
            }
            time_elapsed -= bg->duration;
        }
    }

    bg_set->duration = bg->duration;
    bg_set->bg_curr = bg;

    return bg_set->bg_curr;
}

/**
 * Add background to background_set.
 */
void
background_set_add_background (struct background_set *bg_set,
                               const char *path, unsigned int duration)
{
    struct background *bg = mem_new (sizeof (struct background));

    bg->path = str_dup (path);
    bg->duration = duration;
    bg->next = 0;

    if (bg_set->bg_first) {
        struct background *bg_it = bg_set->bg_first;
        for (; bg_it->next; bg_it = bg_it->next)
            ;
        bg_it->next = bg;
    } else {
        bg_set->bg_first = bg;
    }
}
