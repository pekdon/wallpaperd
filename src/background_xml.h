/*
 * background_xml.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _BACKGROUND_XML_H_
#define _BACKGROUND_XML_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include "background.h"

extern struct background_set *background_xml_parse (FILE *fp);

#endif /* _BACKGROUND_XML_H_ */
