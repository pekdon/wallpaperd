/*
 * compat.h for wallpaperd
 * Copyright (C) 2010 Claes Nästén <me@pekdon.net>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _COMPAT_H_
#define _COMPAT_H_

#include "config.h"

#ifndef HAVE_DAEMON
extern int daemon (int nochdir, int noclose);
#endif /* HAVE_DAEMON */

#endif /* _COMPAT_H_ */
