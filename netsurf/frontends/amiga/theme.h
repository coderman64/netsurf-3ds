/*
 * Copyright 2010 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_THEME_H
#define AMIGA_THEME_H

#include "netsurf/mouse.h"

struct gui_window_2;
struct gui_window;

#define AMI_GUI_POINTER_BLANK GUI_POINTER_PROGRESS+1
#define AMI_GUI_POINTER_DRAG  GUI_POINTER_PROGRESS+2
#define AMI_LASTPOINTER AMI_GUI_POINTER_DRAG

void ami_theme_init(void);
void ami_get_theme_filename(char *filename, const char *themestring, bool protocol);

int ami_theme_throbber_get_width(void);
int ami_theme_throbber_get_height(void);

void ami_theme_throbber_setup(void);
void ami_theme_throbber_free(void);
void ami_update_throbber(struct gui_window_2 *g,bool redraw);

void ami_init_mouse_pointers(void);
void ami_mouse_pointers_free(void);
/* Use the following ONLY if nothing other than the Intuition window pointer is available,
 * and ALWAYS in preference to SetWindowPointer(), as it features more pointers and uses
 * the correct ones specified in user preferences. */
void ami_update_pointer(struct Window *win, gui_pointer_shape shape); 

void gui_window_start_throbber(struct gui_window *g);
void gui_window_stop_throbber(struct gui_window *g);
void ami_throbber_redraw_schedule(int t, struct gui_window *g);

void gui_window_set_pointer(struct gui_window *g, gui_pointer_shape shape);
#endif

