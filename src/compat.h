/*
 * compat.h for wallpaperd
 * Copyright (C) 2010-2020 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the MIT license.
 * See the LICENSE file for more information.
 */

#ifndef _COMPAT_H_
#define _COMPAT_H_

#include "config.h"

#ifndef MIN
#define MIN(a,b) ((a) > (b)) ? (b) : (a)
#endif /* MIN */

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif /* MAX */

#ifndef HAVE_DAEMON
extern int daemon (int nochdir, int noclose);
#endif /* HAVE_DAEMON */

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t dstsize);
#endif /* HAVE_STRLCAT */

#endif /* _COMPAT_H_ */
