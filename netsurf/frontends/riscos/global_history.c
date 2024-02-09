/*
 * Copyright 2010 Stephen Fryatt <stevef@netsurf-browser.org>
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
 * Implementation of RISC OS global history.
 */

#include <stdint.h>
#include <stdlib.h>
#include <oslib/wimp.h>

#include "utils/nsoption.h"
#include "utils/messages.h"
#include "utils/log.h"
#include "netsurf/window.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"
#include "desktop/global_history.h"

#include "riscos/dialog.h"
#include "riscos/gui.h"
#include "riscos/menus.h"
#include "riscos/save.h"
#include "riscos/toolbar.h"
#include "riscos/wimp.h"
#include "riscos/wimp_event.h"
#include "riscos/corewindow.h"
#include "riscos/global_history.h"

struct ro_global_history_window {
	struct ro_corewindow core;
	wimp_menu *menu;
};

/** global_history window is a singleton */
static struct ro_global_history_window *global_history_window = NULL;

/** riscos template for global_history window */
static wimp_window *dialog_global_history_template;


/**
 * callback to draw on drawable area of ro global_history window
 *
 * \param ro_cw The riscos core window structure.
 * \param r The rectangle of the window that needs updating.
 * \param originx The risc os plotter x origin.
 * \param originy The risc os plotter y origin.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
global_history_draw(struct ro_corewindow *ro_cw,
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
	global_history_redraw(0, 0, r, &ctx);
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
static nserror global_history_key(struct ro_corewindow *ro_cw, uint32_t nskey)
{
	if (global_history_keypress(nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * callback for mouse event on ro global_history window
 *
 * \param ro_cw The ro core window structure.
 * \param mouse_state mouse state
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on sucess otherwise apropriate error code.
 */
static nserror
global_history_mouse(struct ro_corewindow *ro_cw,
	     browser_mouse_state mouse_state,
	     int x, int y)
{
	global_history_mouse_action(mouse_state, x, y);

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
global_history_toolbar_click(struct ro_corewindow *ro_cw,
			     button_bar_action action)
{
	switch (action) {
	case TOOLBAR_BUTTON_DELETE:
		global_history_keypress(NS_KEY_DELETE_LEFT);
		break;

	case TOOLBAR_BUTTON_EXPAND:
		global_history_expand(false);
		break;

	case TOOLBAR_BUTTON_COLLAPSE:
		global_history_contract(false);
		break;

	case TOOLBAR_BUTTON_OPEN:
		global_history_expand(true);
		break;

	case TOOLBAR_BUTTON_CLOSE:
		global_history_contract(true);
		break;

	case TOOLBAR_BUTTON_LAUNCH:
		global_history_keypress(NS_KEY_CR);
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
static nserror global_history_toolbar_update(struct ro_corewindow *ro_cw)
{
	ro_toolbar_set_button_shaded_state(ro_cw->toolbar,
			TOOLBAR_BUTTON_DELETE,
			!global_history_has_selection());

	ro_toolbar_set_button_shaded_state(ro_cw->toolbar,
			TOOLBAR_BUTTON_LAUNCH,
			!global_history_has_selection());
	return NSERROR_OK;
}


/**
 * callback for saving of toolbar state in ro global history window
 *
 * \param ro_cw The ro core window structure.
 * \param config The new toolbar configuration.
 * \return NSERROR_OK if config saved, otherwise apropriate error code
 */
static nserror
global_history_toolbar_save(struct ro_corewindow *ro_cw, char *config)
{
	nsoption_set_charp(toolbar_history, config);
	ro_gui_save_options();

	return NSERROR_OK;
}


/**
 * Prepare the global_history menu for display
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu	The menu from which the selection was made.
 * \param pointer The pointer shape
 * \return true if action accepted; else false.
 */
static bool
global_history_menu_prepare(wimp_w w,
		    wimp_i i,
		    wimp_menu *menu,
		    wimp_pointer *pointer)
{
	bool selection;
	struct ro_global_history_window *global_historyw;

	global_historyw = (struct ro_global_history_window *)ro_gui_wimp_event_get_user_data(w);

	if ((global_historyw == NULL) ||
	    (menu != global_historyw->menu)) {
		return false;
	}

	selection = global_history_has_selection();

	ro_gui_menu_set_entry_shaded(menu, TREE_SELECTION, !selection);
	ro_gui_menu_set_entry_shaded(menu, TREE_CLEAR_SELECTION, !selection);

	ro_gui_save_prepare(GUI_SAVE_HISTORY_EXPORT_HTML,
			    NULL, NULL, NULL, NULL);

	ro_gui_menu_set_entry_shaded(menu, TOOLBAR_BUTTONS,
			ro_toolbar_menu_option_shade(global_historyw->core.toolbar));
	ro_gui_menu_set_entry_ticked(menu, TOOLBAR_BUTTONS,
			ro_toolbar_menu_buttons_tick(global_historyw->core.toolbar));

	ro_gui_menu_set_entry_shaded(menu, TOOLBAR_EDIT,
			ro_toolbar_menu_edit_shade(global_historyw->core.toolbar));
	ro_gui_menu_set_entry_ticked(menu, TOOLBAR_EDIT,
			ro_toolbar_menu_edit_tick(global_historyw->core.toolbar));

	return true;
}


/**
 * Handle submenu warnings for the global_history menu
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu The menu to which the warning applies.
 * \param selection The wimp menu selection data.
 * \param action The selected menu action.
 */
static void
global_history_menu_warning(wimp_w w,
		    wimp_i i,
		    wimp_menu *menu,
		    wimp_selection *selection,
		    menu_action action)
{
	/* Do nothing */
}


/**
 * Handle selections from the global_history menu
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu The menu from which the selection was made.
 * \param selection The wimp menu selection data.
 * \param action The selected menu action.
 * \return true if action accepted; else false.
 */
static bool
global_history_menu_select(wimp_w w,
		   wimp_i i,
		   wimp_menu *menu,
		   wimp_selection *selection,
		   menu_action action)
{
	struct ro_global_history_window *global_historyw;

	global_historyw = (struct ro_global_history_window *)ro_gui_wimp_event_get_user_data(w);

	if ((global_historyw == NULL) ||
	    (menu != global_historyw->menu)) {
		return false;
	}

	switch (action) {
	case HISTORY_EXPORT:
		ro_gui_dialog_open_persistent(w, dialog_saveas, true);
		return true;

	case TREE_EXPAND_ALL:
		global_history_expand(false);
		return true;

	case TREE_EXPAND_FOLDERS:
		global_history_expand(true);
		return true;

	case TREE_EXPAND_LINKS:
		global_history_expand(false);
		return true;

	case TREE_COLLAPSE_ALL:
		global_history_contract(true);
		return true;

	case TREE_COLLAPSE_FOLDERS:
		global_history_contract(true);
		return true;

	case TREE_COLLAPSE_LINKS:
		global_history_contract(false);
		return true;

	case TREE_SELECTION_LAUNCH:
		global_history_keypress(NS_KEY_CR);
		return true;

	case TREE_SELECTION_DELETE:
		global_history_keypress(NS_KEY_DELETE_LEFT);
		return true;

	case TREE_SELECT_ALL:
		global_history_keypress(NS_KEY_SELECT_ALL);
		return true;

	case TREE_CLEAR_SELECTION:
		global_history_keypress(NS_KEY_CLEAR_SELECTION);
		return true;

	case TOOLBAR_BUTTONS:
		ro_toolbar_set_display_buttons(global_historyw->core.toolbar,
			!ro_toolbar_get_display_buttons(global_historyw->core.toolbar));
		return true;

	case TOOLBAR_EDIT:
		ro_toolbar_toggle_edit(global_historyw->core.toolbar);
		return true;

	default:
		return false;
	}

	return false;
}


/**
 * Creates the window for the global_history tree.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror ro_global_history_init(void)
{
	os_error *error;
	struct ro_global_history_window *ncwin;
	nserror res;
	static const struct ns_menu global_history_menu_def = {
		"History", {
			{ "History", NO_ACTION, 0 },
			{ "_History.Export", HISTORY_EXPORT, &dialog_saveas },
			{ "History.Expand", TREE_EXPAND_ALL, 0 },
			{ "History.Expand.All", TREE_EXPAND_ALL, 0 },
			{ "History.Expand.Folders", TREE_EXPAND_FOLDERS, 0 },
			{ "History.Expand.Links", TREE_EXPAND_LINKS, 0 },
			{ "History.Collapse", TREE_COLLAPSE_ALL, 0 },
			{ "History.Collapse.All", TREE_COLLAPSE_ALL, 0 },
			{ "History.Collapse.Folders", TREE_COLLAPSE_FOLDERS, 0 },
			{ "History.Collapse.Links", TREE_COLLAPSE_LINKS, 0 },
			{ "History.Toolbars", NO_ACTION, 0 },
			{ "_History.Toolbars.ToolButtons", TOOLBAR_BUTTONS, 0 },
			{ "History.Toolbars.EditToolbar",TOOLBAR_EDIT, 0 },
			{ "Selection", TREE_SELECTION, 0 },
			{ "Selection.Launch", TREE_SELECTION_LAUNCH, 0 },
			{ "Selection.Delete", TREE_SELECTION_DELETE, 0 },
			{ "SelectAll", TREE_SELECT_ALL, 0 },
			{ "Clear", TREE_CLEAR_SELECTION, 0 },
			{ NULL, 0, 0}
		}
	};

	static const struct button_bar_buttons global_history_toolbar_buttons[] = {
		{ "delete", TOOLBAR_BUTTON_DELETE, TOOLBAR_BUTTON_NONE, '0', "0"},
		{ "expand", TOOLBAR_BUTTON_EXPAND, TOOLBAR_BUTTON_COLLAPSE, '1', "1"},
		{ "open", TOOLBAR_BUTTON_OPEN, TOOLBAR_BUTTON_CLOSE, '2', "2"},
		{ "launch", TOOLBAR_BUTTON_LAUNCH, TOOLBAR_BUTTON_NONE, '3', "3"},
		{ NULL, TOOLBAR_BUTTON_NONE, TOOLBAR_BUTTON_NONE, '\0', ""}
	};

	if (global_history_window != NULL) {
		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(*ncwin));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	/* create window from template */
	error = xwimp_create_window(dialog_global_history_template,
				    &ncwin->core.wh);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_create_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		free(ncwin);
		return NSERROR_NOMEM;
	}

	ro_gui_set_window_title(ncwin->core.wh, messages_get("GlobalHistory"));

	/* initialise callbacks */
	ncwin->core.draw = global_history_draw;
	ncwin->core.key = global_history_key;
	ncwin->core.mouse = global_history_mouse;
	ncwin->core.toolbar_click = global_history_toolbar_click;
	ncwin->core.toolbar_save = global_history_toolbar_save;
	/* update is not valid untill global history is initialised */
	ncwin->core.toolbar_update = NULL;

	/* initialise core window */
	res = ro_corewindow_init(&ncwin->core,
				 global_history_toolbar_buttons,
				 nsoption_charp(toolbar_history),
				 THEME_STYLE_GLOBAL_HISTORY_TOOLBAR,
				 "HelpGHistoryToolbar");
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	NSLOG(netsurf, INFO, "Created global history corewindow: %p",
			&ncwin->core);

	res = global_history_init(ncwin->core.cb_table,
				  (struct core_window *)ncwin);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	/* setup toolbar update post global_history manager initialisation */
	ncwin->core.toolbar_update = global_history_toolbar_update;
	global_history_toolbar_update(&ncwin->core);

	/* Build the global_history window menu. */
	ncwin->menu = ro_gui_menu_define_menu(&global_history_menu_def);

	ro_gui_wimp_event_register_menu(ncwin->core.wh,
					ncwin->menu, false, false);
	ro_gui_wimp_event_register_menu_prepare(ncwin->core.wh,
						global_history_menu_prepare);
	ro_gui_wimp_event_register_menu_selection(ncwin->core.wh,
						  global_history_menu_select);
	ro_gui_wimp_event_register_menu_warning(ncwin->core.wh,
						global_history_menu_warning);

	/* memoise window so it can be represented when necessary
	 * instead of recreating every time.
	 */
	global_history_window = ncwin;

	return NSERROR_OK;
}


/* exported interface documented in riscos/global_history.h */
nserror ro_gui_global_history_present(void)
{
	nserror res;

	res = ro_global_history_init();
	if (res == NSERROR_OK) {
		NSLOG(netsurf, INFO, "Presenting");
		ro_gui_dialog_open_top(global_history_window->core.wh,
				       global_history_window->core.toolbar,
				       600, 800);
	} else {
		NSLOG(netsurf, INFO, "Failed presenting code %d", res);
	}

	return res;
}


/* exported interface documented in riscos/global_history.h */
void ro_gui_global_history_initialise(void)
{
	dialog_global_history_template = ro_gui_dialog_load_template("tree");
}


/* exported interface documented in riscos/global_history.h */
nserror ro_gui_global_history_finalise(void)
{
	nserror res;

	if (global_history_window == NULL) {
		return NSERROR_OK;
	}

	res = global_history_fini();
	if (res == NSERROR_OK) {
		res = ro_corewindow_fini(&global_history_window->core);

		free(global_history_window);
		global_history_window = NULL;
	}

	return res;
}


/* exported interface documented in riscos/global_history.h */
bool ro_gui_global_history_check_window(wimp_w wh)
{
	if ((global_history_window != NULL) &&
	    (global_history_window->core.wh == wh)) {
		return true;
	}
	return false;
}


/* exported interface documented in riscos/global_history.h */
bool ro_gui_global_history_check_menu(wimp_menu *menu)
{
	if ((global_history_window != NULL) &&
	    (global_history_window->menu == menu)) {
		return true;
	}
	return false;
}
