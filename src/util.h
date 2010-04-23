
#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

extern void die (const char *msg);

extern void *mem_new (size_t size);
extern void mem_free (void *data);

extern char *expand_home (const char *str);

#endif /* _UTIL_H_ */
