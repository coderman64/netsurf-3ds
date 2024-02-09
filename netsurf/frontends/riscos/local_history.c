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
 * Implementation of RISC OS local history.
 */

#include <stdint.h>
#include <stdlib.h>
#include <oslib/wimp.h>

#include "utils/nsoption.h"
#include "utils/messages.h"
#include "utils/log.h"
#include "utils/nsurl.h"
#include "netsurf/window.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"
#include "desktop/local_history.h"

#include "riscos/dialog.h"
#include "riscos/gui.h"
#include "riscos/menus.h"
#include "riscos/save.h"
#include "riscos/toolbar.h"
#include "riscos/wimp.h"
#include "riscos/wimp_event.h"
#include "riscos/wimputils.h"
#include "riscos/corewindow.h"
#include "riscos/local_history.h"

struct ro_local_history_window {
	struct ro_corewindow core;

	/** local history window context */
	struct local_history_session *session;

	/** tooltip previous x */
	int x;
	/** tooltip previous y */
	int y;
};

/** local_history window is a singleton */
static struct ro_local_history_window *local_history_window = NULL;

/** riscos template for local_history window */
static wimp_window *dialog_local_history_template;


/**
 * callback to draw on drawable area of ro local history window
 *
 * \param ro_cw The riscos core window structure.
 * \param r The rectangle of the window that needs updating.
 * \param originx The risc os plotter x origin.
 * \param originy The risc os plotter y origin.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ro_local_history_draw(struct ro_corewindow *ro_cw,
		      int originx,
		      int originy,
		      struct rect *r)
{
	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &ro_plotters
	};
	struct ro_local_history_window *lhw;

	lhw = (struct ro_local_history_window *)ro_cw;

	ro_plot_origin_x = originx;
	ro_plot_origin_y = originy;
	no_font_blending = true;
	local_history_redraw(lhw->session, 0, 0, r, &ctx);
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
ro_local_history_key(struct ro_corewindow *ro_cw, uint32_t nskey)
{
	struct ro_local_history_window *lhw;

	lhw = (struct ro_local_history_window *)ro_cw;

	if (local_history_keypress(lhw->session, nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * handle hover mouse movement for tooltips
 */
static nserror
ro_local_history_tooltip(struct ro_local_history_window *lhw, int x, int y)
{
	int width;
	nsurl *url;
	wimp_window_state state;
	wimp_icon_state ic;
	os_box box = {0, 0, 0, 0};
	os_error *error;
	wimp_pointer pointer;
	nserror res;

	/* check if tooltip are required */
	if (!nsoption_bool(history_tooltip)) {
		return NSERROR_OK;
	}

	/* ensure pointer has moved */
	if ((lhw->x == x) && (lhw->y == y)) {
		return NSERROR_OK;
	}

	lhw->x = x;
	lhw->y = y;

	res = local_history_get_url(lhw->session, x, y, &url);
	if (res != NSERROR_OK) {
		/* not over a tree entry => close tooltip window. */
		error = xwimp_close_window(dialog_tooltip);
		if (error) {
			NSLOG(netsurf, INFO, "xwimp_close_window: 0x%x: %s",
			      error->errnum, error->errmess);
			ro_warn_user("WimpError", error->errmess);
			return NSERROR_NOMEM;
		}
		return NSERROR_OK;
	}

	/* get width of string */
	error = xwimptextop_string_width(nsurl_access(url),
					 nsurl_length(url) > 256 ? 256 : nsurl_length(url),
					 &width);
	if (error) {
		NSLOG(netsurf, INFO, "xwimptextop_string_width: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		nsurl_unref(url);
		return NSERROR_NOMEM;
	}

	ro_gui_set_icon_string(dialog_tooltip, 0, nsurl_access(url), true);
	nsurl_unref(url);

	/* resize icon appropriately */
	ic.w = dialog_tooltip;
	ic.i = 0;
	error = xwimp_get_icon_state(&ic);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_icon_state: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}
	error = xwimp_resize_icon(dialog_tooltip, 0,
				  ic.icon.extent.x0, ic.icon.extent.y0,
				  width + 16, ic.icon.extent.y1);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_resize_icon: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}

	state.w = dialog_tooltip;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}

	/* update window extent */
	box.x1 = width + 16;
	box.y0 = -36;
	error = xwimp_set_extent(dialog_tooltip, &box);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_set_extent: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}

	error = xwimp_get_pointer_info(&pointer);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_pointer_info: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}

	/* set visible area */
	state.visible.x0 = pointer.pos.x + 24;
	state.visible.y0 = pointer.pos.y - 22 - 36;
	state.visible.x1 = pointer.pos.x + 24 + width + 16;
	state.visible.y1 = pointer.pos.y - 22;
	state.next = wimp_TOP;
	/* open window */
	error = xwimp_open_window(PTR_WIMP_OPEN(&state));
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_open_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return NSERROR_NOMEM;
	}
	return NSERROR_OK;
}


/**
 * callback for mouse event on ro local_history window
 *
 * \param ro_cw The ro core window structure.
 * \param mouse_state mouse state
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on sucess otherwise apropriate error code.
 */
static nserror
ro_local_history_mouse(struct ro_corewindow *ro_cw,
		       browser_mouse_state mouse_state,
		       int x, int y)
{
	struct ro_local_history_window *lhw;

	lhw = (struct ro_local_history_window *)ro_cw;

	switch (mouse_state) {

	case BROWSER_MOUSE_HOVER:
		ro_local_history_tooltip(lhw, x, y);
		break;

	case BROWSER_MOUSE_LEAVE:
		ro_gui_dialog_close(dialog_tooltip);
		break;

	default:
		local_history_mouse_action(lhw->session, mouse_state, x, y);
		break;
	}

	return NSERROR_OK;
}


/**
 * Creates the window for the local_history tree.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror
ro_local_history_init(struct browser_window *bw,
		      struct ro_local_history_window **win_out)
{
	os_error *error;
	struct ro_local_history_window *ncwin;
	nserror res;

	/* memoise window so it can be represented when necessary
	 * instead of recreating every time.
	 */
	if ((*win_out) != NULL) {
		res = local_history_set((*win_out)->session, bw);
		return res;
	}

	ncwin = calloc(1, sizeof(*ncwin));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	/* create window from template */
	error = xwimp_create_window(dialog_local_history_template,
				    &ncwin->core.wh);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_create_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		free(ncwin);
		return NSERROR_NOMEM;
	}

	/* initialise callbacks */
	ncwin->core.draw = ro_local_history_draw;
	ncwin->core.key = ro_local_history_key;
	ncwin->core.mouse = ro_local_history_mouse;

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

	res = local_history_init(ncwin->core.cb_table,
				 (struct core_window *)ncwin,
				 bw,
				 &ncwin->session);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	*win_out = ncwin;

	return NSERROR_OK;
}


/**
 * open RISC OS local history window at the correct size
 */
static nserror
ro_local_history_open(struct ro_local_history_window *lhw, wimp_w parent)
{
	nserror res;
	int width, height;
	os_box box = {0, 0, 0, 0};
	wimp_window_state state;
	os_error *error;

	res = local_history_get_size(lhw->session, &width, &height);
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

	local_history_scroll_to_cursor(lhw->session);

	return NSERROR_OK;
}

/* exported interface documented in riscos/local_history.h */
nserror ro_gui_local_history_present(wimp_w parent, struct browser_window *bw)
{
	nserror res;

	res = ro_local_history_init(bw, &local_history_window);
	if (res == NSERROR_OK) {
		NSLOG(netsurf, INFO, "Presenting");
		res = ro_local_history_open(local_history_window, parent);
	} else {
		NSLOG(netsurf, INFO, "Failed presenting error code %d", res);
	}

	return res;
}


/* exported interface documented in riscos/local_history.h */
void ro_gui_local_history_initialise(void)
{
	dialog_local_history_template = ro_gui_dialog_load_template("history");
}


/* exported interface documented in riscos/local_history.h */
nserror ro_gui_local_history_finalise(void)
{
	nserror res;

	if (local_history_window == NULL) {
		return NSERROR_OK;
	}

	res = local_history_fini(local_history_window->session);
	if (res == NSERROR_OK) {
		res = ro_corewindow_fini(&local_history_window->core);

		free(local_history_window);
		local_history_window = NULL;
	}

	return res;
}
