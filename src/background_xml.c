/*
 * background_xml.c for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "background.h"
#include "background_xml.h"
#include "util.h"

#define TAG_VALUE_SIZE 4096

enum _background_xml_state {
    _BACKGROUND_XML_STARTTIME,
    _BACKGROUND_XML_STATIC,
    _BACKGROUND_XML_TRANSITION,
    _BACKGROUND_XML_SKIP
};

static int _read_tag (FILE *fp, char *tag);
static int _read_value (FILE *fp, char *value);

/**
 * Read background XML file from open file. Caller is responsible for
 * cleaning up resources after use by calling background_set_free.
 */
struct background_set*
background_xml_parse (FILE *fp)
{
    char tag[TAG_VALUE_SIZE], value[TAG_VALUE_SIZE], file[TAG_VALUE_SIZE];
    char from[TAG_VALUE_SIZE], to[TAG_VALUE_SIZE];
    unsigned int duration = 0;

    struct background *bg = 0;
    struct background_set *bg_set = background_set_new ();

    enum _background_xml_state state = _BACKGROUND_XML_SKIP;
    while (_read_tag (fp, tag)) {
        if (state == _BACKGROUND_XML_STARTTIME) {
            if (strcasecmp (tag, "hour") == 0) {
                _read_value (fp, value);
                bg_set->hour = atoi (value);
            } else if (strcasecmp (tag, "minute") == 0) {
                _read_value (fp, value);
                bg_set->min = atoi (value);
            } else if (strcasecmp (tag, "second") == 0) {
                _read_value (fp, value);
                bg_set->sec = atoi (value);
            } else if (strcasecmp (tag, "/starttime") == 0) {
                state = _BACKGROUND_XML_SKIP;
            }

        } else if (state == _BACKGROUND_XML_STATIC) {
            if (strcasecmp (tag, "file") == 0) {
                _read_value (fp, file);
            } else if (strcasecmp (tag, "duration") == 0) {
                _read_value (fp, value);
                duration = (int) atof(value);
            } else if (strcasecmp (tag, "/static") == 0) {
                state = _BACKGROUND_XML_SKIP;
                bg = background_set_add_background(bg_set, file, duration);
            }

        } else if (state == _BACKGROUND_XML_TRANSITION) {
            if (strcasecmp (tag, "duration") == 0) {
                _read_value (fp, value);
                duration = atoi (value);
            } else if (strcasecmp (tag, "from") == 0) {
                _read_value (fp, from);
            } else if (strcasecmp (tag, "to") == 0) {
                _read_value (fp, to);
            } else if (strcasecmp (tag, "/transition") == 0) {
                state = _BACKGROUND_XML_SKIP;
                background_set_add_transition (bg_set, bg, from, to, duration);
            }
        } else if (strcasecmp (tag, "static") == 0) {
            state = _BACKGROUND_XML_STATIC;
            file[0] = '\0';
            duration = 0;
        } else if (strcasecmp (tag, "transition") == 0) {
            state = _BACKGROUND_XML_TRANSITION;
        } else if (strcasecmp (tag, "starttime") == 0) {
            state = _BACKGROUND_XML_STARTTIME;
        }
    }

    background_set_update (bg_set);

    return bg_set;
}

int
_read_tag (FILE *fp, char *tag)
{
    int in_tag = 0, tag_pos = 0;

    int c;
    while (tag_pos < TAG_VALUE_SIZE && (c = fgetc (fp)) != EOF) {
        if (in_tag) {
            if (c == '>') {
                tag[tag_pos] = '\0';
                return 1;
            } else {
                tag[tag_pos++] = c;
            }
        } else if (c == '<') {
            in_tag = 1;
        }
    }

    tag[TAG_VALUE_SIZE - 1] = '\0';

    return 0;
}

int
_read_value (FILE *fp, char *value)
{
    int value_pos = 0;

    int c;
    while (value_pos < TAG_VALUE_SIZE && (c = fgetc (fp)) != EOF) {
        if (c == '<') {
            value[value_pos] = '\0';
            return 1;
        } else {
            value[value_pos++] = c;
        }
    }

    value[TAG_VALUE_SIZE - 1] = '\0';

    return 0;
}
