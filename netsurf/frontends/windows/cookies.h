/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org> 
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
 * Interface to win32 cookie viewing using nsw32 core windows.
 *
 * The interface assumes there is only a single cookie window which is
 * presented (shown) when asked for and hidden by usual toolkit
 * mechanics.
 *
 * The destructor is called once during browser shutdown
 */

#ifndef NETSURF_WINDOWS_COOKIES_H
#define NETSURF_WINDOWS_COOKIES_H

/**
 * make the cookie window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror nsw32_cookies_present(const char *search_term);

/**
 * Free any resources allocated for the cookie window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror nsw32_cookies_finalise(void);

#endif /* NETSURF_WINDOWS_COOKIES_H */
