/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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
 * Windows mouse cursor interface.
 */

#ifndef _NETSURF_WINDOWS_POINTERS_H_
#define _NETSURF_WINDOWS_POINTERS_H_


/**
 * initialise the list of mouse cursors
 */
void nsws_window_init_pointers(HINSTANCE hinstance);

/**
 * get a win32 cursor handle for a pointer shape
 */
HCURSOR nsws_get_pointer(gui_pointer_shape shape);


#endif 
