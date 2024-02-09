/*
 * Copyright 2004 John M Bell <jmb202@ecs.soton.ac.uk>
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
#include <swis.h>
#include <stdlib.h>
#include <oslib/colourtrans.h>
#include <oslib/osspriteop.h>

#include "utils/nsoption.h"
#include "utils/log.h"

#include "riscos/image.h"
#include "riscos/gui.h"
#include "riscos/wimp.h"
#include "riscos/tinct.h"

/**
 * Plot an image at the given coordinates using tinct
 *
 * \param header            The sprite header
 * \param x                 Left edge of sprite
 * \param y                 Top edge of sprite
 * \param req_width         The requested width of the sprite
 * \param req_height        The requested height of the sprite
 * \param width             The actual width of the sprite
 * \param height            The actual height of the sprite
 * \param background_colour The background colour to blend to
 * \param repeatx           Repeat the image in the x direction
 * \param repeaty           Repeat the image in the y direction
 * \param alpha             Use the alpha channel
 * \param tinct_options	    The base option set to use
 * \return true on success, false otherwise
 */
static bool image_redraw_tinct(osspriteop_id header, int x, int y,
		int req_width, int req_height, int width, int height,
		colour background_colour, bool repeatx, bool repeaty,
		bool alpha, unsigned int tinct_options)
{
	_kernel_oserror *error;

	/*	Set up our flagword
	*/
	tinct_options |= background_colour << tinct_BACKGROUND_SHIFT;
	if (print_active)
		tinct_options |= tinct_USE_OS_SPRITE_OP;
	if (repeatx)
		tinct_options |= tinct_FILL_HORIZONTALLY;
	if (repeaty)
		tinct_options |= tinct_FILL_VERTICALLY;

	if (alpha) {
		error = _swix(Tinct_PlotScaledAlpha, _INR(2,7),
				header, x, y,
				req_width, req_height, tinct_options);
	} else {
		error = _swix(Tinct_PlotScaled, _INR(2,7),
				header, x, y,
				req_width, req_height, tinct_options);
	}

	if (error) {
		NSLOG(netsurf, INFO, "xtinct_plotscaled%s: 0x%x: %s",
		      (alpha ? "alpha" : ""), error->errnum, error->errmess);
		return false;
	}

	return true;
}

/**
 * Plot an image at the given coordinates using os_spriteop
 *
 * \param header     The sprite header
 * \param x          Left edge of sprite
 * \param y          Top edge of sprite
 * \param req_width  The requested width of the sprite
 * \param req_height The requested height of the sprite
 * \param width      The actual width of the sprite
 * \param height     The actual height of the sprite
 * \param tile       Whether to tile the sprite
 * \return true on success, false otherwise
 */
static bool image_redraw_os(osspriteop_id header, int x, int y, int req_width,
		int req_height, int width, int height, bool tile)
{
	int size;
	os_factors f;
	osspriteop_trans_tab *table;
	os_error *error;

	error = xcolourtrans_generate_table_for_sprite(
			osspriteop_UNSPECIFIED, header,
			os_CURRENT_MODE,
			colourtrans_CURRENT_PALETTE,
			0, colourtrans_GIVEN_SPRITE, 0, 0, &size);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xcolourtrans_generate_table_for_sprite: 0x%x: %s",
		      error->errnum,
		      error->errmess);
		return false;
	}

	table = calloc(size, sizeof(char));
	if (!table) {
		NSLOG(netsurf, INFO, "malloc failed");
		ro_warn_user("NoMemory", 0);
		return false;
	}

	error = xcolourtrans_generate_table_for_sprite(
			osspriteop_UNSPECIFIED, header,
			os_CURRENT_MODE,
			colourtrans_CURRENT_PALETTE,
			table, colourtrans_GIVEN_SPRITE, 0, 0, 0);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xcolourtrans_generate_table_for_sprite: 0x%x: %s",
		      error->errnum,
		      error->errmess);
		free(table);
		return false;
	}

	f.xmul = req_width;
	f.ymul = req_height;
	f.xdiv = width;
	f.ydiv = height;

	if (tile) {
		error = xosspriteop_plot_tiled_sprite(osspriteop_PTR,
				osspriteop_UNSPECIFIED, header, x, y,
				osspriteop_USE_MASK, &f, table);
	} else {
		error = xosspriteop_put_sprite_scaled(osspriteop_PTR,
				osspriteop_UNSPECIFIED, header, x, y,
				osspriteop_USE_MASK, &f, table);
	}
	if (error) {
		NSLOG(netsurf, INFO,
		      "xosspriteop_put_sprite_scaled: 0x%x: %s",
		      error->errnum,
		      error->errmess);
		free(table);
		return false;
	}

	free(table);

	return true;
}

/**
 * Override a sprite's mode.
 *
 * Only replaces mode if existing mode matches \ref old.
 *
 * \param[in] area  The sprite area containing the sprite.
 * \param[in] type  Requested plot mode.
 * \param[in] old   Existing sprite mode to check for.
 * \param[in] new   Sprite mode to set if existing mode is expected.
 */
static inline void image__override_sprite_mode(
		osspriteop_area *area,
		image_type type,
		os_mode old,
		os_mode new)
{
	osspriteop_header *sprite = (osspriteop_header *)(area + 1);

	if (sprite->mode == old && type == IMAGE_PLOT_TINCT_ALPHA) {
		sprite->mode = new;
	}
}

/**
 * Plot an image at the given coordinates using the method specified
 *
 * \param area              The sprite area containing the sprite
 * \param x                 Left edge of sprite
 * \param y                 Top edge of sprite
 * \param req_width         The requested width of the sprite
 * \param req_height        The requested height of the sprite
 * \param width             The actual width of the sprite
 * \param height            The actual height of the sprite
 * \param background_colour The background colour to blend to
 * \param repeatx           Repeat the image in the x direction
 * \param repeaty           Repeat the image in the y direction
 * \param background	    Use background image settings (otherwise foreground)
 * \param type              The plot method to use
 * \return true on success, false otherwise
 */
bool image_redraw(osspriteop_area *area, int x, int y, int req_width,
		int req_height, int width, int height,
		colour background_colour,
		bool repeatx, bool repeaty, bool background, image_type type)
{
	image_type used_type = type;
	unsigned int tinct_options;
	bool tinct_avoid = false;
	bool res = false;

	/* failed decompression/loading can result in no image being present */
	if (!area)
		return false;

	osspriteop_id header = (osspriteop_id)
			((char*) area + area->first);

	req_width *= 2;
	req_height *= 2;
	width *= 2;
	height *= 2;
	y -= req_height;

	tinct_options = background ? nsoption_int(plot_bg_quality) :
		nsoption_int(plot_fg_quality);

	if (os_alpha_sprite_supported) {
		/* Ideally Tinct would be updated to understand that modern OS
		 * versions can cope with alpha channels, and we could continue
		 * to pass to Tinct.  The main drawback of fully avoiding Tinct
		 * is that we lose the optimisation for tiling tiny bitmaps.
		 */
		if (tinct_options & tinct_USE_OS_SPRITE_OP) {
			used_type = IMAGE_PLOT_OS;
			tinct_avoid = true;
		}
	}

	if (tinct_avoid) {
		int xeig;
		int yeig;

		if (ro_gui_wimp_read_eig_factors(os_CURRENT_MODE,
				&xeig, &yeig)) {

			req_width  = (req_width  / 2) * (4 >> xeig);
			req_height = (req_height / 2) * (4 >> yeig);
		}
	}

	switch (used_type) {
		case IMAGE_PLOT_TINCT_ALPHA:
			res = image_redraw_tinct(header, x, y,
						req_width, req_height,
						width, height,
						background_colour,
						repeatx, repeaty, true,
						tinct_options);
			break;

		case IMAGE_PLOT_TINCT_OPAQUE:
			res = image_redraw_tinct(header, x, y,
						req_width, req_height,
						width, height,
						background_colour,
						repeatx, repeaty, false,
						tinct_options);
			break;

		case IMAGE_PLOT_OS:
			if (tinct_avoid) {
				image__override_sprite_mode(area, type,
						tinct_SPRITE_MODE,
						alpha_SPRITE_MODE);
			}
			res = image_redraw_os(header, x, y, req_width,
						req_height, width, height,
						repeatx | repeaty);
			if (tinct_avoid) {
				image__override_sprite_mode(area, type,
						alpha_SPRITE_MODE,
						tinct_SPRITE_MODE);
			}
			break;

		default:
			break;
	}

	return res;
}
