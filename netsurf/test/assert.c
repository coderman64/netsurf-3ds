/*
 * Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>
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
 * Hack for assertion coverage output
 */

/* Bring in the real __assert_fail */
#include <assert.h>

/* This is what everyone else calls */
extern void
__ns_assert_fail(const char *__assertion, const char *__file,
		 unsigned int __line, const char *__function)
	__THROW __attribute__ ((__noreturn__));

/* We use this to flush coverage data */
extern void __gcov_flush(void);

/* And here's our entry point */
void
__ns_assert_fail(const char *__assertion, const char *__file,
		 unsigned int __line, const char *__function)
{
	__gcov_flush();
	__assert_fail(__assertion, __file, __line, __function);
}
