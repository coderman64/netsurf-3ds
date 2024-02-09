/*
 * Copyright 2004-2007 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2004 John Tytgat <joty@netsurf-browser.org>
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
 * \brief Interface to a number of general purpose functionality.
 * \todo Many of these functions and macros should have their own headers.
 */

#ifndef NETSURF_UTILS_UTILS_H
#define NETSURF_UTILS_UTILS_H

#include <stdbool.h>

#ifndef NOF_ELEMENTS
#define NOF_ELEMENTS(array) (sizeof(array)/sizeof(*(array)))
#endif

#ifndef ABS
#define ABS(x) (((x)>0)?(x):(-(x)))
#endif

#ifdef __MINT__ /* avoid using GCCs builtin min/max functions */
#undef min
#undef max
#endif

#ifndef __cplusplus
#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif
#endif

/* Windows does not have POSIX mkdir so work around that */
#if defined(_WIN32)
/** windows mkdir function */
#define nsmkdir(dir, mode) mkdir((dir))
#else
/** POSIX mkdir function */
#define nsmkdir(dir, mode) mkdir((dir), (mode))
#endif

#if defined(__GNUC__) && (__GNUC__ < 3)
#define FLEX_ARRAY_LEN_DECL 0
#else
#define FLEX_ARRAY_LEN_DECL 
#endif

#if defined(__HAIKU__) || defined(__BEOS__)
#include <stdlib.h>
#define strtof(s,p) ((float)(strtod((s),(p))))
#endif

#if !defined(ceilf) && defined(__MINT__)
#define ceilf(x) (float)ceil((double)x)
#endif

/**
 * Calculate length of constant C string.
 *
 * \param  x a constant C string.
 * \return The length of C string without its terminator.
 */
#define SLEN(x) (sizeof((x)) - 1)


/**
 * Check if a directory exists.
 */
bool is_dir(const char *path);

#endif
