/*
 * Copyright 2006 Richard Wilson <info@tinct.net>
 * Copyright 2010 Stephen Fryatt <stevef@netsurf-browser.org>
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
 * Interface to riscos cookie viewing using riscos core window.
 *
 * The interface assumes there is only a single cookie window which is
 * presented (shown) when asked for and hidden by usual toolkit
 * mechanics.
 *
 * The destructor is called once during browser shutdown
 */

#ifndef NETSURF_RISCOS_COOKIES_H
#define NETSURF_RISCOS_COOKIES_H

/**
 * initialise the cookies window template ready for subsequent use.
 */
void ro_gui_cookies_initialise(void);

/**
 * make the cookie window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_cookies_present(const char *search_term);

/**
 * Free any resources allocated for the cookie window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_cookies_finalise(void);

/**
 * check if window handle is for the cookies window
 */
bool ro_gui_cookies_check_window(wimp_w window);

/**
 * check if menu handle is for the cookies menu
 */
bool ro_gui_cookies_check_menu(wimp_menu *menu);

#endif /* NETSURF_RISCOS_COOKIES_H */
