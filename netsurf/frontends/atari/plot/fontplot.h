/*
 * Copyright 2010 Ole Loots <ole@monochrom.net>
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

#ifndef FONT_PLOT_H
#define FONT_PLOT_H

typedef struct s_font_plotter * FONT_PLOTTER;

struct s_font_driver_table_entry
{
	const char * name;
	int (*ctor)( FONT_PLOTTER self );
	int flags;
};

/* declaration of font plotter member functions: (_fpmf_ prefix) */
typedef int (*_fpmf_str_width)( FONT_PLOTTER self, const plot_font_style_t *fstyle,
						const char * str, size_t length, int * width);
typedef int (*_fpmf_str_split)( FONT_PLOTTER self, const plot_font_style_t *fstyle,
						const char *string, size_t length,
						int x, size_t *char_offset, int *actual_x);
typedef int (*_fpmf_pixel_pos)( FONT_PLOTTER self, const plot_font_style_t *fstyle,
						const char *string, size_t length,
						int x, size_t *char_offset, int *actual_x);
typedef int (*_fpmf_text)( FONT_PLOTTER self, int x, int y, const char *text,
													size_t length, const plot_font_style_t *fstyle);

typedef void (*_fpmf_draw_glyph)(FONT_PLOTTER self, GRECT * clip, GRECT * loc,
								uint8_t * pixdata, int pitch, uint32_t colour);
typedef int (*_fpmf_dtor)( FONT_PLOTTER self );


/* prototype of the font plotter "object" */
struct s_font_plotter
{
	char * name;
	int flags;
	int vdi_handle;
	void * priv_data;

	_fpmf_str_width str_width;
	_fpmf_str_split str_split;
	_fpmf_pixel_pos pixel_pos;
	_fpmf_text text;
	_fpmf_draw_glyph draw_glyph;
	_fpmf_dtor dtor;
};


FONT_PLOTTER plot_get_text_plotter(void);
/* Set the font plotting engine. 
*/
void plot_set_text_plotter(FONT_PLOTTER font_plotter);
void dump_font_drivers(void);
FONT_PLOTTER new_font_plotter( int vdihandle, char * name, unsigned long flags,
		int * error);
int delete_font_plotter( FONT_PLOTTER p );

#ifdef WITH_VDI_FONT_DRIVER
 #include "atari/plot/font_vdi.h"
#endif
#ifdef WITH_INTERNAL_FONT_DRIVER
 #include "atari/plot/font_internal.h"
#endif
#ifdef WITH_FREETYPE_FONT_DRIVER
 #include "atari/plot/font_freetype.h"
#endif



#endif
