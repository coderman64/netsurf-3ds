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

#ifndef NETSURF_WINDOWS_COREWINDOW_H
#define NETSURF_WINDOWS_COREWINDOW_H

#include "netsurf/core_window.h"

/**
 * nsw32 core window state
 */
struct nsw32_corewindow {
	/** window handle */
	HWND hWnd;

	/** content width */
	int content_width;

	/** content height */
	int content_height;

	/** window title */
	const char *title;
	
        /** drag status set by core */
        core_window_drag_status drag_status;

        /** table of callbacks for core window operations */
        struct core_window_callback_table *cb_table;
	
        /**
         * callback to draw on drawable area of nsw32 core window
         *
         * \param nsw32_cw The nsw32 core window structure.
         * \param r The rectangle of the window that needs updating.
         * \return NSERROR_OK on success otherwise apropriate error code
         */
        nserror (*draw)(struct nsw32_corewindow *nsw32_cw, int scrollx, int scrolly, struct rect *r);

        /**
         * callback for keypress on nsw32 core window
         *
         * \param nsw32_cw The nsw32 core window structure.
         * \param nskey The netsurf key code.
         * \return NSERROR_OK if key processed,
         *         NSERROR_NOT_IMPLEMENTED if key not processed
         *         otherwise apropriate error code
         */
        nserror (*key)(struct nsw32_corewindow *nsw32_cw, uint32_t nskey);

        /**
         * callback for mouse event on nsw32 core window
         *
         * \param nsw32_cw The nsw32 core window structure.
         * \param mouse_state mouse state
         * \param x location of event
         * \param y location of event
         * \return NSERROR_OK on sucess otherwise apropriate error code.
         */
        nserror (*mouse)(struct nsw32_corewindow *nsw32_cw, browser_mouse_state mouse_state, int x, int y);

	/**
	 * callback for window close event
	 *
         * \param nsw32_cw The nsw32 core window structure.
         * \return NSERROR_OK on sucess otherwise apropriate error code.
	 */
	nserror (*close)(struct nsw32_corewindow *nsw32_cw);
};

/**
 * initialise elements of nsw32 core window.
 *
 * As a pre-requisite the draw, key and mouse callbacks must be defined
 *
 * \param hInstance The instance to create the core window in
 * \param hWndParent parent window handle may be NULL for top level window.
 * \param nsw32_cw A nsw32 core window structure to initialise
 * \return NSERROR_OK on successful initialisation otherwise error code.
 */
nserror nsw32_corewindow_init(HINSTANCE hInstance,
			      HWND hWndParent,
			      struct nsw32_corewindow *nsw32_cw);

/**
 * finalise elements of nsw32 core window.
 *
 * \param nsw32_cw A nsw32 core window structure to initialise
 * \return NSERROR_OK on successful finalisation otherwise error code.
 */
nserror nsw32_corewindow_fini(struct nsw32_corewindow *nsw32_cw);

nserror nsw32_create_corewindow_class(HINSTANCE hInstance);

#endif
