/*
 * Copyright 2017 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * heap fault injection generation.
 *
 * This library inject allocation faults into NetSurf tests
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <dlfcn.h>

#include "test/malloc_fig.h"

static unsigned int count = UINT_MAX;

void malloc_limit(unsigned int newcount)
{
	count = newcount;
	//fprintf(stderr, "malloc_limit %d\n", count);
}

void* malloc(size_t size)
{
	static void* (*real_malloc)(size_t) = NULL;
	void *p = NULL;

	if (real_malloc == NULL) {
		real_malloc = dlsym(RTLD_NEXT, "malloc");
	}

	if (count > 0) {
		p = real_malloc(size);
		count--;
	}
	//fprintf(stderr, "malloc(%d) = %p remian:%d\n", size, p, count);
	return p;
}
