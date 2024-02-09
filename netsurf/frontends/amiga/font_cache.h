/*
 * Copyright 2015 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_FONT_CACHE_H
#define AMIGA_FONT_CACHE_H

#include <proto/timer.h>

struct ami_font_cache_node
{
#ifdef __amigaos4__
	struct SkipNode skip_node;
#endif
	struct OutlineFont *font;
	char *restrict bold;
	char *restrict italic;
	char *restrict bolditalic;
	struct TimeVal lastused;
};


/* locate an entry in the font cache, NULL if not found */
struct ami_font_cache_node *ami_font_cache_locate(const char *font);

/* allocate a cache entry */
struct ami_font_cache_node *ami_font_cache_alloc_entry(const char *font);

/* insert a cache entry into the list (OS3) */
void ami_font_cache_insert(struct ami_font_cache_node *nodedata, const char *font);

/* initialise the cache */
void ami_font_cache_init(void);

/* cache clean-up */
void ami_font_cache_fini(void);

#endif


