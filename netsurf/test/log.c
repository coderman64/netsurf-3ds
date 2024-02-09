/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
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
 * Minimal unit test log implementation.
 *
 * It is necessary to have a logging implementation for the unit tests
 * so other netsurf modules that assume this functionality work.
 */

#include <stdarg.h>
#include <stdio.h>

#include "utils/log.h"

/** flag to enable verbose logging */
bool verbose_log = false;

nserror nslog_init(nslog_ensure_t *ensure, int *pargc, char **argv)
{
	return NSERROR_OK;
}


void nslog_log(const char *file, const char *func, int ln, const char *format, ...)
{
	va_list ap;

	if (verbose_log) {
		fprintf(stderr, "%s:%i %s: ", file, ln, func);

		va_start(ap, format);

		vfprintf(stderr, format, ap);

		va_end(ap);

		fputc('\n', stderr);
	}
}
