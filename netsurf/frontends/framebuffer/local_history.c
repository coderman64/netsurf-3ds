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
 * Implementation of framebuffer local history manager.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

#include <libnsfb.h>
#include <libnsfb_plot.h>
#include <libnsfb_event.h>

#include "utils/log.h"
#include "netsurf/keypress.h"
#include "netsurf/plotters.h"
#include "desktop/local_history.h"

#include "framebuffer/gui.h"
#include "framebuffer/fbtk.h"
#include "framebuffer/framebuffer.h"
#include "framebuffer/corewindow.h"
#include "framebuffer/local_history.h"

struct fb_local_history_window {
	struct fb_corewindow core;

	struct local_history_session *session;
};

static struct fb_local_history_window *local_history_window = NULL;


/**
 * callback for mouse action on local history window
 *
 * \param fb_cw The fb core window structure.
 * \param mouse_state netsurf mouse state on event
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
fb_local_history_mouse(struct fb_corewindow *fb_cw,
		    browser_mouse_state mouse_state,
		    int x, int y)
{
	struct fb_local_history_window *lhw;
	/* technically degenerate container of */
	lhw = (struct fb_local_history_window *)fb_cw;

	local_history_mouse_action(lhw->session, mouse_state, x, y);

	if (mouse_state != BROWSER_MOUSE_HOVER) {
		fbtk_set_mapping(lhw->core.wnd, false);
	}

	return NSERROR_OK;
}


/**
 * callback for keypress on local history window
 *
 * \param fb_cw The fb core window structure.
 * \param nskey The netsurf key code
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
fb_local_history_key(struct fb_corewindow *fb_cw, uint32_t nskey)
{
	struct fb_local_history_window *lhw;
	/* technically degenerate container of */
	lhw = (struct fb_local_history_window *)fb_cw;

	if (local_history_keypress(lhw->session, nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * callback on draw event for local history window
 *
 * \param fb_cw The fb core window structure.
 * \param r The rectangle of the window that needs updating.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
fb_local_history_draw(struct fb_corewindow *fb_cw, struct rect *r)
{
	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &fb_plotters
	};
	struct fb_local_history_window *lhw;

	/* technically degenerate container of */
	lhw = (struct fb_local_history_window *)fb_cw;

	local_history_redraw(lhw->session, 0, 0, r, &ctx);

	return NSERROR_OK;
}

/**
 * Creates the window for the local history view.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror
fb_local_history_init(fbtk_widget_t *parent,
		      struct browser_window *bw,
		      struct fb_local_history_window **win_out)
{
	struct fb_local_history_window *ncwin;
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

	ncwin->core.draw = fb_local_history_draw;
	ncwin->core.key = fb_local_history_key;
	ncwin->core.mouse = fb_local_history_mouse;

	res = fb_corewindow_init(parent, &ncwin->core);
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


/* exported function documented gtk/history.h */
nserror fb_local_history_present(fbtk_widget_t *parent,
				 struct browser_window *bw)
{
	nserror res;
	int prnt_width, prnt_height;
	int width, height;

	res = fb_local_history_init(parent, bw, &local_history_window);
	if (res == NSERROR_OK) {

		 prnt_width = fbtk_get_width(parent);
		 prnt_height = fbtk_get_height(parent);

		/* resize history widget ensureing the drawing area is
		 * no larger than parent window
		 */
		res = local_history_get_size(local_history_window->session,
					     &width,
					     &height);
		if (width > prnt_width) {
			width = prnt_width;
		}
		if (height > prnt_height) {
			height = prnt_height;
		}
		/* should update scroll area with contents */

		fbtk_set_zorder(local_history_window->core.wnd, INT_MIN);
		fbtk_set_mapping(local_history_window->core.wnd, true);
		local_history_scroll_to_cursor(local_history_window->session);
	}

	return res;
}


/* exported function documented gtk/history.h */
nserror fb_local_history_hide(void)
{
	nserror res = NSERROR_OK;

	if (local_history_window != NULL) {
		fbtk_set_mapping(local_history_window->core.wnd, false);

		res = local_history_set(local_history_window->session, NULL);
	}

	return res;
}


/* exported function documented gtk/history.h */
nserror fb_local_history_destroy(void)
{
	nserror res;

	if (local_history_window == NULL) {
		return NSERROR_OK;
	}

	res = local_history_fini(local_history_window->session);
	if (res == NSERROR_OK) {
		res = fb_corewindow_fini(&local_history_window->core);
		//gtk_widget_destroy(GTK_WIDGET(local_history_window->wnd));
		free(local_history_window);
		local_history_window = NULL;
	}

	return res;

}
