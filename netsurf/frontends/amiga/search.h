/*
 * Copyright 2008 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_SEARCH_H
#define AMIGA_SEARCH_H

struct gui_search_table;
struct gui_window;

extern struct gui_search_table *amiga_search_table;

/**
 * Change the displayed search status.
 *
 * \param gwin gui window to open search for.
 */
void ami_search_open(struct gui_window *gwin);

/**
 * Close search
 */
void ami_search_close(void);

/**
 * Obtain gui window associated with find window.
 */
struct gui_window *ami_search_get_gwin(struct find_window *fw);

#endif
