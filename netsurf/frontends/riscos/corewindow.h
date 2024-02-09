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
 * RISC OS core window interface
 *
 * This module is an object that must be encapsulated. Client users
 * should embed a struct ro_corewindow at the beginning of their
 * context for this display surface, fill in relevant data and then
 * call ro_corewindow_init()
 */

#ifndef NETSURF_RISCOS_COREWINDOW_H
#define NETSURF_RISCOS_COREWINDOW_H

#include "netsurf/core_window.h"

/**
 * ro core window state
 */
struct ro_corewindow {
	/** window handle */
	wimp_w wh;

	/** toolbar */
	struct toolbar *toolbar;

	/** content plot origin y relative to window */
	int origin_y;

	/** content width */
	int content_width;

	/** content height */
	int content_height;

        /** drag status set by core */
        core_window_drag_status drag_status;

        /** table of callbacks for core window operations */
        struct core_window_callback_table *cb_table;
	
        /**
         * callback to draw on drawable area of ro core window
         *
         * \param ro_cw The riscos core window structure.
	 * \param originx The risc os plotter x origin.
	 * \param originy The risc os plotter y origin.
         * \param r The rectangle of the window that needs updating.
         * \return NSERROR_OK on success otherwise apropriate error code
         */
        nserror (*draw)(struct ro_corewindow *ro_cw, int originx, int originy, struct rect *r);

        /**
         * callback for keypress on ro core window
         *
         * \param ro_cw The ro core window structure.
         * \param nskey The netsurf key code.
         * \return NSERROR_OK if key processed,
         *         NSERROR_NOT_IMPLEMENTED if key not processed
         *         otherwise apropriate error code
         */
        nserror (*key)(struct ro_corewindow *ro_cw, uint32_t nskey);

        /**
         * callback for mouse event on ro core window
         *
         * \param ro_cw The ro core window structure.
         * \param mouse_state mouse state
         * \param x location of event
         * \param y location of event
         * \return NSERROR_OK on sucess otherwise apropriate error code.
         */
        nserror (*mouse)(struct ro_corewindow *ro_cw, browser_mouse_state mouse_state, int x, int y);

	/**
         * callback for clicks in ro core window toolbar.
         *
         * \param ro_cw The ro core window structure.
         * \param action The button bar action.
         * \return NSERROR_OK if config saved, otherwise apropriate error code
         */
        nserror (*toolbar_click)(struct ro_corewindow *ro_cw, button_bar_action action);

	/**
         * callback for updating state of buttons in ro core window toolbar.
         *
         * \param ro_cw The ro core window structure.
         * \return NSERROR_OK if config saved, otherwise apropriate error code
         */
        nserror (*toolbar_update)(struct ro_corewindow *ro_cw);

	/**
         * callback for saving ro core window toolbar state.
         *
         * \param ro_cw The ro core window structure.
         * \param config The new toolbar configuration.
         * \return NSERROR_OK if config saved, otherwise apropriate error code
         */
        nserror (*toolbar_save)(struct ro_corewindow *ro_cw, char *config);

};

/**
 * initialise elements of riscos core window.
 *
 * As a pre-requisite the draw, key and mouse callbacks must be defined
 *
 * \param ro_cw A riscos core window structure to initialise
 * \param tb_buttons toolbar button bar context
 * \param tb_order The order of toolbar buttons
 * \param tb_style The style of toolbar buttons
 * \param tb_help Thh toolbar help text
 * \return NSERROR_OK on successful initialisation otherwise error code.
 */
nserror ro_corewindow_init(struct ro_corewindow *ro_cw, const struct button_bar_buttons *tb_buttons, char *tb_order, theme_style tb_style, const char *tb_help);

/**
 * finalise elements of ro core window.
 *
 * \param ro_cw A riscos core window structure to initialise
 * \return NSERROR_OK on successful finalisation otherwise error code.
 */
nserror ro_corewindow_fini(struct ro_corewindow *ro_cw);


#endif
