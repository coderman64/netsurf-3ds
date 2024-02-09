/*
 * Copyright 2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_COREWINDOW_H
#define AMIGA_COREWINDOW_H

#include "netsurf/core_window.h"

#include "amiga/gui.h" /* need to know the size of ami_generic_window :( */
#include "amiga/plotters.h"

/**
 * BOOPSI objects
 */

enum {
	GID_CW_WIN = 0, /* window object */
	GID_CW_MAIN, /* root layout object */
	GID_CW_DRAW, /* drawing area (space.gadget) */
	GID_CW_HSCROLL, /* horizontal scroller */
	GID_CW_VSCROLL, /* vertical scroller */
	GID_CW_HSCROLLLAYOUT, /* horizontal scroller container*/
	GID_CW_VSCROLLLAYOUT, /* vertical scroller container */
	GID_CW_LAST
};

/**
 * Amiga core window state
 */
struct ami_corewindow {
		/*
		 * Any variables common to any frontend window would go here.
		 * e.g. drawing area handles, toolkit pointers or other state
		 */
		struct ami_generic_window w;
		struct Window *win;
		Object *objects[GID_CW_LAST];

		struct Hook idcmp_hook;
		struct timeval lastclick;

		int mouse_x_click;
		int mouse_y_click;
		int mouse_state;

		bool dragging;
		int drag_x_start;
		int drag_y_start;

		bool close_window; // set to true to close the window during event loop

		APTR deferred_rects_pool;
		struct MinList *deferred_rects;

		/** keep track of the scrollbar type we're using */
		bool in_border_scroll;
		bool scroll_x_visible;
		bool scroll_y_visible;

		/** window title, must be allocated wth ami_utf8 function */
		char *wintitle;

		/** stuff for our off-screen render bitmap */
		struct gui_globals *gg;
		struct MinList *shared_pens;

		/** drag status set by core */
		core_window_drag_status drag_status;

		/** table of callbacks for core window operations */
		struct core_window_callback_table *cb_table;

		/**
		 * callback to draw on drawable area of Amiga core window
		 *
		 * \param ami_cw The Amiga core window structure.
		 * \param x Plot origin (X)
		 * \param r Plot origin (Y)
		 * \param r The rectangle of the window that needs updating.
		 * \param ctx Redraw context
		 * \return NSERROR_OK on success otherwise apropriate error code
		 */
		nserror (*draw)(struct ami_corewindow *ami_cw, int x, int y, struct rect *r,
						struct redraw_context *ctx);

		/**
		 * callback for keypress on Amiga core window
		 *
		 * \param ami_cw The Amiga core window structure.
		 * \param nskey The netsurf key code.
		 * \return NSERROR_OK if key processed,
		 *         NSERROR_NOT_IMPLEMENTED if key not processed
		 *         otherwise apropriate error code
		 */
		nserror (*key)(struct ami_corewindow *ami_cw, uint32_t nskey);

		/**
		 * callback for mouse event on Amiga core window
		 *
		 * \param ami_cw The Amiga core window structure.
		 * \param mouse_state mouse state
		 * \param x location of event
		 * \param y location of event
		 * \return NSERROR_OK on sucess otherwise apropriate error code.
		 */
		nserror (*mouse)(struct ami_corewindow *ami_cw, browser_mouse_state mouse_state, int x, int y);

		/**
		 * callback for unknown events on Amiga core window
		 * eg. buttons in the ssl cert window
		 * (result & WMHI_CLASSMASK) gives the class of event (eg. WMHI_GADGETUP)
		 * (result & WMHI_GADGETMASK) gives the gadget ID (eg. GID_SSLCERT_ACCEPT)
		 *
		 * \param ami_cw The Amiga core window structure.
		 * \param result event as returned by RA_HandleInput()
 		 * \return TRUE if window closed during event processing
		 */
		BOOL (*event)(struct ami_corewindow *ami_cw, ULONG result);

		/**
		 * callback for drag end on Amiga core window
		 * ie. a drag *from* this window to a different window
		 *
		 * \param ami_cw The Amiga core window structure.
		 * \param x mouse x position **in screen co-ordinates**
		 * \param y mouse y position **in screen co-ordinates**
		 * \return NSERROR_OK on success otherwise apropriate error code
		 */
		nserror (*drag_end)(struct ami_corewindow *ami_cw, int x, int y);

		/**
		 * callback for icon drop on Amiga core window
		 * ie. a drag has ended *above* this window
		 * \todo this may not be very flexible but serves our current purposes
		 *
		 * \param ami_cw The Amiga core window structure.
		 * \param url url of dropped icon
		 * \param title title of dropped icon
		 * \param x mouse x position **in screen co-ordinates**
		 * \param y mouse y position **in screen co-ordinates**
		 * \return NSERROR_OK on success otherwise apropriate error code
		 */
		nserror (*icon_drop)(struct ami_corewindow *ami_cw, struct nsurl *url, const char *title, int x, int y);

		/**
		 * callback to close an Amiga core window
		 *
		 * \param ami_cw The Amiga core window structure.
		 */
		void (*close)(struct ami_corewindow *ami_cw);

};

/**
 * initialise elements of Amiga core window.
 *
 * As a pre-requisite the draw, key and mouse callbacks must be defined
 *
 * \param ami_cw An Amiga core window structure to initialise
 * \return NSERROR_OK on successful initialisation otherwise error code.
 */
nserror ami_corewindow_init(struct ami_corewindow *ami_cw);

/**
 * finalise elements of Amiga core window.
 *
 * \param ami_cw An Amiga core window structure to finialise
 * \return NSERROR_OK on successful finalisation otherwise error code.
 */
nserror ami_corewindow_fini(struct ami_corewindow *ami_cw);

#endif

