/*
 * Copyright 2004, 2005 Richard Wilson <info@tinct.net>
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
#include <stdlib.h>
#include <string.h>
#include <swis.h>
#include <oslib/colourtrans.h>
#include <oslib/os.h>
#include <oslib/osspriteop.h>
#include <oslib/wimp.h>
#include <oslib/wimpreadsysinfo.h>

#include "utils/nsoption.h"
#include "utils/log.h"

#include "riscos/buffer.h"
#include "riscos/gui.h"
#include "riscos/tinct.h"
#include "riscos/wimp.h"
#include "riscos/wimputils.h"

#define BUFFER_EXCLUSIVE_USER_REDRAW "Only support pure user redraw (faster)"
//#define BUFFER_EMULATE_32BPP "Redirect to a 32bpp sprite and plot with Tinct"

/** Absent from OSLib
*/
#ifndef osspriteop_TYPEEXPANSION
#define osspriteop_TYPEEXPANSION ((osspriteop_mode_word) 0xFu)
#endif
#ifndef osspriteop_TYPE16BPP4K
#define osspriteop_TYPE16BPP4K   ((osspriteop_mode_word) 0x10u)
#endif

static void ro_gui_buffer_free(void);


/** The buffer characteristics
*/
static osspriteop_area *buffer = NULL;
static char buffer_name[12] = "scr_buffer";

/** The current clip area
*/
static os_box clipping;

/** The current save area
*/
static osspriteop_save_area *save_area;
static int context0;
static int context1;
static int context2;
static int context3;

/** The current sprite mode
*/
static os_mode mode;


/**
 * Opens a buffer for writing to.
 *
 * The ro_plot_origin_ variables are updated to reflect the new screen origin,
 * so the variables should be set before calling this function, and not
 * changed until after ro_gui_buffer_close() has been called.
 *
 * \param redraw the current WIMP redraw area to buffer
 */
void ro_gui_buffer_open(const wimp_draw *redraw)
{
	int size;
	int total_size;
	os_coord sprite_size;
	int bpp, word_width;
	bool palette;
	os_error *error;
	int palette_size = 0;
#ifdef BUFFER_EXCLUSIVE_USER_REDRAW
	osspriteop_header *header;
#endif

	/*	Close any open buffer
	*/
	if (buffer)
		ro_gui_buffer_close();

	/*	Store our clipping region
	*/
	clipping = redraw->clip;

	/*	Stop bad rectangles
	*/
	if ((clipping.x1 < clipping.x0) ||
			(clipping.y1 < clipping.y0)) {
		NSLOG(netsurf, INFO,
		      "Invalid clipping rectangle (%i, %i) to (%i,%i)",
		      clipping.x0,
		      clipping.y0,
		      clipping.x1,
		      clipping.y1);
		return;
	}

	/*	Work out how much buffer we need
	*/
	sprite_size.x = clipping.x1 - clipping.x0 + 1;
	sprite_size.y = clipping.y1 - clipping.y0 + 1;
	ro_convert_os_units_to_pixels(&sprite_size, (os_mode)-1);
	if (sprite_size.y == 1) /* work around SpriteExtend bug */
		sprite_size.y = 2;

#ifdef BUFFER_EMULATE_32BPP
	bpp = 5;
	palette = false;
#else
	/*	Get the screen depth as we can't use palettes for >8bpp
	*/
	xos_read_mode_variable((os_mode)-1, os_MODEVAR_LOG2_BPP, &bpp, 0);
	palette = (bpp < 4);
#endif

	/*	Get our required buffer size
	*/
	word_width = ((sprite_size.x << bpp) + 31) >> 5;
	if (palette)
		palette_size = ((1 << (1 << bpp)) << 3);
	total_size = sizeof(osspriteop_area) + sizeof(osspriteop_header) +
			(word_width * sprite_size.y * 4) + palette_size;
	buffer = (osspriteop_area *)malloc(total_size);
	if (!buffer) {
		NSLOG(netsurf, INFO, "Failed to allocate memory");
		ro_gui_buffer_free();
		return;
	}
	buffer->size = total_size;
	buffer->first = 16;

#ifdef BUFFER_EMULATE_32BPP
	mode = tinct_SPRITE_MODE;
#else
	if ((error = xwimpreadsysinfo_wimp_mode(&mode)) != NULL) {
		NSLOG(netsurf, INFO, "Error reading mode '%s'",
		      error->errmess);
		ro_gui_buffer_free();
		return;
	}

	/* if we're not in a numbered screen mode then we need
	   to build a suitable sprite mode word */
	if (mode >= (os_mode)0x100) {
		static const ns_os_vdu_var_list vars = {
			os_MODEVAR_LOG2_BPP,
			{
				os_MODEVAR_MODE_FLAGS,
				os_MODEVAR_NCOLOUR,
				os_MODEVAR_XEIG_FACTOR,
				os_MODEVAR_YEIG_FACTOR,
				os_VDUVAR_END_LIST
			}
		};
		struct {
			int log2bpp;
			int flags;
			int ncolour;
			int xeig, yeig;
		} vals;
		int type;

		error = xos_read_vdu_variables(PTR_OS_VDU_VAR_LIST(&vars), (int *)&vals);
		if (error) {
			NSLOG(netsurf, INFO,
			      "Error reading mode properties '%s'",
			      error->errmess);
			ro_gui_buffer_free();
			return;
		}

		switch (vals.ncolour) {
		case 1:
		case 3:
		case 15:
		case 63:
		case 255:
			/* Paletted modes are pixel packing order agnostic */
			type = 1 + vals.log2bpp;
			mode = (os_mode)((type << osspriteop_TYPE_SHIFT) |
					osspriteop_NEW_STYLE |
					((180 >> vals.yeig) << osspriteop_YRES_SHIFT) |
					((180 >> vals.xeig) << osspriteop_XRES_SHIFT));
			break;
		case 4095:
			/* 16bpp 4k colours */
			type = osspriteop_TYPE16BPP4K;
			mode = (os_mode)((osspriteop_TYPEEXPANSION << osspriteop_TYPE_SHIFT) |
					osspriteop_NEW_STYLE |
					(vals.yeig << 6) |
					(vals.xeig << 4) |
					(type << 20) |
					(vals.flags & 0xFF00));
			break;
		case 65535:
			switch ((vals.flags & 0x3000) >> os_MODE_FLAG_DATA_FORMAT_SHIFT) {
			case os_MODE_FLAG_DATA_FORMAT_RGB:
				if (vals.flags & 0xC000) {
					/* Non VIDC packing order */
					if (vals.flags & os_MODE_FLAG_FULL_PALETTE)
						type = osspriteop_TYPE16BPP64K;
					else
						type = osspriteop_TYPE16BPP;
					mode = (os_mode)((osspriteop_TYPEEXPANSION << osspriteop_TYPE_SHIFT) |
							osspriteop_NEW_STYLE |
							(vals.yeig << 6) |
							(vals.xeig << 4) |
							(type << 20) |
							(vals.flags & 0xFF00));
				} else {
					/* VIDC packing order */
					if (vals.flags & os_MODE_FLAG_FULL_PALETTE)
						type = osspriteop_TYPE16BPP64K;
					else
						type = osspriteop_TYPE16BPP;
					mode = (os_mode)((type << osspriteop_TYPE_SHIFT) |
							osspriteop_NEW_STYLE |
							((180 >> vals.yeig) << osspriteop_YRES_SHIFT) |
							((180 >> vals.xeig) << osspriteop_XRES_SHIFT));
				}
				break;
			default:
				NSLOG(netsurf, INFO,
				      "Unhandled 16bpp format from flags %d",
				      vals.flags);
				ro_gui_buffer_free();
				return;
			}
			break;
		case -1:
			/* 16M colours */
			switch ((vals.flags & 0x3000) >> os_MODE_FLAG_DATA_FORMAT_SHIFT) {
			case os_MODE_FLAG_DATA_FORMAT_RGB:
				if (vals.flags & 0xC000) {
					/* Non VIDC packing order */
					type = osspriteop_TYPE32BPP;
					mode = (os_mode)((osspriteop_TYPEEXPANSION << osspriteop_TYPE_SHIFT) |
							osspriteop_NEW_STYLE |
							(vals.yeig << 6) |
							(vals.xeig << 4) |
							(type << 20) |
							(vals.flags & 0xFF00));
				} else {
					/* VIDC packing order */
					type = osspriteop_TYPE32BPP;
					mode = (os_mode)((type << osspriteop_TYPE_SHIFT) |
							osspriteop_NEW_STYLE |
							((180 >> vals.yeig) << osspriteop_YRES_SHIFT) |
							((180 >> vals.xeig) << osspriteop_XRES_SHIFT));
				}
				break;
			default:
				NSLOG(netsurf, INFO,
				      "Unhandled 32bpp data format from flags %d",
				      vals.flags);
				ro_gui_buffer_free();
				return;
			}
			break;
		default:
			NSLOG(netsurf, INFO, "Unhandled NCOLOUR value %d",
			      vals.ncolour);
			ro_gui_buffer_free();
			return;
		}
	}
#endif

#ifdef BUFFER_EXCLUSIVE_USER_REDRAW
	/*	Create the sprite manually so we don't waste time clearing the
		background.
	*/
	buffer->sprite_count = 1;
	buffer->used = total_size;
	header = (osspriteop_header *)(buffer + 1);
	header->size = total_size - sizeof(osspriteop_area);
	memcpy(header->name, buffer_name, 12);
	header->width = word_width - 1;
	header->height = sprite_size.y - 1;
	header->left_bit = 0;
	header->right_bit = ((sprite_size.x << bpp) - 1) & 31;
	header->image = sizeof(osspriteop_header) + palette_size;
	header->mask = header->image;
	header->mode = mode;
	if (palette)
		xcolourtrans_read_palette((osspriteop_area *)mode,
			(osspriteop_id)os_CURRENT_MODE,
			(os_palette *)(header + 1), palette_size,
			(colourtrans_palette_flags)
				colourtrans_FLASHING_PALETTE, 0);
#else
	/*	Read the current contents of the screen
	*/
	buffer->sprite_count = 0;
	buffer->used = 16;
	if ((error = xosspriteop_get_sprite_user_coords(osspriteop_NAME,
			buffer, buffer_name, palette,
			clipping.x0, clipping.y0,
			clipping.x1, clipping.y1)) != NULL) {
		NSLOG(netsurf, INFO, "Grab error '%s'", error->errmess);
		ro_gui_buffer_free();
		return;
	}
#endif
	/*	Allocate OS_SpriteOp save area
	*/
	if ((error = xosspriteop_read_save_area_size(osspriteop_PTR,
			buffer, (osspriteop_id)(buffer + 1), &size)) != NULL) {
		NSLOG(netsurf, INFO, "Save area error '%s'", error->errmess);
		ro_gui_buffer_free();
		return;
	}
	if ((save_area = malloc((size_t)size)) == NULL) {
		ro_gui_buffer_free();
		return;
	}
	save_area->a[0] = 0;

	/*	Switch output to sprite
	*/
	if ((error = xosspriteop_switch_output_to_sprite(osspriteop_PTR,
			buffer, (osspriteop_id)(buffer + 1), save_area,
			&context0, &context1, &context2, &context3)) != NULL) {
		NSLOG(netsurf, INFO, "Switching error '%s'", error->errmess);
		free(save_area);
		ro_gui_buffer_free();
		return;
	}

	/*	Emulate an origin as the FontManager doesn't respect it in
		most cases.
	*/
	ro_plot_origin_x -= clipping.x0;
	ro_plot_origin_y -= clipping.y0;

	/*	Update the ECF origin
	*/
	if ((error = xos_set_ecf_origin(-ro_plot_origin_x,
			-ro_plot_origin_y)) != NULL) {
		NSLOG(netsurf, INFO, "Invalid ECF origin: '%s'",
		      error->errmess);
	}
}


/**
 * Closes any open buffer and flushes the contents to screen
 */
void ro_gui_buffer_close(void)
{
	/*	Check we have an open buffer
	*/
	if (!buffer)
		return;

	/*	Remove any previous redirection
	*/
	ro_plot_origin_x += clipping.x0;
	ro_plot_origin_y += clipping.y0;
	xosspriteop_unswitch_output(context0, context1, context2, context3);
	free(save_area);

	/*	Plot the contents to screen
	*/
	if (mode == tinct_SPRITE_MODE)
		_swix(Tinct_Plot, _IN(2) | _IN(3) | _IN(4) | _IN(7),
				(char *)(buffer + 1),
				clipping.x0, clipping.y0,
				nsoption_int(plot_fg_quality));
	else
		xosspriteop_put_sprite_user_coords(osspriteop_PTR,
			buffer, (osspriteop_id)(buffer + 1),
			clipping.x0, clipping.y0, (os_action)0);
	ro_gui_buffer_free();

	/*	Update the ECF origin
	*/
	os_set_ecf_origin(0, 0);
}


/**
 * Releases any buffer memory depending on cache constraints.
 */
static void ro_gui_buffer_free(void)
{
	free(buffer);
	buffer = NULL;
}
