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
 * framebuffer generic core window interface.
 *
 * Provides interface for core renderers to the framebufefr toolkit
 * drawable area.
 *
 * This module is an object that must be encapsulated. Client users
 * should embed a struct fb_corewindow at the beginning of their
 * context for this display surface, fill in relevant data and then
 * call fb_corewindow_init()
 *
 * The fb core window structure requires the callback for draw, key and
 * mouse operations.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

#include <libnsfb.h>
#include <libnsfb_plot.h>
#include <libnsfb_event.h>

#include "utils/log.h"
#include "utils/utils.h"
#include "utils/messages.h"
#include "utils/utf8.h"
#include "utils/nsoption.h"
#include "netsurf/keypress.h"
#include "netsurf/mouse.h"
#include "netsurf/plot_style.h"

#include "framebuffer/gui.h"
#include "framebuffer/fbtk.h"
#include "framebuffer/corewindow.h"


/* toolkit event handlers that do generic things and call internal callbacks */


static int
fb_cw_mouse_press_event(fbtk_widget_t *widget, fbtk_callback_info *cbi)
{
	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cbi->context;
	browser_mouse_state state;

	/** \todo frambuffer corewindow mouse event handling needs improving */
	if (cbi->event->type != NSFB_EVENT_KEY_UP) {
		state = BROWSER_MOUSE_HOVER;
	} else {
		state = BROWSER_MOUSE_PRESS_1;
	}

	fb_cw->mouse(fb_cw, state, cbi->x, cbi->y);

	return 1;
}

/*
static bool
fb_cw_input_event(toolkit_widget *widget, void *ctx)
{
	struct fb_corewindow *fb_cw = (struct fb_corewindow *)ctx;

	fb_cw->key(fb_cw, keycode);

	return true;
}
*/

/**
 * handler for toolkit window redraw event
 */
static int fb_cw_draw_event(fbtk_widget_t *widget, fbtk_callback_info *cbi)
{
	struct fb_corewindow *fb_cw;
	nsfb_bbox_t rbox;
	struct rect clip;

	fb_cw = (struct fb_corewindow *)cbi->context;

	rbox.x0 = fbtk_get_absx(widget);
	rbox.y0 = fbtk_get_absy(widget);

	rbox.x1 = rbox.x0 + fbtk_get_width(widget);
	rbox.y1 = rbox.y0 + fbtk_get_height(widget);

	nsfb_claim(fbtk_get_nsfb(widget), &rbox);

	clip.x0 = fb_cw->scrollx;
	clip.y0 = fb_cw->scrolly;
	clip.x1 = fbtk_get_width(widget) + fb_cw->scrollx;
	clip.y1 = fbtk_get_height(widget) + fb_cw->scrolly;

	fb_cw->draw(fb_cw, &clip);

	nsfb_update(fbtk_get_nsfb(widget), &rbox);

	return 0;
}


/**
 * callback from core to request a redraw
 */
static nserror
fb_cw_invalidate(struct core_window *cw, const struct rect *r)
{
/*	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cw;

	toolkit_widget_queue_draw_area(fb_cw->widget,
				       r->x0, r->y0,
				       r->x1 - r->x0, r->y1 - r->y0);
*/
	return NSERROR_OK;
}


static nserror
fb_cw_update_size(struct core_window *cw, int width, int height)
{
/*	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cw;

	toolkit_widget_set_size_request(FB_WIDGET(fb_cw->drawing_area),
				    width, height);
*/
	return NSERROR_OK;
}


static nserror
fb_cw_set_scroll(struct core_window *cw, int x, int y)
{
/*	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cw;

	toolkit_scroll_widget(fb_cw->widget, r);
*/
	return NSERROR_OK;
}


static nserror
fb_cw_get_scroll(const struct core_window *cw, int *x, int *y)
{
/*	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cw;

	toolkit_scroll_widget(fb_cw->widget, r);
*/
	return NSERROR_NOT_IMPLEMENTED;
}


static nserror
fb_cw_get_window_dimensions(const struct core_window *cw,
		int *width, int *height)
{
	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cw;

	*width = fbtk_get_width(fb_cw->drawable);
	*height = fbtk_get_height(fb_cw->drawable);
	return NSERROR_OK;
}


static nserror
fb_cw_drag_status(struct core_window *cw, core_window_drag_status ds)
{
	struct fb_corewindow *fb_cw = (struct fb_corewindow *)cw;
	fb_cw->drag_status = ds;

	return NSERROR_OK;
}


struct core_window_callback_table fb_cw_cb_table = {
	.invalidate = fb_cw_invalidate,
	.update_size = fb_cw_update_size,
	.set_scroll = fb_cw_set_scroll,
	.get_scroll = fb_cw_get_scroll,
	.get_window_dimensions = fb_cw_get_window_dimensions,
	.drag_status = fb_cw_drag_status
};

/* exported function documented fb/corewindow.h */
nserror fb_corewindow_init(fbtk_widget_t *parent, struct fb_corewindow *fb_cw)
{
	int furniture_width;

	furniture_width = nsoption_int(fb_furniture_size);

	/* setup the core window callback table */
	fb_cw->cb_table = &fb_cw_cb_table;
	fb_cw->drag_status = CORE_WINDOW_DRAG_NONE;

	/* container window */
	fb_cw->wnd = fbtk_create_window(parent, 0, 0, 0, 0, 0);

	fb_cw->drawable = fbtk_create_user(fb_cw->wnd,
					   0, 0,
					   -furniture_width, -furniture_width,
					   fb_cw);

	fbtk_set_handler(fb_cw->drawable,
			 FBTK_CBT_REDRAW,
			 fb_cw_draw_event,
			 fb_cw);

	fbtk_set_handler(fb_cw->drawable,
			 FBTK_CBT_CLICK,
			 fb_cw_mouse_press_event,
			 fb_cw);
/*
	fbtk_set_handler(fb_cw->drawable,
			 FBTK_CBT_INPUT,
			 fb_cw_input_event,
			 fb_cw);

	fbtk_set_handler(fb_cw->drawable,
			 FBTK_CBT_POINTERMOVE,
			 fb_cw_move_event,
			 fb_cw);
*/

	/* create horizontal scrollbar */
	fb_cw->hscroll = fbtk_create_hscroll(fb_cw->wnd,
					   0,
					   fbtk_get_height(fb_cw->wnd) - furniture_width,
					   fbtk_get_width(fb_cw->wnd) - furniture_width,
					   furniture_width,
					   FB_SCROLL_COLOUR,
					   FB_FRAME_COLOUR,
					   NULL,
					   NULL);

	fb_cw->vscroll = fbtk_create_vscroll(fb_cw->wnd,
					   fbtk_get_width(fb_cw->wnd) - furniture_width,
					   0,
					   furniture_width,
					   fbtk_get_height(fb_cw->wnd) - furniture_width,
					   FB_SCROLL_COLOUR,
					   FB_FRAME_COLOUR,
					   NULL,
					   NULL);

	fbtk_create_fill(fb_cw->wnd,
			 fbtk_get_width(fb_cw->wnd) - furniture_width,
			 fbtk_get_height(fb_cw->wnd) - furniture_width,
			 furniture_width,
			 furniture_width,
			 FB_FRAME_COLOUR);


	return NSERROR_OK;
}

/* exported interface documented in fb/corewindow.h */
nserror fb_corewindow_fini(struct fb_corewindow *fb_cw)
{
	return NSERROR_OK;
}
