/*
 * Copyright 2005 Richard Wilson <info@tinct.net>
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

#include <stdbool.h>
#include <oslib/hourglass.h>

#include "utils/nsoption.h"
#include "utils/filename.h"
#include "utils/messages.h"

#include "riscos/gui.h"
#include "riscos/wimp.h"
#include "riscos/wimp_event.h"
#include "riscos/configure.h"
#include "riscos/configure/configure.h"
#include "riscos/dialog.h"


#define CACHE_MEMORY_SIZE 3
#define CACHE_MEMORY_DEC 4
#define CACHE_MEMORY_INC 5
#define CACHE_DISC_SIZE 10
#define CACHE_DISC_DEC 11
#define CACHE_DISC_INC 12
#define CACHE_DISC_EXPIRE 15
#define CACHE_DISC_EXPIRE_DEC 16
#define CACHE_DISC_EXPIRE_INC 17
#define CACHE_DEFAULT_BUTTON 19
#define CACHE_CANCEL_BUTTON 20
#define CACHE_OK_BUTTON 21

static bool ro_gui_options_cache_click(wimp_pointer *pointer);
static bool ro_gui_options_cache_ok(wimp_w w);

bool ro_gui_options_cache_initialise(wimp_w w)
{
	/* set the current values */
	ro_gui_set_icon_decimal(w, CACHE_MEMORY_SIZE,
			(nsoption_int(memory_cache_size) * 10) >> 20, 1);
	ro_gui_set_icon_decimal(w, CACHE_DISC_SIZE,
			(int) ((nsoption_uint(disc_cache_size)) >> 20), 0);
	ro_gui_set_icon_decimal(w, CACHE_DISC_EXPIRE,
			(nsoption_int(disc_cache_age)), 0);

	/* initialise all functions for a newly created window */
	ro_gui_wimp_event_register_numeric_field(w, CACHE_MEMORY_SIZE,
			CACHE_MEMORY_INC, CACHE_MEMORY_DEC, 0, 640, 1, 1);
	ro_gui_wimp_event_register_numeric_field(w, CACHE_DISC_SIZE,
			CACHE_DISC_INC, CACHE_DISC_DEC, 0, 4095, 1, 0);
	ro_gui_wimp_event_register_numeric_field(w, CACHE_DISC_EXPIRE,
			CACHE_DISC_EXPIRE_INC, CACHE_DISC_EXPIRE_DEC, 1, 3650,
			1, 0);
	ro_gui_wimp_event_register_mouse_click(w, ro_gui_options_cache_click);
	ro_gui_wimp_event_register_cancel(w, CACHE_CANCEL_BUTTON);
	ro_gui_wimp_event_register_ok(w, CACHE_OK_BUTTON,
			ro_gui_options_cache_ok);
	ro_gui_wimp_event_set_help_prefix(w, "HelpCacheConfig");
	ro_gui_wimp_event_memorise(w);
	return true;

}

bool ro_gui_options_cache_click(wimp_pointer *pointer)
{
	switch (pointer->i) {
		case CACHE_DEFAULT_BUTTON:
			/* set the default values */
			ro_gui_set_icon_decimal(pointer->w, CACHE_MEMORY_SIZE,
					120, 1);
			ro_gui_set_icon_decimal(pointer->w, CACHE_DISC_SIZE,
					1024, 0);
			ro_gui_set_icon_decimal(pointer->w, CACHE_DISC_EXPIRE,
					28, 0);
			return true;
	}
	return false;
}

bool ro_gui_options_cache_ok(wimp_w w)
{
	nsoption_set_int(memory_cache_size,
			(((ro_gui_get_icon_decimal(w,
					CACHE_MEMORY_SIZE, 1) + 1) << 20) - 1) / 10);
	nsoption_set_uint(disc_cache_size,
			(uint) (ro_gui_get_icon_decimal(w,
					CACHE_DISC_SIZE, 0) << 20));
	nsoption_set_int(disc_cache_age,
			ro_gui_get_icon_decimal(w, CACHE_DISC_EXPIRE, 0));

	ro_gui_save_options();
  	return true;
}
