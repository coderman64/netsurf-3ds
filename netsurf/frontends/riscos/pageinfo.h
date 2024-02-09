/*
 * Copyright 2020 Vincent Sanders <vince@nesurf-browser.org>
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
 * Interface to page info core window for RISC OS
 */

#ifndef NETSURF_RISCOS_PAGEINFO_H_
#define NETSURF_RISCOS_PAGEINFO_H_

struct gui_window;

/**
 * initialise the pageinfo window template ready for subsequent use.
 */
nserror ro_gui_pageinfo_initialise(void);

/**
 * make the pageinfo window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_pageinfo_present(struct gui_window *gw);

/**
 * Free any resources allocated for the page info window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_pageinfo_finalise(void);

#endif
