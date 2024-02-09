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
 * Interface to win32 global history manager using nsw32 core window
 */

#ifndef NETSURF_WINDOWS_GLOBAL_HISTORY_H
#define NETSURF_WINDOWS_GLOBAL_HISTORY_H

/**
 * make the global history window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror nsw32_global_history_present(HINSTANCE hinstance);

/**
 * Destroys the global history window and performs any other necessary cleanup
 * actions.
 */
nserror nsw32_global_history_finalise(void);

#endif
