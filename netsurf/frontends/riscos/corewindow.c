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
 * RISC OS generic core window implementation.
 *
 * Provides interface for core renderers to a risc os drawing area.
 *
 * This module is an object that must be encapsulated. Client users
 * should embed a struct ro_corewindow at the beginning of their
 * context for this display surface, fill in relevant data and then
 * call ro_corewindow_init()
 *
 * The ro core window structure requires the callback for draw, key
 * and mouse operations.
 */

#include <stdint.h>
#include <oslib/wimp.h>

#include "utils/log.h"
#include "netsurf/types.h"
#include "netsurf/mouse.h"
#include "netsurf/keypress.h"

#include "riscos/wimp_event.h"
#include "riscos/dialog.h"
#include "riscos/gui.h"
#include "riscos/window.h"
#include "riscos/toolbar.h"
#include "riscos/mouse.h"
#include "riscos/wimputils.h"
#include "riscos/corewindow.h"

#ifndef wimp_KEY_END
#define wimp_KEY_END wimp_KEY_COPY
#endif

static struct ro_corewindow *ro_cw_drag_cw;

/**
 * Update a windows scrollbars.
 *
 * in the wimp this is done by setting the extent and calling window open
 */
static void update_scrollbars(struct ro_corewindow *ro_cw, wimp_open *open)
{
	os_error *error;
	int extent_width;
	int extent_height;
	os_box extent;

	NSLOG(netsurf, INFO, "RO corewindow context %p", ro_cw);

	/* extent of content in not smaller than window so start there */
	extent_width = open->visible.x1 - open->visible.x0;
	extent_height = open->visible.y0 - open->visible.y1;
	NSLOG(netsurf, INFO,
	      "extent w:%d h:%d content w:%d h:%d origin h:%d", extent_width,
	      extent_height, ro_cw->content_width, ro_cw->content_height,
	      ro_cw->origin_y);
	if (ro_cw->content_width > extent_width) {
		extent_width = ro_cw->content_width;
	}
	if (extent_height > (ro_cw->origin_y + ro_cw->content_height)) {
		extent_height = ro_cw->origin_y + ro_cw->content_height;
	}
	NSLOG(netsurf, INFO, "extent w:%d h:%d", extent_width, extent_height);
	extent.x0 = 0;
	extent.y0 = extent_height;
	extent.x1 = extent_width;
	extent.y1 = 0;

	error = xwimp_set_extent(ro_cw->wh, &extent);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_set_extent: 0x%x: %s",
		      error->errnum, error->errmess);
		return;
	}

	error = xwimp_open_window(open);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_open_window: 0x%x: %s",
		      error->errnum, error->errmess);
	}
}


/**
 * wimp callback on redraw event
 */
static void ro_cw_redraw(wimp_draw *redraw)
{
	struct ro_corewindow *ro_cw;
	osbool more;
	os_error *error;
	struct rect r;
	int origin_x;
	int origin_y;

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(redraw->w);

	error = xwimp_redraw_window(redraw, &more);
	while ((error == NULL) && (more)) {
		/* compute rectangle to redraw */
		origin_x = redraw->box.x0 - redraw->xscroll;
		origin_y = redraw->box.y1 + ro_cw->origin_y - redraw->yscroll;

		ro_plot_clip_rect.x0 = redraw->clip.x0 - origin_x;
		ro_plot_clip_rect.y0 = origin_y - redraw->clip.y0;
		ro_plot_clip_rect.x1 = redraw->clip.x1 - origin_x;
		ro_plot_clip_rect.y1 = origin_y - redraw->clip.y1;

		r.x0 = (ro_plot_clip_rect.x0    ) / 2; /* left   */
		r.y0 = (ro_plot_clip_rect.y1    ) / 2; /* top    */
		r.x1 = (ro_plot_clip_rect.x1 + 1) / 2; /* right  */
		r.y1 = (ro_plot_clip_rect.y0 + 1) / 2; /* bottom */

		/* call the draw callback */
		ro_cw->draw(ro_cw, origin_x, origin_y, &r);

		error = xwimp_get_rectangle(redraw, &more);
	}
	if (error != NULL) {
		NSLOG(netsurf, INFO, "xwimp_redraw_window: 0x%x: %s",
		      error->errnum, error->errmess);
	}

}

static void ro_cw_scroll(wimp_scroll *scroll)
{
	os_error *error;
	int page_x;
	int page_y;
	struct ro_corewindow *ro_cw;
	wimp_open open;
	wimp_window_state state;

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(scroll->w);
	NSLOG(netsurf, INFO, "RO corewindow context %p", ro_cw);

	state.w = ro_cw->wh;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		return;
	}

	/* Don't try to update window if it's closed */
	if (!(state.flags & wimp_WINDOW_OPEN)) {
		return;
	}

	page_x = scroll->visible.x1 - scroll->visible.x0 - 32;
	page_y = scroll->visible.y1 - scroll->visible.y0 - 32;

	page_y += ro_cw->origin_y;

	open.w = scroll->w;
	open.visible = scroll->visible;
	open.next = scroll->next;

	switch (scroll->xmin) {
	case wimp_SCROLL_PAGE_LEFT:
		open.xscroll = scroll->xscroll - page_x;
		break;

	case wimp_SCROLL_COLUMN_LEFT:
		open.xscroll = scroll->xscroll - 32;
		break;

	case wimp_SCROLL_COLUMN_RIGHT:
		open.xscroll = scroll->xscroll + 32;
		break;

	case wimp_SCROLL_PAGE_RIGHT:
		open.xscroll = scroll->xscroll + page_x;
		break;

	default:
		open.xscroll = scroll->xscroll + ((page_x * (scroll->xmin>>2)) >> 2);
		break;
	}

	switch (scroll->ymin) {
	case wimp_SCROLL_PAGE_UP:
		open.yscroll = scroll->yscroll + page_y;
		break;

	case wimp_SCROLL_LINE_UP:
		open.yscroll = scroll->yscroll + 32;
		break;

	case wimp_SCROLL_LINE_DOWN:
		open.yscroll = scroll->yscroll - 32;
		break;

	case wimp_SCROLL_PAGE_DOWN:
		open.yscroll = scroll->yscroll - page_y;
		break;

	default:
		open.yscroll = scroll->yscroll + ((page_y * (scroll->ymin>>2)) >> 2);
		break;
	}

	error = xwimp_open_window(&open);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_open_window: 0x%x: %s",
		      error->errnum, error->errmess);
	}

}



/**
 * Track the mouse under Null Polls from the wimp, to support dragging.
 *
 * \param pointer Pointer to a Wimp Pointer block.
 * \param data NULL to allow use as a ro_mouse callback.
 */
static void ro_cw_mouse_at(wimp_pointer *pointer, void *data)
{
	os_error *error;
	struct ro_corewindow *ro_cw;
	wimp_window_state state;
	int xpos, ypos;
	browser_mouse_state mouse;

	/* ignore menu clicks */
	if (pointer->buttons & (wimp_CLICK_MENU)) {
		return;
	}

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(pointer->w);
	if (ro_cw == NULL) {
		NSLOG(netsurf, INFO, "no corewindow conext for window: 0x%x",
		      (unsigned int)pointer->w);
		return;
	}
	if (ro_cw != ro_cw_drag_cw) {
		NSLOG(netsurf, DEEPDEBUG, "Called without drag window: %p",
				ro_cw);
		return;
	}
	NSLOG(netsurf, INFO, "RO corewindow context %p", ro_cw);

	/* Not a Menu click. */
	state.w = pointer->w;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		return;
	}

	/* Convert the returned mouse coordinates into
	 * NetSurf's internal units.
	 */
	xpos = ((pointer->pos.x - state.visible.x0) + state.xscroll) / 2;
	ypos = ((state.visible.y1 - pointer->pos.y) -
		state.yscroll + ro_cw->origin_y) / 2;

	/* if no drag in progress report hover */
	if (ro_cw->drag_status == CORE_WINDOW_DRAG_NONE) {
		mouse = BROWSER_MOUSE_HOVER;
	} else {
		/* Start to process the mouse click. */
		mouse = ro_gui_mouse_drag_state(pointer->buttons,
						wimp_BUTTON_DOUBLE_CLICK_DRAG);

		ro_cw->mouse(ro_cw, mouse, xpos, ypos);
	}

	if (!(mouse & BROWSER_MOUSE_DRAG_ON)) {
		ro_cw->mouse(ro_cw, BROWSER_MOUSE_HOVER, xpos, ypos);
		ro_cw->drag_status = CORE_WINDOW_DRAG_NONE;
	}

	ro_cw->toolbar_update(ro_cw);
}

/**
 * Process RISC OS User Drag Box events which relate to us: in effect, drags
 * started by ro_cw_drag_start().
 *
 * \param drag Pointer to the User Drag Box Event block.
 * \param data NULL to allow use as a ro_mouse callback.
 */
static void ro_cw_drag_end(wimp_dragged *drag, void *data)
{
	os_error *error;

	error = xwimp_drag_box((wimp_drag *) -1);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_drag_box: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
	}

	error = xwimp_auto_scroll(0, NULL, NULL);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_auto_scroll: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
	}
}


/**
 * Start a RISC OS drag event to reflect on screen what is happening
 * during the core tree drag.
 *
 * \param ro_cw The RO corewindow to which the drag is attached.
 * \param pointer The RO pointer event data block starting the drag.
 * \param state The RO window state block for the treeview window.
 */
static void
ro_cw_drag_start(struct ro_corewindow *ro_cw,
		 wimp_pointer *pointer,
		 wimp_window_state *state)
{
	os_error *error;
	wimp_drag drag;
	wimp_auto_scroll_info auto_scroll;

	drag.w = ro_cw->wh;
	drag.bbox.x0 = state->visible.x0;
	drag.bbox.y0 = state->visible.y0;
	drag.bbox.x1 = state->visible.x1;
	drag.bbox.y1 = state->visible.y1 - ro_toolbar_height(ro_cw->toolbar) - 2;

	switch (ro_cw->drag_status) {
	case CORE_WINDOW_DRAG_SELECTION:
		drag.type = wimp_DRAG_USER_RUBBER;

		drag.initial.x0 = pointer->pos.x;
		drag.initial.y0 = pointer->pos.y;
		drag.initial.x1 = pointer->pos.x;
		drag.initial.y1 = pointer->pos.y;
		break;

	case CORE_WINDOW_DRAG_MOVE:
		drag.type = wimp_DRAG_USER_POINT;

		drag.initial.x0 = pointer->pos.x - 4;
		drag.initial.y0 = pointer->pos.y - 48;
		drag.initial.x1 = pointer->pos.x + 48;
		drag.initial.y1 = pointer->pos.y + 4;
		break;

	default:
		/* No other drag types are supported. */
		break;
	}

	NSLOG(netsurf, INFO, "Drag start...");

	error = xwimp_drag_box_with_flags(&drag,
			wimp_DRAG_BOX_KEEP_IN_LINE | wimp_DRAG_BOX_CLIP);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_drag_box: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
	} else {
		auto_scroll.w = ro_cw->wh;
		auto_scroll.pause_zone_sizes.x0 = 80;
		auto_scroll.pause_zone_sizes.y0 = 80;
		auto_scroll.pause_zone_sizes.x1 = 80;
		auto_scroll.pause_zone_sizes.y1 = 80 + ro_toolbar_height(ro_cw->toolbar);
		auto_scroll.pause_duration = 0;
		auto_scroll.state_change = (void *) 1;

		error = xwimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_VERTICAL,
				&auto_scroll, NULL);
		if (error) {
			NSLOG(netsurf, INFO, "xwimp_auto_scroll: 0x%x: %s",
			      error->errnum, error->errmess);
			ro_warn_user("WimpError", error->errmess);
		}

		ro_cw_drag_cw = ro_cw;
		ro_mouse_drag_start(ro_cw_drag_end, ro_cw_mouse_at, NULL, NULL);
	}
}


/**
 * Handle Pointer Leaving Window events.
 *
 * These events are delivered as the termination callback handler from
 * ro_mouse's mouse tracking.
 *
 * \param leaving The Wimp_PointerLeavingWindow block.
 * \param data NULL data pointer.
 */
static void ro_cw_pointer_leaving(wimp_leaving *leaving, void *data)
{
	struct ro_corewindow *ro_cw;

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(leaving->w);
	if (ro_cw == NULL) {
		NSLOG(netsurf, INFO, "no corewindow conext for window: 0x%x",
		      (unsigned int)leaving->w);
		return;
	}

	ro_cw->mouse(ro_cw, BROWSER_MOUSE_LEAVE, 0, 0);
}


/**
 * Wimp callback on pointer entering window.
 *
 * The wimp has issued an event to the window because the pointer has
 * entered it.
 *
 * \param entering The entering event to be processed
 */
static void ro_cw_pointer_entering(wimp_entering *entering)
{
	ro_mouse_track_start(ro_cw_pointer_leaving, ro_cw_mouse_at, NULL);
}


/**
 * Wimp callback on window open event.
 *
 * The wimp has issued an event to the window because an attempt has
 * been made to open or resize it. This requires the new dimensions to
 * be calculated and set within the wimp.
 *
 * \param open The open event to be processed
 */
static void ro_cw_open(wimp_open *open)
{
	struct ro_corewindow *ro_cw;

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(open->w);

	update_scrollbars(ro_cw, open);
}

static bool ro_cw_mouse_click(wimp_pointer *pointer)
{
	os_error *error;
	wimp_window_state state;
	int xpos, ypos;
	browser_mouse_state mouse = 0;
	bool handled = false;
	struct ro_corewindow *ro_cw;

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(pointer->w);
	NSLOG(netsurf, INFO, "RO corewindow context %p", ro_cw);


	state.w = ro_cw->wh;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		return false;
	}

	/* Convert the returned mouse coordinates into NetSurf's internal
	 * units.
	 */
	xpos = ((pointer->pos.x - state.visible.x0) + state.xscroll) / 2;
	ypos = ((state.visible.y1 - pointer->pos.y) -
		state.yscroll + ro_cw->origin_y) / 2;

	if (pointer->buttons != wimp_CLICK_MENU) {
		mouse = ro_gui_mouse_click_state(pointer->buttons,
						 wimp_BUTTON_DOUBLE_CLICK_DRAG);

		/* Give the window input focus on Select-clicks.  This wouldn't
		 * be necessary if the core used the RISC OS caret.
		 */
		if (mouse & BROWSER_MOUSE_CLICK_1) {
			xwimp_set_caret_position(ro_cw->wh,
						 -1, -100, -100, 32, -1);
		}
	}

	/* call the mouse callback */
	if (mouse != 0) {
		ro_cw->mouse(ro_cw, mouse, xpos, ypos);

		/* If it's a visible drag, start the RO side of the visible
		 * effects.
		 */
		if ((ro_cw->drag_status == CORE_WINDOW_DRAG_SELECTION) ||
		    (ro_cw->drag_status == CORE_WINDOW_DRAG_MOVE)) {
			ro_cw_drag_start(ro_cw, pointer, &state);
		}

		ro_cw->toolbar_update(ro_cw);
	}

	/* Special actions for some mouse buttons.  Adjust closes the dialog;
	 * Menu opens a menu.  For the latter, we assume that the owning module
	 * will have attached a window menu to our parent window with the auto
	 * flag unset (so that we can fudge the selection above).  If it hasn't,
	 * the call will quietly fail.
	 *
	 * \TODO -- Adjust-click close isn't a perfect copy of what the RO
	 *          version did: adjust clicks anywhere close the tree, and
	 *          selections persist.
	 */
	switch(pointer->buttons) {
	case wimp_CLICK_ADJUST:
		if (handled) {
			ro_gui_dialog_close(ro_cw->wh);
		}
		break;

	case wimp_CLICK_MENU:
		ro_gui_wimp_event_process_window_menu_click(pointer);
		break;
	}

	return true;
}

static bool ro_cw_keypress(wimp_key *key)
{
	uint32_t c;
	struct ro_corewindow *ro_cw;
	nserror res;

	ro_cw = (struct ro_corewindow *)ro_gui_wimp_event_get_user_data(key->w);
	NSLOG(netsurf, INFO, "RO corewindow context %p", ro_cw);

	c = (uint32_t) key->c;

	if ((unsigned)c < 0x20 ||
	    (0x7f <= c && c <= 0x9f) ||
	    (c & IS_WIMP_KEY)) {
		/* Munge control keys into unused control chars */
		/* We can't map onto 1->26 (reserved for ctrl+<qwerty>
		   That leaves 27->31 and 128->159 */
		switch (c & ~IS_WIMP_KEY) {
		case wimp_KEY_TAB:
			c = 9;
			break;

		case wimp_KEY_SHIFT | wimp_KEY_TAB:
			c = 11;
			break;

		/* cursor movement keys */
		case wimp_KEY_HOME:
		case wimp_KEY_CONTROL | wimp_KEY_LEFT:
			c = NS_KEY_LINE_START;
			break;

		case wimp_KEY_END:
			if (os_version >= RISCOS5)
				c = NS_KEY_LINE_END;
			else
				c = NS_KEY_DELETE_RIGHT;
			break;

		case wimp_KEY_CONTROL | wimp_KEY_RIGHT:
			c = NS_KEY_LINE_END;
			break;

		case wimp_KEY_CONTROL | wimp_KEY_UP:
			c = NS_KEY_TEXT_START;
			break;

		case wimp_KEY_CONTROL | wimp_KEY_DOWN:
			c = NS_KEY_TEXT_END;
			break;

		case wimp_KEY_SHIFT | wimp_KEY_LEFT:
			c = NS_KEY_WORD_LEFT;
			break;

		case wimp_KEY_SHIFT | wimp_KEY_RIGHT:
			c = NS_KEY_WORD_RIGHT;
			break;

		case wimp_KEY_SHIFT | wimp_KEY_UP:
			c = NS_KEY_PAGE_UP;
			break;

		case wimp_KEY_SHIFT | wimp_KEY_DOWN:
			c = NS_KEY_PAGE_DOWN;
			break;

		case wimp_KEY_LEFT:
			c = NS_KEY_LEFT;
			break;

		case wimp_KEY_RIGHT:
			c = NS_KEY_RIGHT;
			break;

		case wimp_KEY_UP:
			c = NS_KEY_UP;
			break;

		case wimp_KEY_DOWN:
			c = NS_KEY_DOWN;
			break;

		/* editing */
		case wimp_KEY_CONTROL | wimp_KEY_END:
			c = NS_KEY_DELETE_LINE_END;
			break;

		case wimp_KEY_DELETE:
			if (ro_gui_ctrl_pressed())
				c = NS_KEY_DELETE_LINE_START;
			else if (os_version < RISCOS5)
				c = NS_KEY_DELETE_LEFT;
			break;

		default:
			break;
		}
	}

	if (!(c & IS_WIMP_KEY)) {
		res = ro_cw->key(ro_cw, c);
		if (res == NSERROR_OK) {
			ro_cw->toolbar_update(ro_cw);
			return true;
		}
	}

	return false;
}


/**
 * Update a corewindow toolbar to a new size.
 *
 * \param ctx Context as passed to toolbar creation.
 */
static void cw_tb_size(void *ctx)
{
	struct ro_corewindow *ro_cw;
	wimp_window_state state;
	os_error *error;

	ro_cw = (struct ro_corewindow *)ctx;

	ro_cw->origin_y = -(ro_toolbar_height(ro_cw->toolbar));

	state.w = ro_cw->wh;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		return;
	}

	error = xwimp_force_redraw(ro_cw->wh,
				   0, state.visible.y0 - state.visible.y1,
				   state.visible.x1 - state.visible.x0, 0);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_force_redraw: 0x%x: %s",
		      error->errnum, error->errmess);
		return;
	}
}


/**
 * Update a corewindow toolbar to use a new theme.
 *
 * \param ctx Context as passed to toolbar creation.
 * \param exists true if the bar still exists; else false.
 */
static void cw_tb_theme(void *ctx, bool exists)
{
	if (exists) {
		cw_tb_size(ctx);
	}
}


/**
 * Allow a corewindow toolbar button state to be updated.
 *
 * \param ctx Context as passed to toolbar creation.
 */
static void cw_tb_update(void *ctx)
{
	struct ro_corewindow *ro_cw;

	ro_cw = (struct ro_corewindow *)ctx;

	ro_cw->toolbar_update(ro_cw);
}


/**
 * Respond to user actions (click) in a corewindow.
 *
 * \param ctx Context as passed to toolbar creation.
 * \param action_type type of action on toolbar
 * \param action data for action.
 */
static void
cw_tb_click(void *ctx,
	    toolbar_action_type action_type,
	    union toolbar_action action)
{
	struct ro_corewindow *ro_cw;

	ro_cw = (struct ro_corewindow *)ctx;

	if (action_type == TOOLBAR_ACTION_BUTTON) {
		ro_cw->toolbar_click(ro_cw, action.button);
		ro_cw->toolbar_update(ro_cw);
	}
}


/**
 * Save positions of core window toolbar buttons.
 *
 * \param ctx Context as passed to toolbar creation.
 * \param config The new button config string.
 */
static void cw_tb_save(void *ctx, char *config)
{
	struct ro_corewindow *ro_cw;

	ro_cw = (struct ro_corewindow *)ctx;

	ro_cw->toolbar_save(ro_cw, config);
}



/**
 * riscos core window toolbar callbacks
 */
static const struct toolbar_callbacks corewindow_toolbar_callbacks = {
	.theme_update = cw_tb_theme,
	.change_size = cw_tb_size,
	.update_buttons = cw_tb_update,
	.user_action = cw_tb_click,
	.save_buttons = cw_tb_save,
};


/**
 * callback from core to request an invalidation of a window area.
 *
 * The specified area of the window should now be considered
 *  out of date. If the area is NULL the entire window must be
 *  invalidated.
 *
 * \param[in] cw The core window to invalidate.
 * \param[in] r area to redraw or NULL for the entire window area.
 * \return NSERROR_OK on success or appropriate error code.
 */
static nserror
ro_cw_invalidate(struct core_window *cw, const struct rect *r)
{
	struct ro_corewindow *ro_cw = (struct ro_corewindow *)cw;
	os_error *error;
	wimp_window_info info;

	if (r == NULL) {
		info.w = ro_cw->wh;
		error = xwimp_get_window_info_header_only(&info);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xwimp_get_window_info_header_only: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			return NSERROR_INVALID;
		}
	} else {
		/* convert the passed rectangle into RO window dimensions */
		info.extent.x0 = 2 * r->x0;
		info.extent.y0 = (-2 * (r->y0 + (r->y1 - r->y0))) + ro_cw->origin_y;
		info.extent.x1 = 2 * (r->x0 + (r->x1 - r->x0));
		info.extent.y1 = (-2 * r->y0) + ro_cw->origin_y;
	}

	error = xwimp_force_redraw(ro_cw->wh,
				   info.extent.x0, info.extent.y0,
				   info.extent.x1, info.extent.y1);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_force_redraw: 0x%x: %s",
		      error->errnum, error->errmess);
		return NSERROR_INVALID;
	}
	return NSERROR_OK;
}


/**
 * Callback from the core to update the content area size.
 */
static nserror
ro_cw_update_size(struct core_window *cw, int width, int height)
{
	struct ro_corewindow *ro_cw = (struct ro_corewindow *)cw;
	wimp_open open;
	wimp_window_state state;
	os_error *error;

	NSLOG(netsurf, INFO, "content resize from w:%d h:%d to w:%d h:%d",
	      ro_cw->content_width, ro_cw->content_height, width, height);

	ro_cw->content_width = width * 2;
	ro_cw->content_height = -(2 * height);

	state.w = ro_cw->wh;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		return NSERROR_INVALID;
	}

	/* only update the window if it is open */
	if (state.flags & wimp_WINDOW_OPEN) {
		open.w = ro_cw->wh;
		open.visible = state.visible;
		open.xscroll = state.xscroll;
		open.yscroll = state.yscroll;
		open.next = state.next;

		update_scrollbars(ro_cw, &open);
	}
	return NSERROR_OK;
}


/**
 * Callback from the core to scroll the visible content.
 */
static nserror
ro_cw_get_scroll(const struct core_window *cw, int *x, int *y)
{
	struct ro_corewindow *ro_cw = (struct ro_corewindow *)cw;
	wimp_window_state state = {
		.w = ro_cw->wh,
	};
	os_error *error;

	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, ERROR, "xwimp_get_window_state: 0x%x: %s",
				error->errnum, error->errmess);
		return NSERROR_INVALID;
	}

	*x =  state.xscroll / 2;
	*y = -state.yscroll / 2;
	return NSERROR_OK;
}


/**
 * Callback from the core to scroll the visible content.
 */
static nserror
ro_cw_set_scroll(struct core_window *cw, int x, int y)
{
	struct ro_corewindow *ro_cw = (struct ro_corewindow *)cw;
	wimp_window_state state = {
		.w = ro_cw->wh,
	};
	os_error *error;

	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, ERROR, "xwimp_get_window_state: 0x%x: %s",
				error->errnum, error->errmess);
		return NSERROR_INVALID;
	}

	state.xscroll =  x * 2;
	state.yscroll = -y * 2;

	/* only update the window if it is open */
	if (state.flags & wimp_WINDOW_OPEN) {
		update_scrollbars(ro_cw, PTR_WIMP_OPEN(&state));
	}

	return NSERROR_OK;
}


/**
 * Callback from the core to obtain the window viewport dimensions
 *
 * \param[in] cw the core window object
 * \param[out] width to be set to viewport width in px
 * \param[out] height to be set to viewport height in px
 */
static nserror
ro_cw_get_window_dimensions(const struct core_window *cw,
		int *width, int *height)
{
	struct ro_corewindow *ro_cw = (struct ro_corewindow *)cw;
	os_error *error;
	wimp_window_state state;

	state.w = ro_cw->wh;
	error = xwimp_get_window_state(&state);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_get_window_state: 0x%x: %s",
		      error->errnum, error->errmess);
		return NSERROR_INVALID;
	}

	*width = (state.visible.x1 - state.visible.x0) / 2;
	*height = (state.visible.y1 - state.visible.y0) / 2;

	/* Account for toolbar height, if present */
	if (ro_cw->toolbar != NULL) {
		*height -= ro_toolbar_full_height(ro_cw->toolbar) / 2;
	}
	if (*height < 0) {
		*height = 0;
	}

	return NSERROR_OK;
}


/**
 * Callback from the core to update the drag status.
 */
static nserror
ro_cw_drag_status(struct core_window *cw, core_window_drag_status ds)
{
	struct ro_corewindow *ro_cw = (struct ro_corewindow *)cw;
	ro_cw->drag_status = ds;

	return NSERROR_OK;
}


struct core_window_callback_table ro_cw_cb_table = {
	.invalidate = ro_cw_invalidate,
	.update_size = ro_cw_update_size,
	.set_scroll = ro_cw_set_scroll,
	.get_scroll = ro_cw_get_scroll,
	.get_window_dimensions = ro_cw_get_window_dimensions,
	.drag_status = ro_cw_drag_status
};

/**
 * dummy toolbar click callback
 *
 */
static nserror dummy_toolbar_click(struct ro_corewindow *ro_cw, button_bar_action action)
{
	return NSERROR_OK;
}

/**
 * dummy toolbar update callback
 */
static nserror dummy_toolbar_update(struct ro_corewindow *ro_cw)
{
	return NSERROR_OK;
}

/**
 * dummy toolbar save callback
 */
static nserror dummy_toolbar_save(struct ro_corewindow *ro_cw, char *config)
{
	return NSERROR_OK;
}

/* exported function documented ro/corewindow.h */
nserror
ro_corewindow_init(struct ro_corewindow *ro_cw,
		   const struct button_bar_buttons *tb_buttons,
		   char *tb_order,
		   theme_style tb_style,
		   const char *tb_help)
{
	/* setup the core window callback table */
	ro_cw->cb_table = &ro_cw_cb_table;

	/* start with the content area being as small as possible */
	ro_cw->content_width = -1;
	ro_cw->content_height = -1;
	ro_cw->origin_y = 0; /* no offset */
	ro_cw->drag_status = CORE_WINDOW_DRAG_NONE; /* no drag */

	/* Create toolbar. */
	if (tb_buttons != NULL) {
		/* ensure toolbar callbacks are always valid so calls
		 * do not have to be conditional
		 */
		if (ro_cw->toolbar_click == NULL) {
			ro_cw->toolbar_click = dummy_toolbar_click;
		}
		if (ro_cw->toolbar_save == NULL) {
			ro_cw->toolbar_save = dummy_toolbar_save;
		}
		if (ro_cw->toolbar_update == NULL) {
			ro_cw->toolbar_update = dummy_toolbar_update;
		}

		ro_cw->toolbar = ro_toolbar_create(NULL,
						   ro_cw->wh,
						   tb_style,
						   TOOLBAR_FLAGS_NONE,
						   &corewindow_toolbar_callbacks,
						   ro_cw,
						   tb_help);
		if (ro_cw->toolbar == NULL) {
			return NSERROR_INIT_FAILED;
		}

		ro_toolbar_add_buttons(ro_cw->toolbar, tb_buttons, tb_order);
		ro_toolbar_rebuild(ro_cw->toolbar);
		ro_cw->origin_y = -(ro_toolbar_height(ro_cw->toolbar));
	} else {
		ro_cw->toolbar = NULL; /* no toolbar */

		/* ensure callback functions are set to defaults when
		 * no toolbar
		 */
		ro_cw->toolbar_click = dummy_toolbar_click;
		ro_cw->toolbar_save = dummy_toolbar_save;
		ro_cw->toolbar_update = dummy_toolbar_update;
	}

	/* setup context for event handlers */
	NSLOG(netsurf, INFO, "Setting corewindow %p for window handle %p",
			ro_cw, ro_cw->wh);
	ro_gui_wimp_event_set_user_data(ro_cw->wh, ro_cw);

	/* register wimp events. */
	ro_gui_wimp_event_register_redraw_window(ro_cw->wh,
			ro_cw_redraw);
	ro_gui_wimp_event_register_scroll_window(ro_cw->wh,
			ro_cw_scroll);
	ro_gui_wimp_event_register_pointer_entering_window(ro_cw->wh,
			ro_cw_pointer_entering);
	ro_gui_wimp_event_register_open_window(ro_cw->wh,
			ro_cw_open);
	ro_gui_wimp_event_register_mouse_click(ro_cw->wh,
			ro_cw_mouse_click);
	ro_gui_wimp_event_register_keypress(ro_cw->wh,
			ro_cw_keypress);

	return NSERROR_OK;
}

/* exported interface documented in ro/corewindow.h */
nserror ro_corewindow_fini(struct ro_corewindow *ro_cw)
{
	ro_gui_wimp_event_finalise(ro_cw->wh);

	/** \todo need to consider freeing of toolbar */

	return NSERROR_OK;
}
