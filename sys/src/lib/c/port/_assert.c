/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

void (*__assert)(char*);

void
_assert(const char *s)
{
	if(__assert)
		(*__assert)((char*)s);
	fprint(2, "assert failed: %s\n", s);
	abort();
	__builtin_unreachable();
}
