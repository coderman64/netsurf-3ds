/*
 * Copyright 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_FONT_BULLET_H
#define AMIGA_FONT_BULLET_H
struct ami_font_cache_node;

void ami_font_bullet_init(void);
void ami_font_bullet_fini(void);
void ami_font_bullet_close(void *nso);

/* Alternate entry points into font_scan */
void ami_font_initscanner(bool force, bool save);
void ami_font_finiscanner(void);
void ami_font_savescanner(void);
#endif

