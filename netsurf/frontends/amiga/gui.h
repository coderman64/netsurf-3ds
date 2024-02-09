/*
 * Copyright 2008-2019 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_GUI_H
#define AMIGA_GUI_H

#include <stdbool.h>
#include <graphics/rastport.h>
#include <intuition/classusr.h>
#include <dos/dos.h>
#include <devices/inputevent.h>

#include "netsurf/window.h"
#include "netsurf/mouse.h"

#include "amiga/gui_menu.h"
#include "amiga/object.h"
#include "amiga/os3support.h"

#ifdef __amigaos4__
#define HOOKF(ret,func,type,ptr,msgtype) static ret func(struct Hook *hook, type ptr, msgtype msg)
#else
#define HOOKF(ret,func,type,ptr,msgtype) static ASM ret func(REG(a0, struct Hook *hook),REG(a2, type ptr), REG(a1, msgtype msg))
#endif

/* valid options for ami_gui_get_object */
enum {
	AMI_GAD_THROBBER = 0,
	AMI_GAD_TABS,
	AMI_GAD_URL,
	AMI_GAD_SEARCH,
	AMI_WIN_MAIN
};

struct find_window;
struct ami_history_local_window;
struct ami_menu_data;
struct gui_window;
struct gui_window_2;
struct IBox;

#define AMI_GUI_TOOLBAR_MAX 20

struct ami_win_event_table {
	/* callback to handle events when using a shared msgport
	 *
	 * @param pointer to our window structure (must start with ami_generic_window)
	 * @return TRUE if window was destroyed during event processing
	 */
	BOOL (*event)(void *w);

	/* callback for explicit window closure
	 * some windows are implicitly closed by the browser and should set this to NULL
	*/
	void (*close)(void *w);
};

struct ami_generic_window {
	struct nsObject *node;
	const struct ami_win_event_table *tbl;
};

#define IS_CURRENT_GW(GWIN,GW) (ami_gui2_get_gui_window(GWIN) == GW)

/* The return value for these functions must be deallocated using FreeVec() */
STRPTR ami_locale_langs(int *codeset);
char *ami_gui_get_cache_favicon_name(struct nsurl *url, bool only_if_avail);

/* Functions lacking documentation */
void ami_get_msg(void);
void ami_try_quit(void);
void ami_quit_netsurf(void);
void ami_schedule_redraw(struct gui_window_2 *gwin, bool full_redraw);
int ami_key_to_nskey(ULONG keycode, struct InputEvent *ie);
bool ami_text_box_at_point(struct gui_window_2 *gwin, ULONG *restrict x, ULONG *restrict y);
bool ami_mouse_to_ns_coords(struct gui_window_2 *gwin, int *restrict x, int *restrict y,
	int mouse_x, int mouse_y);
BOOL ami_gadget_hit(Object *obj, int x, int y);
void ami_gui_history(struct gui_window_2 *gwin, bool back);
void ami_gui_hotlist_update_all(void);
void ami_gui_tabs_toggle_all(void);
bool ami_locate_resource(char *fullpath, const char *file);
void ami_gui_update_hotlist_button(struct gui_window_2 *gwin);
nserror ami_gui_new_blank_tab(struct gui_window_2 *gwin);
int ami_gui_count_windows(int window, int *tabs);
void ami_gui_set_scale(struct gui_window *gw, float scale);
void ami_set_pointer(struct gui_window_2 *gwin, gui_pointer_shape shape, bool update);
void ami_reset_pointer(struct gui_window_2 *gwin);
void *ami_window_at_pointer(int type);

/**
 * Beep
 */
void ami_gui_beep(void);

/**
 * Close a window and all tabs attached to it.
 *
 * @param w gui_window_2 to act upon.
 */
void ami_gui_close_window(void *w);

/**
 * Close all tabs in a window except the active one.
 *
 * @param gwin gui_window_2 to act upon.
 */
void ami_gui_close_inactive_tabs(struct gui_window_2 *gwin);

/**
 * Compatibility function to get space.gadget render area.
 *
 * @param obj A space.gadget object.
 * @param bbox A pointer to a struct IBox *.
 * @return error status.
 */
nserror ami_gui_get_space_box(Object *obj, struct IBox **bbox);

/**
 * Free any data obtained via ami_gui_get_space_box().
 *
 * @param bbox A pointer to a struct IBox.
 */
void ami_gui_free_space_box(struct IBox *bbox);

/**
 * Get shared message port
 *
 * @return Pointer to an initialised MsgPort
 */
struct MsgPort *ami_gui_get_shared_msgport(void);

/**
 * Get the application.library ID NetSurf is registered as.
 *
 * @return App ID.
 */
uint32 ami_gui_get_app_id(void);

/**
 * Get a pointer to the screen NetSurf is running on.
 *
 * @return Pointer to struct Screen.
 */
struct Screen *ami_gui_get_screen(void);

/**
 * Get the string for NetSurf's screen titlebar.
 *
 * @return String to use as the screen's titlebar text.
 */
STRPTR ami_gui_get_screen_title(void);

/**
 * Switch to the most-recently-opened tab
 */
void ami_gui_switch_to_new_tab(struct gui_window_2 *gwin);

/**
 * Add a window to the NetSurf window list (to enable event processing)
 */
nserror ami_gui_win_list_add(void *win, int type, const struct ami_win_event_table *table);

/**
 * Remove a window from the NetSurf window list
 */
void ami_gui_win_list_remove(void *win);

/**
 * Get the window list.
 *
 *\TODO: Nothing should be poking around in this list, but we aren't
 *       assigning unique IDs to windows (ARexx interface needs this)
 *       ami_find_gwin_by_id() is close but not ARexx-friendly
 */
struct MinList *ami_gui_get_window_list(void);

/**
 * Get which qualifier keys are being pressed
 */
int ami_gui_get_quals(Object *win_obj);

/**
 * Check rect is not already queued for redraw
 */
bool ami_gui_window_update_box_deferred_check(struct MinList *deferred_rects,
				const struct rect *restrict new_rect, APTR mempool);

/**
 * Adjust scale by specified amount
 */
void ami_gui_adjust_scale(struct gui_window *gw, float adjustment);

/**
 * Get a pointer to the gui_window which NetSurf considers
 * to be the current/active one
 */
struct gui_window *ami_gui_get_active_gw(void);

/**
 * Get browser window from gui_window
 */
struct browser_window *ami_gui_get_browser_window(struct gui_window *gw);

/**
 * Get browser window from gui_window_2
 */
struct browser_window *ami_gui2_get_browser_window(struct gui_window_2 *gwin);

/**
 * Get gui_window_2 from gui_window
 */
struct gui_window_2 *ami_gui_get_gui_window_2(struct gui_window *gw);

/**
 * Get gui_window from gui_window_2
 */
struct gui_window *ami_gui2_get_gui_window(struct gui_window_2 *gwin);

/**
 * Get download list from gui_window
 */
struct List *ami_gui_get_download_list(struct gui_window *gw);

/**
 * Get tab title from gui_window
 */
const char *ami_gui_get_tab_title(struct gui_window *gw);

/**
 * Get window title from gui_window
 */
const char *ami_gui_get_win_title(struct gui_window *gw);

/**
 * Get tab node from gui_window
 */
struct Node *ami_gui_get_tab_node(struct gui_window *gw);

/**
 * Get tabs from gui_window_2
 */
ULONG ami_gui2_get_tabs(struct gui_window_2 *gwin);

/**
 * Get tab list from gui_window_2
 */
struct List *ami_gui2_get_tab_list(struct gui_window_2 *gwin);

/**
 * Get favicon from gui_window
 */
struct hlcache_handle *ami_gui_get_favicon(struct gui_window *gw);

/**
 * Get local history window from gui_window
 */
struct ami_history_local_window *ami_gui_get_history_window(struct gui_window *gw);

/**
 * Set local history window in gui_window
 */
void ami_gui_set_history_window(struct gui_window *gw, struct ami_history_local_window *hw);

/**
 * Set search window in gui_window
 */
void ami_gui_set_find_window(struct gui_window *gw, struct find_window *fw);

/**
 * Get throbbing status from gui_window
 */
bool ami_gui_get_throbbing(struct gui_window *gw);

/**
 * Get throbbing frame from gui_window
 */
int ami_gui_get_throbber_frame(struct gui_window *gw);

/**
 * Set throbbing frame in gui_window
 */
void ami_gui_set_throbber_frame(struct gui_window *gw, int frame);

/**
 * Set throbbing status in gui_window
 */
void ami_gui_set_throbbing(struct gui_window *gw, bool throbbing);

/**
 * Get object from gui_window
 */
Object *ami_gui2_get_object(struct gui_window_2 *gwin, int object_type);

/**
 * Get window from gui_window
 */
struct Window *ami_gui_get_window(struct gui_window *gw);

/**
 * Get window from gui_window_2
 */
struct Window *ami_gui2_get_window(struct gui_window_2 *gwin);

/**
 * Get imenu from gui_window
 */
struct Menu *ami_gui_get_menu(struct gui_window *gw);

/**
 * Set imenu to gui_window_2. A value of NULL will free the menu (and menu_data!)
 */
void ami_gui2_set_menu(struct gui_window_2 *gwin, struct Menu *menu);

/**
 * Get menu_data from gui_window_2
 */
struct ami_menu_data **ami_gui2_get_menu_data(struct gui_window_2 *gwin);

/**
 * Set ctxmenu history tmp in gui_window_2
 */
void ami_gui2_set_ctxmenu_history_tmp(struct gui_window_2 *gwin, int temp);

/**
 * Get ctxmenu history tmp from gui_window_2
 */
int ami_gui2_get_ctxmenu_history_tmp(struct gui_window_2 *gwin);

/**
 * Get ctxmenu history from gui_window_2
 */
Object *ami_gui2_get_ctxmenu_history(struct gui_window_2 *gwin, ULONG direction);

/**
 * Set ctxmenu history in gui_window_2
 */
void ami_gui2_set_ctxmenu_history(struct gui_window_2 *gwin, ULONG direction, Object *ctx_hist);

/**
 * Set closed in gui_window_2
 */
void ami_gui2_set_closed(struct gui_window_2 *gwin, bool closed);

/**
 * Set new_content in gui_window_2
 * Indicates the window needs redrawing
 */
void ami_gui2_set_new_content(struct gui_window_2 *gwin, bool new_content);

#endif

