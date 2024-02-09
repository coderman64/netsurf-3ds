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

#ifndef FB_COREWINDOW_H
#define FB_COREWINDOW_H

#include "netsurf/core_window.h"

/**
 * fb core window state
 */
struct fb_corewindow {

	/**
	 * framebuffer toolkit window.
	 */
	struct fbtk_widget_s *wnd;
	/**
	 * framebuffer toolkit horizontal scrollbar.
	 */
	struct fbtk_widget_s *hscroll;
	/**
	 * framebuffer toolkit vertical scrollbar.
	 */
	struct fbtk_widget_s *vscroll;
	/**
	 * framebuffer toolkit user drawable widget.
	 */
	struct fbtk_widget_s *drawable;

	int scrollx, scrolly; /**< scroll offsets. */


        /** drag status set by core */
        core_window_drag_status drag_status;

        /** table of callbacks for core window operations */
        struct core_window_callback_table *cb_table;

        /**
         * callback to draw on drawable area of fb core window
         *
         * \param fb_cw The fb core window structure.
         * \param r The rectangle of the window that needs updating.
         * \return NSERROR_OK on success otherwise apropriate error code
         */
        nserror (*draw)(struct fb_corewindow *fb_cw, struct rect *r);

        /**
         * callback for keypress on fb core window
         *
         * \param fb_cw The fb core window structure.
         * \param nskey The netsurf key code.
         * \return NSERROR_OK if key processed,
         *         NSERROR_NOT_IMPLEMENTED if key not processed
         *         otherwise apropriate error code
         */
        nserror (*key)(struct fb_corewindow *fb_cw, uint32_t nskey);

        /**
         * callback for mouse event on fb core window
         *
         * \param fb_cw The fb core window structure.
         * \param mouse_state mouse state
         * \param x location of event
         * \param y location of event
         * \return NSERROR_OK on sucess otherwise apropriate error code.
         */
        nserror (*mouse)(struct fb_corewindow *fb_cw, browser_mouse_state mouse_state, int x, int y);
};


/**
 * initialise elements of fb core window.
 *
 * As a pre-requisite the draw, key and mouse callbacks must be defined
 *
 * \param fb_cw A fb core window structure to initialise
 * \return NSERROR_OK on successful initialisation otherwise error code.
 */
nserror fb_corewindow_init(fbtk_widget_t *parent, struct fb_corewindow *fb_cw);


/**
 * finalise elements of fb core window.
 *
 * \param fb_cw A fb core window structure to initialise
 * \return NSERROR_OK on successful finalisation otherwise error code.
 */
nserror fb_corewindow_fini(struct fb_corewindow *fb_cw);

#endif
