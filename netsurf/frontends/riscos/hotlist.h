/*
 * Copyright 2006 Richard Wilson <info@tinct.net>
 * Copyright 2010, 2013 Stephen Fryatt <stevef@netsurf-browser.org>
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

/** \file
 * Hotlist (interface).
 */

#ifndef _NETSURF_RISCOS_HOTLIST_H_
#define _NETSURF_RISCOS_HOTLIST_H_

/* Hotlist Protocol Messages, which are currently not in OSLib. */

#ifndef message_HOTLIST_ADD_URL
#define message_HOTLIST_ADD_URL 0x4af81
#endif

#ifndef message_HOTLIST_CHANGED
#define message_HOTLIST_CHANGED 0x4af82
#endif

struct nsurl;

/**
 * initialise the hotlist window template ready for subsequent use.
 */
void ro_gui_hotlist_initialise(void);

/**
 * make the cookie window visible.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_hotlist_present(void);

/**
 * Free any resources allocated for the cookie window.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
nserror ro_gui_hotlist_finalise(void);

bool ro_gui_hotlist_check_window(wimp_w window);
bool ro_gui_hotlist_check_menu(wimp_menu *menu);

/**
 * Add a URL to the hotlist.
 *
 * This will be passed on to the core hotlist, then
 * Message_HotlistAddURL will broadcast to any bookmark applications
 * via the Hotlist Protocol.
 *
 * \param *url	The URL to be added.
 */
void ro_gui_hotlist_add_page(struct nsurl *url);

/**
 * Clean up RMA storage used by the Message_HotlistAddURL protocol.
 */
void ro_gui_hotlist_add_cleanup(void);

/**
 * Remove a URL from the hotlist.
 *
 * This will be passed on to the core hotlist, unless we're configured
 * to use external hotlists in which case we ignore it.
 *
 * \param *url	The URL to be removed.
 */
void ro_gui_hotlist_remove_page(struct nsurl *url);

/**
 * Report whether the hotlist contains a given URL.
 *
 * This will be passed on to the core hotlist, unless we're configured
 * to use an external hotlist in which case we always report false.
 *
 * \param url The URL to be tested.
 * \return true if the hotlist contains the URL; else false.
 */
bool ro_gui_hotlist_has_page(struct nsurl *url);

#endif
