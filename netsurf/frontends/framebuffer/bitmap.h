/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.h>
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

#ifndef NS_FB_BITMAP_H
#define NS_FB_BITMAP_H

extern struct gui_bitmap_table *framebuffer_bitmap_table;

bool framebuffer_bitmap_get_opaque(void *bitmap);

#endif /* NS_FB_BITMAP_H */
