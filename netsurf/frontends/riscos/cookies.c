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
 * Implementation of RISC OS cookie manager.
 */

#include <stdint.h>
#include <stdlib.h>
#include <oslib/wimp.h>

#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/messages.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"
#include "desktop/cookie_manager.h"

#include "riscos/gui.h"
#include "riscos/wimp.h"
#include "riscos/wimp_event.h"
#include "riscos/dialog.h"
#include "riscos/toolbar.h"
#include "riscos/corewindow.h"
#include "riscos/cookies.h"

struct ro_cookie_window {
	struct ro_corewindow core;
	wimp_menu *menu;
};

/** cookie window is a singleton */
static struct ro_cookie_window *cookie_window = NULL;

/** riscos template for cookie window */
static wimp_window *dialog_cookie_template;


/**
 * callback to draw on drawable area of ro cookie window
 *
 * \param ro_cw The riscos core window structure.
 * \param r The rectangle of the window that needs updating.
 * \param originx The risc os plotter x origin.
 * \param originy The risc os plotter y origin.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
cookie_draw(struct ro_corewindow *ro_cw,
	       int originx,
	       int originy,
	       struct rect *r)
{
	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &ro_plotters
	};

	ro_plot_origin_x = originx;
	ro_plot_origin_y = originy;
	no_font_blending = true;
	cookie_manager_redraw(0, 0, r, &ctx);
	no_font_blending = false;

	return NSERROR_OK;
}


/**
 * callback for keypress on ro cookie window
 *
 * \param ro_cw The ro core window structure.
 * \param nskey The netsurf key code.
 * \return NSERROR_OK if key processed,
 *         NSERROR_NOT_IMPLEMENTED if key not processed
 *         otherwise apropriate error code
 */
static nserror cookie_key(struct ro_corewindow *ro_cw, uint32_t nskey)
{
	if (cookie_manager_keypress(nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * callback for mouse event on ro cookie window
 *
 * \param ro_cw The ro core window structure.
 * \param mouse_state mouse state
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on sucess otherwise apropriate error code.
 */
static nserror
cookie_mouse(struct ro_corewindow *ro_cw,
	     browser_mouse_state mouse_state,
	     int x, int y)
{
	cookie_manager_mouse_action(mouse_state, x, y);

	return NSERROR_OK;
}


/**
 * handle clicks in ro core window toolbar.
 *
 * \param ro_cw The ro core window structure.
 * \param action The button bar action.
 * \return NSERROR_OK if config saved, otherwise apropriate error code
 */
static nserror
cookie_toolbar_click(struct ro_corewindow *ro_cw, button_bar_action action)
{
	switch (action) {
	case TOOLBAR_BUTTON_DELETE:
		cookie_manager_keypress(NS_KEY_DELETE_LEFT);
		break;

	case TOOLBAR_BUTTON_EXPAND:
		cookie_manager_expand(false);
		break;

	case TOOLBAR_BUTTON_COLLAPSE:
		cookie_manager_contract(false);
		break;

	case TOOLBAR_BUTTON_OPEN:
		cookie_manager_expand(true);
		break;

	case TOOLBAR_BUTTON_CLOSE:
		cookie_manager_contract(true);
		break;

	default:
		break;
	}

	return NSERROR_OK;
}


/**
 * Handle updating state of buttons in ro core window toolbar.
 *
 * \param ro_cw The ro core window structure.
 * \return NSERROR_OK if config saved, otherwise apropriate error code
 */
static nserror cookie_toolbar_update(struct ro_corewindow *ro_cw)
{
	ro_toolbar_set_button_shaded_state(ro_cw->toolbar,
			TOOLBAR_BUTTON_DELETE,
			!cookie_manager_has_selection());
	return NSERROR_OK;
}


/**
 * callback for saving of toolbar state in ro cookie window
 *
 * \param ro_cw The ro core window structure.
 * \param config The new toolbar configuration.
 * \return NSERROR_OK if config saved, otherwise apropriate error code
 */
static nserror cookie_toolbar_save(struct ro_corewindow *ro_cw, char *config)
{
	nsoption_set_charp(toolbar_cookies, config);
	ro_gui_save_options();

	return NSERROR_OK;
}


/**
 * Prepare the cookie meu for display
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu	The menu from which the selection was made.
 * \param pointer The pointer shape
 * \return true if action accepted; else false.
 */
static bool
cookie_menu_prepare(wimp_w w,
		    wimp_i i,
		    wimp_menu *menu,
		    wimp_pointer *pointer)
{
	bool selection;
	struct ro_cookie_window *cookiew;

	cookiew = (struct ro_cookie_window *)ro_gui_wimp_event_get_user_data(w);

	if ((cookiew == NULL) ||
	    (menu != cookiew->menu)) {
		return false;
	}

	selection = cookie_manager_has_selection();

	ro_gui_menu_set_entry_shaded(menu, TREE_SELECTION, !selection);
	ro_gui_menu_set_entry_shaded(menu, TREE_CLEAR_SELECTION, !selection);

	ro_gui_menu_set_entry_shaded(menu, TOOLBAR_BUTTONS,
			ro_toolbar_menu_option_shade(cookiew->core.toolbar));
	ro_gui_menu_set_entry_ticked(menu, TOOLBAR_BUTTONS,
			ro_toolbar_menu_buttons_tick(cookiew->core.toolbar));

	ro_gui_menu_set_entry_shaded(menu, TOOLBAR_EDIT,
			ro_toolbar_menu_edit_shade(cookiew->core.toolbar));
	ro_gui_menu_set_entry_ticked(menu, TOOLBAR_EDIT,
			ro_toolbar_menu_edit_tick(cookiew->core.toolbar));

	return true;
}


/**
 * Handle submenu warnings for the cookies menu
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu The menu to which the warning applies.
 * \param selection The wimp menu selection data.
 * \param action The selected menu action.
 */
static void
cookie_menu_warning(wimp_w w,
		    wimp_i i,
		    wimp_menu *menu,
		    wimp_selection *selection,
		    menu_action action)
{
	/* Do nothing */
}


/**
 * Handle selections from the cookies menu
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu The menu from which the selection was made.
 * \param selection The wimp menu selection data.
 * \param action The selected menu action.
 * \return true if action accepted; else false.
 */
static bool
cookie_menu_select(wimp_w w,
		   wimp_i i,
		   wimp_menu *menu,
		   wimp_selection *selection,
		   menu_action action)
{
	struct ro_cookie_window *cookiew;

	cookiew = (struct ro_cookie_window *)ro_gui_wimp_event_get_user_data(w);

	if ((cookiew == NULL) ||
	    (menu != cookiew->menu)) {
		return false;
	}

	switch (action) {
	case TREE_EXPAND_ALL:
		cookie_manager_expand(false);
		return true;

	case TREE_EXPAND_FOLDERS:
		cookie_manager_expand(true);
		return true;

	case TREE_EXPAND_LINKS:
		cookie_manager_expand(false);
		return true;

	case TREE_COLLAPSE_ALL:
		cookie_manager_contract(true);
		return true;

	case TREE_COLLAPSE_FOLDERS:
		cookie_manager_contract(true);
		return true;

	case TREE_COLLAPSE_LINKS:
		cookie_manager_contract(false);
		return true;

	case TREE_SELECTION_DELETE:
		cookie_manager_keypress(NS_KEY_DELETE_LEFT);
		return true;

	case TREE_SELECT_ALL:
		cookie_manager_keypress(NS_KEY_SELECT_ALL);
		return true;

	case TREE_CLEAR_SELECTION:
		cookie_manager_keypress(NS_KEY_CLEAR_SELECTION);
		return true;

	case TOOLBAR_BUTTONS:
		ro_toolbar_set_display_buttons(cookiew->core.toolbar,
			!ro_toolbar_get_display_buttons(cookiew->core.toolbar));
		return true;

	case TOOLBAR_EDIT:
		ro_toolbar_toggle_edit(cookiew->core.toolbar);
		return true;

	default:
		return false;
	}

	return false;
}


/**
 * Creates the window for the cookie tree.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror ro_cookie_init(void)
{
	os_error *error;
	struct ro_cookie_window *ncwin;
	nserror res;
	static const struct ns_menu cookie_menu_def = {
		"Cookies", {
			{ "Cookies", NO_ACTION, 0 },
			{ "Cookies.Expand", TREE_EXPAND_ALL, 0 },
			{ "Cookies.Expand.All", TREE_EXPAND_ALL, 0 },
			{ "Cookies.Expand.Folders", TREE_EXPAND_FOLDERS, 0 },
			{ "Cookies.Expand.Links", TREE_EXPAND_LINKS, 0 },
			{ "Cookies.Collapse", TREE_COLLAPSE_ALL, 0 },
			{ "Cookies.Collapse.All", TREE_COLLAPSE_ALL, 0 },
			{ "Cookies.Collapse.Folders", TREE_COLLAPSE_FOLDERS, 0 },
			{ "Cookies.Collapse.Links", TREE_COLLAPSE_LINKS, 0 },
			{ "Cookies.Toolbars", NO_ACTION, 0 },
			{ "_Cookies.Toolbars.ToolButtons", TOOLBAR_BUTTONS, 0 },
			{ "Cookies.Toolbars.EditToolbar",TOOLBAR_EDIT, 0 },
			{ "Selection", TREE_SELECTION, 0 },
			{ "Selection.Delete", TREE_SELECTION_DELETE, 0 },
			{ "SelectAll", TREE_SELECT_ALL, 0 },
			{ "Clear", TREE_CLEAR_SELECTION, 0 },
			{ NULL, 0, 0}
		}
	};

	static const struct button_bar_buttons cookies_toolbar_buttons[] = {
		{ "delete", TOOLBAR_BUTTON_DELETE, TOOLBAR_BUTTON_NONE, '0', "0"},
		{ "expand", TOOLBAR_BUTTON_EXPAND, TOOLBAR_BUTTON_COLLAPSE, '1', "1"},
		{ "open", TOOLBAR_BUTTON_OPEN, TOOLBAR_BUTTON_CLOSE, '2', "2"},
		{ NULL, TOOLBAR_BUTTON_NONE, TOOLBAR_BUTTON_NONE, '\0', ""}
	};

	if (cookie_window != NULL) {
		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(*ncwin));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	/* create window from template */
	error = xwimp_create_window(dialog_cookie_template, &ncwin->core.wh);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_create_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		free(ncwin);
		return NSERROR_NOMEM;
	}

	ro_gui_set_window_title(ncwin->core.wh, messages_get("Cookies"));

	ncwin->core.draw = cookie_draw;
	ncwin->core.key = cookie_key;
	ncwin->core.mouse = cookie_mouse;
	ncwin->core.toolbar_click = cookie_toolbar_click;
	ncwin->core.toolbar_save = cookie_toolbar_save;
	/* update is not valid untill cookie manager is initialised */
	ncwin->core.toolbar_update = NULL;

	/* initialise core window */
	res = ro_corewindow_init(&ncwin->core,
				 cookies_toolbar_buttons,
				 nsoption_charp(toolbar_cookies),
				 THEME_STYLE_COOKIES_TOOLBAR,
				 "HelpCookiesToolbar");
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	res = cookie_manager_init(ncwin->core.cb_table,
				  (struct core_window *)ncwin);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	/* setup toolbar update post cookie manager initialisation */
	ncwin->core.toolbar_update = cookie_toolbar_update;
	cookie_toolbar_update(&ncwin->core);

	/* Build the cookies window menu. */
	ncwin->menu = ro_gui_menu_define_menu(&cookie_menu_def);

	ro_gui_wimp_event_register_menu(ncwin->core.wh,
					ncwin->menu, false, false);
	ro_gui_wimp_event_register_menu_prepare(ncwin->core.wh,
						cookie_menu_prepare);
	ro_gui_wimp_event_register_menu_selection(ncwin->core.wh,
						  cookie_menu_select);
	ro_gui_wimp_event_register_menu_warning(ncwin->core.wh,
						cookie_menu_warning);

	/* memoise window so it can be represented when necessary
	 * instead of recreating every time.
	 */
	cookie_window = ncwin;

	return NSERROR_OK;
}


/* exported interface documented in riscos/cookies.h */
nserror ro_gui_cookies_present(const char *search_term)
{
	nserror res;

	res = ro_cookie_init();
	if (res == NSERROR_OK) {
		NSLOG(netsurf, INFO, "Presenting");
		ro_gui_dialog_open_top(cookie_window->core.wh,
				       cookie_window->core.toolbar,
				       600, 800);
		if (search_term != NULL) {
			res = cookie_manager_set_search_string(search_term);
		}
	} else {
		NSLOG(netsurf, INFO, "Failed presenting code %d", res);
	}

	return res;
}


/* exported interface documented in riscos/cookies.h */
void ro_gui_cookies_initialise(void)
{
	dialog_cookie_template = ro_gui_dialog_load_template("tree");
}


/* exported interface documented in riscos/cookies.h */
nserror ro_gui_cookies_finalise(void)
{
	nserror res;

	if (cookie_window == NULL) {
		return NSERROR_OK;
	}

	res = cookie_manager_fini();
	if (res == NSERROR_OK) {
		res = ro_corewindow_fini(&cookie_window->core);

		free(cookie_window);
		cookie_window = NULL;
	}

	return res;
}


/* exported interface documented in riscos/cookies.h */
bool ro_gui_cookies_check_window(wimp_w wh)
{
	if ((cookie_window != NULL) &&
	    (cookie_window->core.wh == wh)) {
		return true;
	}
	return false;
}


/* exported interface documented in riscos/cookies.h */
bool ro_gui_cookies_check_menu(wimp_menu *menu)
{
	if ((cookie_window != NULL) &&
	    (cookie_window->menu == menu)) {
		return true;
	}
	return false;
}
