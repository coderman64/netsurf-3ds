/*
 * Copyright 2008-2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_GUI_MENU_H
#define AMIGA_GUI_MENU_H

/** Maximum number of hotlist items (somewhat arbitrary value) */
#define AMI_HOTLIST_ITEMS 200

/** Maximum number of ARexx menu items (somewhat arbitrary value) */
#define AMI_MENU_AREXX_ITEMS 20

/** enum menu structure, has to be here as we need it below. */
enum {
	/* Project menu */
	M_PROJECT = 0,
	 M_NEWWIN,
	 M_NEWTAB,
	 M_BAR_P1,
	 M_OPEN,
	 M_SAVEAS,
	  M_SAVESRC,
	  M_SAVETXT,
	  M_SAVECOMP,
	  M_SAVEIFF,
#ifdef WITH_PDF_EXPORT
	  M_SAVEPDF,
#endif
	 M_BAR_P2,
 	 M_PRINT,
	 M_BAR_P3,
	 M_CLOSETAB,
	 M_CLOSEWIN,
	 M_BAR_P4,
	 M_ABOUT,
	 M_BAR_P5,
	 M_QUIT,
	/* Edit menu */
	M_EDIT,
	 M_CUT,
	 M_COPY,
	 M_PASTE,
	 M_BAR_E1,
	 M_SELALL,
	 M_CLEAR,
	 M_BAR_E2,
	 M_UNDO,
	 M_REDO,
	/* Browser menu */
	M_BROWSER,
	 M_FIND,
	 M_BAR_B1,
	 M_HISTLOCL,
	 M_HISTGLBL,
	 M_BAR_B2,
	 M_COOKIES,
	 M_BAR_B3,
	 M_SCALE,
	  M_SCALEDEC,
	  M_SCALENRM,
	  M_SCALEINC,
	 M_IMAGES,
	  M_IMGFORE,
	  M_IMGBACK,
	 M_JS,
	 M_BAR_B4,
	 M_REDRAW,
	/* Hotlist menu */
	M_HOTLIST,
	 M_HLADD,
	 M_HLSHOW,
	 M_BAR_H1, // 47
	 AMI_MENU_HOTLIST, /* Where the hotlist entries start */
	 AMI_MENU_HOTLIST_MAX = AMI_MENU_HOTLIST + AMI_HOTLIST_ITEMS,
	/* Settings menu */
	M_PREFS,
	 M_PREDIT,
	 M_BAR_S1,
	 M_SNAPSHOT,
	 M_PRSAVE,
	/* ARexx menu */
	M_AREXX,
	 M_AREXXEX,
	 M_BAR_A1,
	 AMI_MENU_AREXX,
	 AMI_MENU_AREXX_MAX = AMI_MENU_AREXX + AMI_MENU_AREXX_ITEMS
};

/* We can get away with AMI_MENU_MAX falling short as it is
 * only used for freeing the UTF-8 converted menu labels */
#define AMI_MENU_MAX AMI_MENU_AREXX

struct gui_window;
struct gui_window_2;
struct hlcache_handle;
struct Window;
struct ami_menu_data;

ULONG ami_gui_menu_number(int item);
struct Menu *ami_gui_menu_create(struct gui_window_2 *gwin);
void ami_gui_menu_free(struct gui_window_2 *gwin);

void ami_gui_menu_update_checked(struct gui_window_2 *gwin);
void ami_gui_menu_update_disabled(struct gui_window *g, struct hlcache_handle *c);

/**
 * Sets that an item linked to a toggle menu item has been changed.
 */
void ami_gui_menu_set_check_toggled(void);

/**
 * Gets if the menu needs updating because an item linked
 * to a toggle menu item has been changed.
 * NB: This also *clears* the state
 *
 * \return true if the menus need refreshing
 */
bool ami_gui_menu_get_check_toggled(void);

/**
 * Set checked state of a menu item
 * almost generic, but not quite
 */
void ami_gui_menu_set_checked(struct Menu *menu, int item, bool check);

/**
 * Set disabled state of a menu item
 * almost generic, but not quite
 */
void ami_gui_menu_set_disabled(struct Window *win, struct Menu *menu, int item, bool disable);

/**
 * Refresh the Hotlist menu
 */
void ami_gui_menu_refresh_hotlist(void);

/**
 * Gets if NetSurf has been quit from the menu
 *
 * \return true if NetSurf has been quit
 */
bool ami_gui_menu_quit_selected(void);

/**
 * Frees a menu.  Only used on OS3 and old OS4.
 */
void ami_gui_menu_freemenus(struct Menu *menu, struct ami_menu_data **md);
#endif

