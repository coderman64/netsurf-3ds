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
 * Interface to win32 bookmark manager (hotlist).
 */

#ifndef NETSURF_WINDOWS_HOTLIST_H
#define NETSURF_WINDOWS_HOTLIST_H

/**
 * make the hotlist window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror nsw32_hotlist_present(HINSTANCE hinstance);

/**
 * Free any resources allocated for the hotlist window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror nsw32_hotlist_finalise(void);

#endif /* NETSURF_WINDOWS_HOTLIST_H */
