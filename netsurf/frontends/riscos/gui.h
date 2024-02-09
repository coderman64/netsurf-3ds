/*
 * Copyright 2003 Phil Mellor <monkeyson@users.sourceforge.net>
 * Copyright 2004 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2004 Andrew Timmins <atimmins@blueyonder.co.uk>
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

#ifndef _NETSURF_RISCOS_GUI_H_
#define _NETSURF_RISCOS_GUI_H_

#include <oslib/wimp.h>

#define RISCOS5 0xAA

#define THUMBNAIL_WIDTH 100
#define THUMBNAIL_HEIGHT 86

/* The maximum size for user-editable URLs in the RISC OS GUI. */

#define RO_GUI_MAX_URL_SIZE 2048

extern int os_version;

extern bool os_alpha_sprite_supported;

extern const char * NETSURF_DIR;

struct toolbar;
struct status_bar;
struct plotter_table;
struct gui_window;
struct tree;
struct node;
struct history;
struct css_style;
struct nsurl;
struct hlcache_handle;

enum gui_pointer_shape;

extern wimp_t task_handle;	/**< RISC OS wimp task handle. */

extern wimp_w dialog_info, dialog_saveas, dialog_zoom, dialog_pageinfo,
	dialog_objinfo, dialog_tooltip, dialog_warning, dialog_openurl,
	dialog_folder, dialog_entry, dialog_url_complete,
	dialog_search, dialog_print, dialog_theme_install;
extern wimp_w current_menu_window;
extern bool current_menu_open;
extern wimp_menu *recent_search_menu;	/* search.c */
extern bool gui_redraw_debug;
extern osspriteop_area *gui_sprites;
extern bool dialog_folder_add, dialog_entry_add, hotlist_insert;
extern bool print_active, print_text_black;
extern bool no_font_blending;

typedef enum { GUI_DRAG_NONE, GUI_DRAG_DOWNLOAD_SAVE, GUI_DRAG_SAVE }
		ro_gui_drag_type;

extern ro_gui_drag_type gui_current_drag_type;

extern bool riscos_done;

/** RISC OS data for a browser window. */
struct gui_window {
	/** Associated platform-independent browser window data. */
	struct browser_window *bw;

	struct toolbar *toolbar;	/**< Toolbar, or 0 if not present. */
	struct status_bar *status_bar;	/**< Status bar, or 0 if not present. */

	wimp_w window;		/**< RISC OS window handle. */

	int old_width;		/**< Width when last opened / os units. */
	int old_height;		/**< Height when last opened / os units. */
	bool update_extent;	/**< Update the extent on next opening */
	bool active;		/**< Whether the throbber should be active */

	char title[256];	/**< Buffer for window title. */

	int iconise_icon;	/**< ID number of icon when window is iconised */

	char validation[12];	/**< Validation string for colours */

	/** Options. */
	struct {
		bool buffer_animations;	/**< Use screen buffering for animations. */
		bool buffer_everything;	/**< Use screen buffering for everything. */
	} option;

	struct gui_window *prev;	/**< Previous in linked list. */
	struct gui_window *next;	/**< Next in linked list. */
};


extern struct gui_window *ro_gui_current_redraw_gui;


/* in gui.c */
void ro_gui_open_window_request(wimp_open *open);
void ro_gui_screen_size(int *width, int *height);
void ro_gui_view_source(struct hlcache_handle *c);
void ro_gui_dump_browser_window(struct browser_window *bw);
void ro_gui_drag_box_start(wimp_pointer *pointer);
bool ro_gui_prequit(void);
const char *ro_gui_default_language(void);
nserror ro_warn_user(const char *warning, const char *detail);

/**
 * Cause an abnormal program termination.
 *
 * \note This never returns and is intended to terminate without any cleanup.
 *
 * \param error The message to display to the user.
 */
void die(const char * const error) __attribute__ ((noreturn));

/* in download.c */
void ro_gui_download_init(void);
void ro_gui_download_datasave_ack(wimp_message *message);
bool ro_gui_download_prequit(void);
extern struct gui_download_table *riscos_download_table;

/* in schedule.c */
extern bool sched_active;
extern os_t sched_time;

/**
 * Process events up to current time.
 */
bool schedule_run(void);

/**
 * Schedule a callback.
 *
 * \param  t         interval before the callback should be made in ms
 * \param  callback  callback function
 * \param  p         user parameter, passed to callback function
 *
 * The callback function will be called as soon as possible after t ms have
 * passed.
 */
nserror riscos_schedule(int t, void (*callback)(void *p), void *p);

/* in search.c */
void ro_gui_search_init(void);
void ro_gui_search_prepare(struct browser_window *g);
extern struct gui_search_table *riscos_search_table;

/* in print.c */
void ro_gui_print_init(void);
void ro_gui_print_prepare(struct gui_window *g);

/* in plotters.c */
extern const struct plotter_table ro_plotters;
extern int ro_plot_origin_x;
extern int ro_plot_origin_y;
extern struct rect ro_plot_clip_rect;

/* in theme_install.c */
bool ro_gui_theme_install_apply(wimp_w w);

/* icon numbers */
#define ICON_STATUS_RESIZE 0
#define ICON_STATUS_TEXT 1

#define ICON_SAVE_ICON 0
#define ICON_SAVE_PATH 1
#define ICON_SAVE_OK 2
#define ICON_SAVE_CANCEL 3

#define ICON_PAGEINFO_TITLE 0
#define ICON_PAGEINFO_URL 1
#define ICON_PAGEINFO_ENC 2
#define ICON_PAGEINFO_TYPE 3
#define ICON_PAGEINFO_ICON 4

#define ICON_OBJINFO_URL 0
#define ICON_OBJINFO_TARGET 1
#define ICON_OBJINFO_TYPE 2
#define ICON_OBJINFO_ICON 3

#define ICON_WARNING_MESSAGE 0
#define ICON_WARNING_CONTINUE 1
#define ICON_WARNING_HELP 2

#define ICON_SEARCH_TEXT 0
#define ICON_SEARCH_CASE_SENSITIVE 1
#define ICON_SEARCH_FIND_NEXT 2
#define ICON_SEARCH_FIND_PREV 3
#define ICON_SEARCH_CANCEL 4
#define ICON_SEARCH_STATUS 5
#define ICON_SEARCH_MENU 8
#define ICON_SEARCH_SHOW_ALL 9

#define ICON_THEME_INSTALL_MESSAGE 0
#define ICON_THEME_INSTALL_INSTALL 1
#define ICON_THEME_INSTALL_CANCEL 2

#define ICON_OPENURL_URL 1
#define ICON_OPENURL_CANCEL 2
#define ICON_OPENURL_OPEN 3
#define ICON_OPENURL_MENU 4

#define ICON_ENTRY_NAME 1
#define ICON_ENTRY_URL 3
#define ICON_ENTRY_CANCEL 4
#define ICON_ENTRY_OK 5
#define ICON_ENTRY_RECENT 6

#define ICON_FOLDER_NAME 1
#define ICON_FOLDER_CANCEL 2
#define ICON_FOLDER_OK 3

#endif
