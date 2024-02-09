/*
 * Copyright 2017 Vincent Sanders <vince@netsurf-browser.org>
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
 * RISC OS local history interface.
 */

#ifndef RISCOS_LOCALHISTORY_H
#define RISCOS_LOCALHISTORY_H

/**
 * initialise the local history window template ready for subsequent use.
 */
void ro_gui_local_history_initialise(void);

/**
 * make the local history window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_local_history_present(wimp_w parent, struct browser_window *bw);

/**
 * Free any resources allocated for the local history window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_local_history_finalise(void);

#endif
