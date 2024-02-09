/*
 * Copyright 2005 Richard Wilson <info@tinct.net>
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
 * RISc OS global history interface.
 */

#ifndef RISCOS_GLOBALHISTORY_H
#define RISCOS_GLOBALHISTORY_H

/**
 * initialise the global history window template ready for subsequent use.
 */
void ro_gui_global_history_initialise(void);

/**
 * make the global history window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_global_history_present(void);

/**
 * Free any resources allocated for the global history window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_global_history_finalise(void);

/**
 * check if window handle is for the global history window
 */
bool ro_gui_global_history_check_window(wimp_w window);

/**
 * check if menu handle is for the global history menu
 */
bool ro_gui_global_history_check_menu(wimp_menu *menu);

#endif

