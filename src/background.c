/*
 * background.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include <stdio.h>
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

    bg_set->hour = 0;
    bg_set->min = 0;
    bg_set->sec = 0;

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
    struct background *bg = bg_set->bg_first;

    if (! bg_set->bg_first) {
        /* Nothing to do, no backgrounds. */
    } else if (! bg_set->bg_first->next) {
        bg = bg_set->bg_first;
    } else {
        int time_elapsed = time (0) - bg_set->time;
        do {
            time_elapsed -= bg->duration + bg->transition;
            if (time_elapsed >= 0) {
                if (bg->next) {
                    bg = bg->next;
                } else {
                    /* All image durations have been subtracted, remove
                       total time from start offset to avoid excessive
                       looping. */
                    bg = bg_set->bg_first;
                    bg_set->time -= bg_set->total;
                }
            }
        } while (time_elapsed >= 0);
    }

    /* Set duration to duration + transition until transitions are
       supported */
    if (bg) {
        bg_set->duration = bg->duration + bg->transition;
    }
    bg_set->bg_curr = bg;

    return bg_set->bg_curr;
}

/**
 * Add background to background_set.
 */
struct background*
background_set_add_background (struct background_set *bg_set,
                               const char *path, unsigned int duration)
{
    struct background *bg = mem_new (sizeof (struct background));

    bg->path = str_dup (path);
    bg->duration = duration;
    bg->transition = 0;
    bg->next = 0;

    if (bg_set->bg_first) {
        struct background *bg_it = bg_set->bg_first;
        for (; bg_it->next; bg_it = bg_it->next)
            ;
        bg_it->next = bg;
    } else {
        bg_set->bg_first = bg;
    }

    return bg;
}

/**
 * Add transition time to first matching background, looking at the
 * provided background first if next is 0.
 */
void
background_set_add_transition (struct background_set *bg_set,
                               struct background *bg,
                               const char *from, const char *to,
                               unsigned int duration)
{
    /* Start with checking the provided background, if from matches
       and next is not set assume transition comes before the next
       image. */
    if (strcmp (bg->path, from) == 0 && ! bg->next) {
        bg->transition = duration;
    } else {
        for (bg = bg_set->bg_first; bg && bg->next; bg = bg->next) {
            if (strcmp (bg->path, from) == 0
                && strcmp (bg->next->path, to) == 0) {
                bg->transition = duration;
                break;
            }
        }
    }
}

/**
 * Update offset time elapsed time is calculated from, should be
 * called after adding all images to a background set.
 */
void
background_set_update (struct background_set *bg_set)
{
    /* Count total time. */
    struct background *it = bg_set->bg_first;
    for (bg_set->total = 0; it; it = it->next) {
        bg_set->total += it->duration + it->transition;
    }

    /* Update start time. */
    time_t t = time (0);
    struct tm *tm = localtime (&t);

    unsigned int beg_s = bg_set->hour * 3600 + bg_set->min * 60 + bg_set->sec;
    unsigned int cur_s = tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;

    unsigned int elapsed_s;
    if (beg_s > cur_s) {
        elapsed_s = 86400 - cur_s;
    } else {
        elapsed_s = cur_s - beg_s;
    }

    bg_set->time = time (0) - elapsed_s;
}
