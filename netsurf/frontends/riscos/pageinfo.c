/*
 * Copyright 2020 Vincent Sanders <vince@netsurf-browser.org>
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
 * Implementation of RISC OS page info core window.
 */

#include <stdint.h>
#include <stdlib.h>
#include <oslib/wimp.h>

#include "utils/log.h"
#include "netsurf/mouse.h"
#include "netsurf/plotters.h"
#include "desktop/page-info.h"

#include "riscos/gui.h"
#include "riscos/dialog.h"
#include "riscos/toolbar.h"
#include "riscos/wimputils.h"
#include "riscos/corewindow.h"
#include "riscos/pageinfo.h"


/**
 * Page info window container for RISC OS.
 */
struct ro_pageinfo_window {
	struct ro_corewindow core;
	/** Core page-info window */
	struct page_info *pgi;
};

/** page info window is a singleton */
static struct ro_pageinfo_window *pageinfo_window = NULL;

/** riscos template for pageinfo window */
static wimp_window *dialog_pageinfo_template;

/**
 * callback to draw on drawable area of ro page info window
 *
 * \param ro_cw The riscos core window structure.
 * \param r The rectangle of the window that needs updating.
 * \param originx The risc os plotter x origin.
 * \param originy The risc os plotter y origin.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ro_pageinfo_draw(struct ro_corewindow *ro_cw,
		      int originx,
		      int originy,
		      struct rect *r)
{
	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &ro_plotters
	};
	struct ro_pageinfo_window *lhw;

	lhw = (struct ro_pageinfo_window *)ro_cw;

	ro_plot_origin_x = originx;
	ro_plot_origin_y = originy;
	no_font_blending = true;
	page_info_redraw(lhw->pgi, 0, 0, r, &ctx);
	no_font_blending = false;

	return NSERROR_OK;
}


/**
 * callback for keypress on ro coookie window
 *
 * \param ro_cw The ro core window structure.
 * \param nskey The netsurf key code.
 * \return NSERROR_OK if key processed,
 *         NSERROR_NOT_IMPLEMENTED if key not processed
 *         otherwise apropriate error code
 */
static nserror
ro_pageinfo_key(struct ro_corewindow *ro_cw, uint32_t nskey)
{
	struct ro_pageinfo_window *lhw;

	lhw = (struct ro_pageinfo_window *)ro_cw;

	if (page_info_keypress(lhw->pgi, nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * callback for mouse event on ro page info window
 *
 * \param ro_cw The ro core window structure.
 * \param mouse_state mouse state
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on sucess otherwise apropriate error code.
 */
static nserror
ro_pageinfo_mouse(struct ro_corewindow *ro_cw,
		       browser_mouse_state mouse_state,
		       int x, int y)
{
	struct ro_pageinfo_window *pgiw;

	pgiw = (struct ro_pageinfo_window *)ro_cw;
	bool did_something = false;

	if (page_info_mouse_action(pgiw->pgi, mouse_state, x, y, &did_something) == NSERROR_OK) {
		if (did_something == true) {
			/* Something happened so we need to close ourselves */
			ro_gui_dialog_close(ro_cw->wh);
		}
	}

	if ((mouse_state & BROWSER_MOUSE_LEAVE) != 0) {
		ro_gui_dialog_close(ro_cw->wh);
	}

	return NSERROR_OK;
}


/**
 * Creates the window for the page info tree.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror
ro_pageinfo_init(struct browser_window *bw,
		      struct ro_pageinfo_window **win_out)
{
	os_error *error;
	struct ro_pageinfo_window *ncwin;
	nserror res;

	/* memoise window so it can be represented when necessary
	 * instead of recreating every time.
	 */
	if ((*win_out) != NULL) {
		res = page_info_set((*win_out)->pgi, bw);
		return res;
	}

	ncwin = calloc(1, sizeof(*ncwin));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	/* create window from template */
	error = xwimp_create_window(dialog_pageinfo_template,
				    &ncwin->core.wh);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_create_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		free(ncwin);
		return NSERROR_NOMEM;
	}

	/* initialise callbacks */
	ncwin->core.draw = ro_pageinfo_draw;
	ncwin->core.key = ro_pageinfo_key;
	ncwin->core.mouse = ro_pageinfo_mouse;

	/* initialise core window */
	res = ro_corewindow_init(&ncwin->core,
				 NULL,
				 NULL,
				 0,
				 NULL);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	res = page_info_create(ncwin->core.cb_table,
				 (struct core_window *)ncwin,
				 bw,
				 &ncwin->pgi);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	*win_out = ncwin;

	return NSERROR_OK;
}


/**
 * open RISC OS page info window at the correct size
 */
static nserror
ro_pageinfo_open(struct ro_pageinfo_window *lhw, wimp_w parent)
{
	nserror res;
	int width, height;
	os_box box = {0, 0, 0, 0};
	wimp_window_state state;
	os_error *error;

	res = page_info_get_size(lhw->pgi, &width, &height);
	if (res != NSERROR_OK) {
		return res;
	}

	width *= 2;
	height *= 2;

	/* set extent */
	box.x1 = width;
	box.y0 = -height;
	error = xwimp_set_extent(lhw->core.wh, &box);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_set_extent: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}

	/* open full size */
	state.w = lhw->core.wh;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}
	state.visible.x0 = 0;
	state.visible.y0 = 0;
	state.visible.x1 = width;
	state.visible.y1 = height;
	state.next = wimp_HIDDEN;
	error = xwimp_open_window(PTR_WIMP_OPEN(&state));
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_open_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}

	ro_gui_dialog_open_persistent(parent, lhw->core.wh, true);

	/* Give the window focus. */
	error = xwimp_set_caret_position(lhw->core.wh, -1, 0, 0, -1, 0);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xwimp_set_caret_position: 0x%x : %s",
		      error->errnum,
		      error->errmess);
	}

	return NSERROR_OK;
}


/* exported interface documented in riscos/pageinfo.h */
nserror ro_gui_pageinfo_initialise(void)
{
	dialog_pageinfo_template = ro_gui_dialog_load_template("corepginfo");

	return NSERROR_OK;
}

/* exported interface documented in riscos/pageinfo.h */
nserror ro_gui_pageinfo_present(struct gui_window *gw)
{
	nserror res;

	res = ro_pageinfo_init(gw->bw, &pageinfo_window);
	if (res == NSERROR_OK) {
		NSLOG(netsurf, INFO, "Presenting");
		res = ro_pageinfo_open(pageinfo_window, gw->window);
	} else {
		NSLOG(netsurf, INFO, "Failed presenting error code %d", res);
	}

	return res;
}

/* exported interface documented in riscos/pageinfo.h */
nserror ro_gui_pageinfo_finalise(void)
{
	nserror res;

	if (pageinfo_window == NULL) {
		return NSERROR_OK;
	}

	res = page_info_destroy(pageinfo_window->pgi);
	if (res == NSERROR_OK) {
		res = ro_corewindow_fini(&pageinfo_window->core);

		free(pageinfo_window);
		pageinfo_window = NULL;
	}

	return res;
}
