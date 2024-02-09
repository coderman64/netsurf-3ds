/*
 * Copyright 2018 Vincent Sanders <vince@nexturf-browser.org>
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

#include <stdio.h>
#include <stdarg.h>

#include "monkey/output.h"

/**
 * output type prefixes
 */
static const char *type_text[]={
	"DIE",
	"ERROR",
	"WARN",
	"GENERIC",
	"WINDOW",
	"LOGIN",
	"DOWNLOAD",
	"PLOT",
};

/* exported interface documented in monkey/output.h */
int moutf(enum monkey_output_type mout_type, const char *fmt, ...)
{
	va_list ap;
	int res;

	res = fprintf(stdout, "%s ", type_text[mout_type]);

	va_start(ap, fmt);
	res += vfprintf(stdout, fmt, ap);
	va_end(ap);

	fputc('\n', stdout);

	return res + 1;
}
