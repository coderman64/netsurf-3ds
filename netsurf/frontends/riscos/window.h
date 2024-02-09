/*
 * Copyright 2010, 2011 Stephen Fryatt <stevef@netsurf-browser.org>
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

/** \file
 * Browser window handling (interface).
 */

#include <stdbool.h>

#ifndef _NETSURF_RISCOS_WINDOW_H_
#define _NETSURF_RISCOS_WINDOW_H_

struct gui_window;
struct nsurl;

extern struct gui_window_table *riscos_window_table;

/**
 * Initialise the browser window module and its menus.
 */
void ro_gui_window_initialise(void);


/**
 * Check if a particular menu handle is a browser window menu
 *
 * \param menu The menu in question.
 * \return true if this menu is a browser window menu
 */
bool ro_gui_window_check_menu(wimp_menu *menu);


/**
 * Set the contents of a window's address bar.
 *
 * \param g gui_window to update
 * \param url new url for address bar
 */
nserror ro_gui_window_set_url(struct gui_window *g, struct nsurl *url);


/**
 * Cause an area of a window to be invalidated
 *
 * The specified area of the window should now be considered out of
 *  date. If the entire window is invalidated this simply calls
 *  wimp_force_redraw() otherwise the area is added to a queue of
 *  pending updates which will be processed from a wimp poll allowing
 *  multiple invalidation requests to be agregated.
 *
 * \param g The window to update
 * \param rect The area of the window to update or NULL to redraw entire contents.
 */
nserror ro_gui_window_invalidate_area(struct gui_window *g, const struct rect *rect);


/**
 * Set a gui_window's scale
 */
void ro_gui_window_set_scale(struct gui_window *g, float scale);


/**
 * Handle Message_DataLoad (file dragged in) for a window.
 *
 * If the file was dragged into a form file input, it is used as the value.
 *
 * \param g window
 * \param message Message_DataLoad block
 * \return true if the load was processed
 */
bool ro_gui_window_dataload(struct gui_window *g, wimp_message *message);


/**
 * Handle pointer movements in a browser window.
 *
 * \param pointer new mouse position
 * \param data browser window that the pointer is in
 */
void ro_gui_window_mouse_at(wimp_pointer *pointer, void *data);


/**
 * Window is being iconised.
 *
 * Create a suitable thumbnail sprite (which, sadly, must be in the
 * Wimp sprite pool), and return the sprite name and truncated title
 * to the iconiser
 *
 * \param g the gui window being iconised
 * \param wi the WindowInfo message from the iconiser
 */
void ro_gui_window_iconise(struct gui_window *g, wimp_full_message_window_info *wi);


/**
 * Handle Message_DataLoad (file dragged in) for a toolbar
 *
 * @todo This belongs in the toolbar module, and should be moved there
 * once the module is able to usefully handle its own events.
 *
 * \param g window
 * \param message  Message_DataLoad block
 * \return true if the load was processed
 */
bool ro_gui_toolbar_dataload(struct gui_window *g, wimp_message *message);


/**
 * Redraws the content for all windows.
 */
void ro_gui_window_redraw_all(void);


/**
 * Redraw any pending update boxes.
 */
void ro_gui_window_update_boxes(void);


/**
 * Destroy all browser windows.
 */
void ro_gui_window_quit(void);


/**
 * Close all browser windows
 *
 * no need for a separate fn same operation as quit
*/
#define ro_gui_window_close_all ro_gui_window_quit


/**
 * Animate the "throbbers" of all browser windows.
 */
void ro_gui_throb(void);

/**
 * Makes a browser window's options the default.
 *
 * \param gui The riscos gui window to set default options in.
 */
void ro_gui_window_default_options(struct gui_window *gui);


/**
 * Convert a RISC OS window handle to a gui_window.
 *
 * \param window RISC OS window handle.
 * \return A pointer to a riscos gui window if found or NULL.
 */
struct gui_window *ro_gui_window_lookup(wimp_w window);


/**
 * Convert a toolbar RISC OS window handle to a gui_window.
 *
 * \param  window RISC OS window handle of a toolbar
 * \return pointer to a structure if found, NULL otherwise
 */
struct gui_window *ro_gui_toolbar_lookup(wimp_w window);


/**
 * Convert x,y screen co-ordinates into window co-ordinates.
 *
 * \param g gui window
 * \param x x ordinate
 * \param y y ordinate
 * \param pos receives position in window co-ordinatates
 * \return true iff conversion successful
 */
bool ro_gui_window_to_window_pos(struct gui_window *g, int x, int y, os_coord *pos);


/**
 * Returns the state of the mouse buttons and modifiers keys for a
 * mouse action, suitable for passing to the OS-independent
 * browser window/ treeview/ etc code.
 *
 * \param  buttons		Wimp button state.
 * \param  type			Wimp work-area/icon type for decoding.
 * \return			NetSurf core button state.
 */
enum browser_mouse_state ro_gui_mouse_click_state(wimp_mouse_state buttons, wimp_icon_flags type);


/**
 * Returns the state of the mouse buttons and modifiers keys whilst
 * dragging, for passing to the OS-independent browser window/ treeview/
 * etc code
 *
 * \param buttons Wimp button state.
 * \param type Wimp work-area/icon type for decoding.
 * \return NetSurf core button state.
 */
enum browser_mouse_state ro_gui_mouse_drag_state(wimp_mouse_state buttons, wimp_icon_flags type);


/**
 * Returns true iff one or more Shift keys is held down
 */
bool ro_gui_shift_pressed(void);


/**
 * Returns true iff one or more Ctrl keys is held down
 */
bool ro_gui_ctrl_pressed(void);


/**
 * Returns true iff one or more Alt keys is held down
 */
bool ro_gui_alt_pressed(void);


/**
 * Change mouse pointer shape
 */
void gui_window_set_pointer(struct gui_window *g, enum gui_pointer_shape shape);

#endif

