/*
 * Copyright 2010, 2013 Stephen Fryatt <stevef@netsurf-browser.org>
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
 * Implementation of RISC OS hotlist manager.
 */

#include <stdint.h>
#include <stdlib.h>
#include <oslib/osmodule.h>
#include <oslib/wimp.h>

#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "netsurf/url_db.h"
#include "netsurf/window.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"
#include "desktop/hotlist.h"

#include "riscos/gui.h"
#include "riscos/dialog.h"
#include "riscos/message.h"
#include "riscos/save.h"
#include "riscos/toolbar.h"
#include "riscos/wimp.h"
#include "riscos/wimp_event.h"
#include "riscos/query.h"
#include "riscos/menus.h"
#include "riscos/corewindow.h"
#include "riscos/hotlist.h"

/**
 * Hotlist window container for RISC OS.
 */
struct ro_hotlist_window {
	struct ro_corewindow core;
	wimp_menu *menu;
};

/** hotlist window singleton */
static struct ro_hotlist_window *hotlist_window = NULL;

/** riscos template for hotlist window */
static wimp_window *dialog_hotlist_template;

/** Hotlist Query Handler. */
static query_id	hotlist_query = QUERY_INVALID;
static nsurl *hotlist_delete_url = NULL;

/**
 * URL adding hotlist protocol message block.
 *
 * Message block is not currently in OSLib.
 */
struct ro_hotlist_message_hotlist_addurl {
	wimp_MESSAGE_HEADER_MEMBERS /**< The standard message header. */
	char *url; /**< Pointer to the URL in RMA. */
	char *title; /**< Pointer to the title in RMA. */
	char appname[32]; /**< The application name. */
};

/**
 * change hotlist protocol message block.
 *
 * Message block is not currently in OSLib.
 */
struct ro_hotlist_message_hotlist_changed {
	wimp_MESSAGE_HEADER_MEMBERS /**< The standard message header. */
};

/** URL area claimed from RMA. */
static char *hotlist_url = NULL;
/** Title area claimed from RMA. */
static char *hotlist_title = NULL;


/**
 * callback to draw on drawable area of ro hotlist window
 *
 * \param ro_cw The riscos core window structure.
 * \param r The rectangle of the window that needs updating.
 * \param originx The risc os plotter x origin.
 * \param originy The risc os plotter y origin.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
hotlist_draw(struct ro_corewindow *ro_cw,
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
	hotlist_redraw(0, 0, r, &ctx);
	no_font_blending = false;

	return NSERROR_OK;
}


/**
 * callback for keypress on ro hotlist window
 *
 * \param ro_cw The ro core window structure.
 * \param nskey The netsurf key code.
 * \return NSERROR_OK if key processed,
 *         NSERROR_NOT_IMPLEMENTED if key not processed
 *         otherwise apropriate error code
 */
static nserror hotlist_key(struct ro_corewindow *ro_cw, uint32_t nskey)
{
	if (hotlist_keypress(nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * callback for mouse event on ro hotlist window
 *
 * \param ro_cw The ro core window structure.
 * \param mouse_state mouse state
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on sucess otherwise apropriate error code.
 */
static nserror
hotlist_mouse(struct ro_corewindow *ro_cw,
	      browser_mouse_state mouse_state,
	      int x, int y)
{
	hotlist_mouse_action(mouse_state, x, y);

	return NSERROR_OK;
}


/**
 * handle clicks in ro hotlist window toolbar.
 *
 * \param ro_cw The ro core window structure.
 * \param action The button bar action.
 * \return NSERROR_OK if config saved, otherwise apropriate error code
 */
static nserror
hotlist_toolbar_click(struct ro_corewindow *ro_cw, button_bar_action action)
{
	switch (action) {
	case TOOLBAR_BUTTON_DELETE:
		hotlist_keypress(NS_KEY_DELETE_LEFT);
		ro_toolbar_update_all_hotlists();
		break;

	case TOOLBAR_BUTTON_EXPAND:
		hotlist_expand(false);
		break;

	case TOOLBAR_BUTTON_COLLAPSE:
		hotlist_contract(false);
		break;

	case TOOLBAR_BUTTON_OPEN:
		hotlist_expand(true);
		break;

	case TOOLBAR_BUTTON_CLOSE:
		hotlist_contract(true);
		break;

	case TOOLBAR_BUTTON_LAUNCH:
		hotlist_keypress(NS_KEY_CR);
		break;

	case TOOLBAR_BUTTON_CREATE:
		hotlist_add_folder(NULL, false, 0);
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
static nserror hotlist_toolbar_update(struct ro_corewindow *ro_cw)
{
	bool has_selection;

	has_selection = hotlist_has_selection();

	ro_toolbar_set_button_shaded_state(ro_cw->toolbar,
					   TOOLBAR_BUTTON_DELETE,
					   !has_selection);

	ro_toolbar_set_button_shaded_state(ro_cw->toolbar,
					   TOOLBAR_BUTTON_LAUNCH,
					   !has_selection);

	return NSERROR_OK;
}


/**
 * callback for saving of toolbar state in ro hotlist window
 *
 * \param ro_cw The ro core window structure.
 * \param config The new toolbar configuration.
 * \return NSERROR_OK if config saved, otherwise apropriate error code
 */
static nserror hotlist_toolbar_save(struct ro_corewindow *ro_cw, char *config)
{
	nsoption_set_charp(toolbar_hotlist, config);
	ro_gui_save_options();

	return NSERROR_OK;
}


/**
 * Prepare the hotlist menu for display
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu	The menu from which the selection was made.
 * \param pointer The pointer shape
 * \return true if action accepted; else false.
 */
static bool
hotlist_menu_prepare(wimp_w w, wimp_i i, wimp_menu *menu, wimp_pointer *pointer)
{
	bool selection;
	struct ro_hotlist_window *hotlistw;

	hotlistw = (struct ro_hotlist_window *)ro_gui_wimp_event_get_user_data(w);

	if ((hotlistw == NULL) ||
	    (menu != hotlistw->menu)) {
		return false;
	}

	selection = hotlist_has_selection();

	ro_gui_menu_set_entry_shaded(menu, TREE_SELECTION, !selection);
	ro_gui_menu_set_entry_shaded(menu, TREE_CLEAR_SELECTION, !selection);

	ro_gui_save_prepare(GUI_SAVE_HOTLIST_EXPORT_HTML,
			    NULL, NULL, NULL, NULL);

	ro_gui_menu_set_entry_shaded(menu, TOOLBAR_BUTTONS,
			ro_toolbar_menu_option_shade(hotlistw->core.toolbar));
	ro_gui_menu_set_entry_ticked(menu, TOOLBAR_BUTTONS,
			ro_toolbar_menu_buttons_tick(hotlistw->core.toolbar));

	ro_gui_menu_set_entry_shaded(menu, TOOLBAR_EDIT,
			ro_toolbar_menu_edit_shade(hotlistw->core.toolbar));
	ro_gui_menu_set_entry_ticked(menu, TOOLBAR_EDIT,
			ro_toolbar_menu_edit_tick(hotlistw->core.toolbar));

	return true;
}


/**
 * Handle submenu warnings for the hotlist menu
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu The menu to which the warning applies.
 * \param selection The wimp menu selection data.
 * \param action The selected menu action.
 */
static void
hotlist_menu_warning(wimp_w w,
		     wimp_i i,
		     wimp_menu *menu,
		     wimp_selection *selection,
		     menu_action action)
{
	/* Do nothing */
}


/**
 * Handle selections from the hotlist menu
 *
 * \param w The window owning the menu.
 * \param i The icon owning the menu.
 * \param menu The menu from which the selection was made.
 * \param selection The wimp menu selection data.
 * \param action The selected menu action.
 * \return true if action accepted; else false.
 */
static bool
hotlist_menu_select(wimp_w w,
		    wimp_i i,
		    wimp_menu *menu,
		    wimp_selection *selection,
		    menu_action action)
{
	struct ro_hotlist_window *hotlistw;

	hotlistw = (struct ro_hotlist_window *)ro_gui_wimp_event_get_user_data(w);

	if ((hotlistw == NULL) ||
	    (menu != hotlistw->menu)) {
		return false;
	}

	switch (action) {
	case HOTLIST_EXPORT:
		ro_gui_dialog_open_persistent(w, dialog_saveas, true);
		return true;

	case TREE_NEW_FOLDER:
		hotlist_add_folder(NULL, false, 0);
		return true;

	case TREE_NEW_LINK:
		hotlist_add_entry(NULL, NULL, false, 0);
		return true;

	case TREE_EXPAND_ALL:
		hotlist_expand(false);
		return true;

	case TREE_EXPAND_FOLDERS:
		hotlist_expand(true);
		return true;

	case TREE_EXPAND_LINKS:
		hotlist_expand(false);
		return true;

	case TREE_COLLAPSE_ALL:
		hotlist_contract(true);
		return true;

	case TREE_COLLAPSE_FOLDERS:
		hotlist_contract(true);
		return true;

	case TREE_COLLAPSE_LINKS:
		hotlist_contract(false);
		return true;

	case TREE_SELECTION_EDIT:
		hotlist_edit_selection();
		return true;

	case TREE_SELECTION_LAUNCH:
		hotlist_keypress(NS_KEY_CR);
		return true;

	case TREE_SELECTION_DELETE:
		hotlist_keypress(NS_KEY_DELETE_LEFT);
		ro_toolbar_update_all_hotlists();
		return true;

	case TREE_SELECT_ALL:
		hotlist_keypress(NS_KEY_SELECT_ALL);
		return true;

	case TREE_CLEAR_SELECTION:
		hotlist_keypress(NS_KEY_CLEAR_SELECTION);
		return true;

	case TOOLBAR_BUTTONS:
		ro_toolbar_set_display_buttons(hotlistw->core.toolbar,
			!ro_toolbar_get_display_buttons(hotlistw->core.toolbar));
		return true;

	case TOOLBAR_EDIT:
		ro_toolbar_toggle_edit(hotlistw->core.toolbar);
		return true;

	default:
		return false;
	}

	return false;
}


/**
 * Creates the window for the hotlist tree.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror ro_hotlist_init(void)
{
	os_error *error;
	struct ro_hotlist_window *ncwin;
	nserror res;
	static const struct ns_menu hotlist_menu_def = {
		"Hotlist", {
			{ "Hotlist", NO_ACTION, 0 },
			{ "Hotlist.New", NO_ACTION, 0 },
			{ "Hotlist.New.Folder", TREE_NEW_FOLDER, 0 },
			{ "Hotlist.New.Link", TREE_NEW_LINK, 0 },
			{ "_Hotlist.Export", HOTLIST_EXPORT, &dialog_saveas },
			{ "Hotlist.Expand", TREE_EXPAND_ALL, 0 },
			{ "Hotlist.Expand.All", TREE_EXPAND_ALL, 0 },
			{ "Hotlist.Expand.Folders", TREE_EXPAND_FOLDERS, 0 },
			{ "Hotlist.Expand.Links", TREE_EXPAND_LINKS, 0 },
			{ "Hotlist.Collapse", TREE_COLLAPSE_ALL, 0 },
			{ "Hotlist.Collapse.All", TREE_COLLAPSE_ALL, 0 },
			{ "Hotlist.Collapse.Folders", TREE_COLLAPSE_FOLDERS, 0 },
			{ "Hotlist.Collapse.Links", TREE_COLLAPSE_LINKS, 0 },
			{ "Hotlist.Toolbars", NO_ACTION, 0 },
			{ "_Hotlist.Toolbars.ToolButtons", TOOLBAR_BUTTONS, 0 },
			{ "Hotlist.Toolbars.EditToolbar", TOOLBAR_EDIT, 0 },
			{ "Selection", TREE_SELECTION, 0 },
			{ "Selection.Edit", TREE_SELECTION_EDIT, 0 },
			{ "Selection.Launch", TREE_SELECTION_LAUNCH, 0 },
			{ "Selection.Delete", TREE_SELECTION_DELETE, 0 },
			{ "SelectAll", TREE_SELECT_ALL, 0 },
			{ "Clear", TREE_CLEAR_SELECTION, 0 },
			{ NULL, 0, 0}
		}
	};

	static const struct button_bar_buttons hotlist_toolbar_buttons[] = {
		{ "delete", TOOLBAR_BUTTON_DELETE, TOOLBAR_BUTTON_NONE, '0', "0" },
		{ "expand", TOOLBAR_BUTTON_EXPAND, TOOLBAR_BUTTON_COLLAPSE, '1', "1" },
		{ "open", TOOLBAR_BUTTON_OPEN, TOOLBAR_BUTTON_CLOSE, '2', "2" },
		{ "launch", TOOLBAR_BUTTON_LAUNCH, TOOLBAR_BUTTON_NONE, '3', "3" },
		{ "create", TOOLBAR_BUTTON_CREATE, TOOLBAR_BUTTON_NONE, '4', "4" },
		{ NULL, TOOLBAR_BUTTON_NONE, TOOLBAR_BUTTON_NONE, '\0', "" }
	};

	if (hotlist_window != NULL) {
		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(*ncwin));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	/* create window from template */
	error = xwimp_create_window(dialog_hotlist_template, &ncwin->core.wh);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_create_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		free(ncwin);
		return NSERROR_NOMEM;
	}

	ro_gui_set_window_title(ncwin->core.wh, messages_get("Hotlist"));

	/* Set up callback handlers */
	ncwin->core.draw = hotlist_draw;
	ncwin->core.key = hotlist_key;
	ncwin->core.mouse = hotlist_mouse;
	ncwin->core.toolbar_click = hotlist_toolbar_click;
	ncwin->core.toolbar_save = hotlist_toolbar_save;
	/* update is not valid untill hotlist manager is initialised */
	ncwin->core.toolbar_update = NULL;

	/* initialise core window */
	res = ro_corewindow_init(&ncwin->core,
				 hotlist_toolbar_buttons,
				 nsoption_charp(toolbar_hotlist),
				 THEME_STYLE_HOTLIST_TOOLBAR,
				 "HelpHotToolbar");
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	res = hotlist_manager_init(ncwin->core.cb_table,
			   (struct core_window *)ncwin);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	/* setup toolbar update post hotlist manager initialisation */
	ncwin->core.toolbar_update = hotlist_toolbar_update;
	hotlist_toolbar_update(&ncwin->core);

	/* Build the hotlist window menu. */
	ncwin->menu = ro_gui_menu_define_menu(&hotlist_menu_def);

	ro_gui_wimp_event_register_menu(ncwin->core.wh,
					ncwin->menu, false, false);
	ro_gui_wimp_event_register_menu_prepare(ncwin->core.wh,
						hotlist_menu_prepare);
	ro_gui_wimp_event_register_menu_selection(ncwin->core.wh,
						  hotlist_menu_select);
	ro_gui_wimp_event_register_menu_warning(ncwin->core.wh,
						hotlist_menu_warning);

	/* memoise window so it can be re-presented when necessary
	 * instead of recreating every time.
	 */
	hotlist_window = ncwin;

	return NSERROR_OK;
}


/* exported interface documented in riscos/hotlist.h */
nserror ro_gui_hotlist_present(void)
{
	nserror res;

	/* deal with external hotlist handler */
	if (nsoption_bool(external_hotlists) &&
	    (nsoption_charp(external_hotlist_app) != NULL) &&
	    (*nsoption_charp(external_hotlist_app) != '\0')) {
		char command[2048];
		os_error *error;

		snprintf(command, sizeof(command), "Filer_Run %s",
			 nsoption_charp(external_hotlist_app));
		error = xos_cli(command);

		if (error == NULL) {
			return NSERROR_OK;
		}

		NSLOG(netsurf, INFO, "xos_cli: 0x%x: %s", error->errnum,
		      error->errmess);
		ro_warn_user("Failed to launch external hotlist: %s",
			     error->errmess);
	}

	res = ro_hotlist_init();
	if (res == NSERROR_OK) {
		NSLOG(netsurf, INFO, "Presenting");
		ro_gui_dialog_open_top(hotlist_window->core.wh,
				       hotlist_window->core.toolbar,
				       600, 800);
	} else {
		NSLOG(netsurf, INFO, "Failed presenting code %d", res);
	}

	return res;
}


/* exported interface documented in riscos/hotlist.h */
void ro_gui_hotlist_initialise(void)
{
	dialog_hotlist_template = ro_gui_dialog_load_template("tree");
}


/* exported interface documented in riscos/hotlist.h */
nserror ro_gui_hotlist_finalise(void)
{
	nserror res;

	if (hotlist_window == NULL) {
		return NSERROR_OK;
	}

	res = hotlist_fini();
	if (res == NSERROR_OK) {
		res = ro_corewindow_fini(&hotlist_window->core);

		free(hotlist_window);
		hotlist_window = NULL;
	}

	return res;
}


/* exported interface documented in riscos/hotlist.h */
bool ro_gui_hotlist_check_window(wimp_w wh)
{
	if ((hotlist_window != NULL) &&
	    (hotlist_window->core.wh == wh)) {
		return true;
	}
	return false;
}


/* exported interface documented in riscos/hotlist.h */
bool ro_gui_hotlist_check_menu(wimp_menu *menu)
{
	if ((hotlist_window != NULL) &&
	    (hotlist_window->menu == menu)) {
		return true;
	}
	return false;
}


/**
 * Callback to schedule for the next available Null poll, by which point
 * a hotlist client will have claimed the Message_HotlistAddURL and any
 * details in RMA can safely be discarded.
 *
 * \param p Unused data pointer.
 */
static void ro_gui_hotlist_scheduled_callback(void *p)
{
	ro_gui_hotlist_add_cleanup();
}


/**
 * Handle bounced Message_HotlistAddURL, so that RMA storage can be freed.
 *
 * \param message The bounced message content.
 */
static void ro_gui_hotlist_addurl_bounce(wimp_message *message)
{
	if (hotlist_url != NULL) {
		nsurl *nsurl;

		if (nsurl_create(hotlist_url, &nsurl) != NSERROR_OK)
			return;

		hotlist_add_url(nsurl);
		nsurl_unref(nsurl);
	}

	ro_gui_hotlist_add_cleanup();

	/* There's no longer any need to listen for the next Null poll. */

	riscos_schedule(-1, ro_gui_hotlist_scheduled_callback, NULL);
}


/* exported interface documented in riscos/hotlist.h */
void ro_gui_hotlist_add_page(nsurl *url)
{
	const struct url_data *data;
	wimp_message message;
	struct ro_hotlist_message_hotlist_addurl *add_url =
		(struct ro_hotlist_message_hotlist_addurl *) &message;

	if (url == NULL) {
		return;
	}

	/* If we're not using external hotlists, add the page to NetSurf's
	 * own hotlist and return...
	 */
	if (!nsoption_bool(external_hotlists)) {
		hotlist_add_url(url);
		return;
	}

	/* ...otherwise try broadcasting the details to any other
	 * interested parties.  If no-one answers, we'll fall back to
	 * NetSurf's hotlist anyway when the message bounces.
	 */

	ro_gui_hotlist_add_cleanup();

	data = urldb_get_url_data(url);
	if (data == NULL)
		return;

	hotlist_url = osmodule_alloc(nsurl_length(url) + 1);
	hotlist_title = osmodule_alloc(strlen(data->title) + 1);

	if (hotlist_url == NULL || hotlist_title == NULL) {
		ro_gui_hotlist_add_cleanup();
		return;
	}

	strcpy(hotlist_url, nsurl_access(url));
	strcpy(hotlist_title, data->title);

	add_url->size = 60;
	add_url->your_ref = 0;
	add_url->action = message_HOTLIST_ADD_URL;
	add_url->url = hotlist_url;
	add_url->title = hotlist_title;
	strcpy(add_url->appname, "NetSurf");

	if (!ro_message_send_message(wimp_USER_MESSAGE_RECORDED,
				     &message,
				     0,
				     ro_gui_hotlist_addurl_bounce)) {
		ro_gui_hotlist_add_cleanup();
	}

	/* Listen for the next Null poll, as an indication that the
	 * message didn't bounce.
	 */
	riscos_schedule(0, ro_gui_hotlist_scheduled_callback, NULL);
}


/* exported interface documented in riscos/hotlist.h */
void ro_gui_hotlist_add_cleanup(void)
{
	if (hotlist_url != NULL) {
		osmodule_free(hotlist_url);
		hotlist_url = NULL;
	}

	if (hotlist_title != NULL) {
		osmodule_free(hotlist_title);
		hotlist_title = NULL;
	}
}


/**
 * Callback confirming a URL delete query.
 *
 * \param id The ID of the query calling us.
 * \param res The user's response to the query.
 * \param p Callback data (always NULL).
 */
static void
ro_gui_hotlist_remove_confirmed(query_id id, enum query_response res, void *p)
{
	hotlist_remove_url(hotlist_delete_url);
	ro_toolbar_update_all_hotlists();

	nsurl_unref(hotlist_delete_url);
	hotlist_delete_url = NULL;
	hotlist_query = QUERY_INVALID;
}


/**
 * Callback cancelling a URL delete query.
 *
 * \param id The ID of the query calling us.
 * \param res The user's response to the query.
 * \param p Callback data (always NULL).
 */
static void
ro_gui_hotlist_remove_cancelled(query_id id, enum query_response res, void *p)
{
	nsurl_unref(hotlist_delete_url);
	hotlist_delete_url = NULL;
	hotlist_query = QUERY_INVALID;
}


/**
 * removal query dialog callbacks.
 */
static const query_callback remove_funcs = {
	ro_gui_hotlist_remove_confirmed,
	ro_gui_hotlist_remove_cancelled
};


/* exported interface documented in riscos/hotlist.h */
void ro_gui_hotlist_remove_page(nsurl *url)
{
	if ((url == NULL) ||
	    nsoption_bool(external_hotlists) ||
	    !hotlist_has_url(url)) {
		return;
	}

	/* Clean up any existing delete attempts before continuing. */
	if (hotlist_query != QUERY_INVALID) {
		query_close(hotlist_query);
		hotlist_query = QUERY_INVALID;
	}

	if (hotlist_delete_url != NULL) {
		nsurl_unref(hotlist_delete_url);
		hotlist_delete_url = NULL;
	}

	/* Check with the user before removing the URL, unless they don't
	 * want us to be careful in which case just do it.
	 */
	if (nsoption_bool(confirm_hotlist_remove)) {
		hotlist_query = query_user("RemoveHotlist", NULL,
					   &remove_funcs, NULL,
					   messages_get("Remove"),
					   messages_get("DontRemove"));

		hotlist_delete_url = nsurl_ref(url);
	} else {
		hotlist_remove_url(url);
		ro_toolbar_update_all_hotlists();
	}
}


/* exported interface documented in riscos/hotlist.h */
bool ro_gui_hotlist_has_page(nsurl *url)
{
	if ((url == NULL) ||
	    nsoption_bool(external_hotlists)) {
		return false;
	}

	return hotlist_has_url(url);
}
