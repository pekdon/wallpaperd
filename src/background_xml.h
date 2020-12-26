/*
 * background_xml.h for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _BACKGROUND_XML_H_
#define _BACKGROUND_XML_H_

#include "config.h"

#include <stdio.h>

#include "background.h"

extern struct background_set *background_xml_parse (FILE *fp);

#endif /* _BACKGROUND_XML_H_ */
