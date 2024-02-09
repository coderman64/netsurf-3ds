/*
 * Copyright 2008, 2009 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_PLOTTERS_H
#define AMIGA_PLOTTERS_H

#include "netsurf/plotters.h"

struct IBox;
struct gui_globals;

extern const struct plotter_table amiplot;

void ami_clearclipreg(struct gui_globals *gg);
void ami_plot_clear_bbox(struct RastPort *rp, struct IBox *bbox);
void ami_plot_release_pens(struct MinList *shared_pens);
bool ami_plot_screen_is_palettemapped(void);

/* Plotter render area management */

/**
 * Alloc a plotter render area
 * \param width of render bitmap
 * \param height of render bitmap
 * \param force32bit allocate a 32-bit bitmap even if this does not match the screen
 * \param alloc_pen_list set to false to use own pen list (eg. if multiple pen lists will be required)
 * \returns pointer to render area
 */
struct gui_globals *ami_plot_ra_alloc(ULONG width, ULONG height, bool force32bit, bool alloc_pen_list);

/**
 * Free a plotter render area
 * \param gg render area to free
 */
void ami_plot_ra_free(struct gui_globals *gg);

/**
 * Get RastPort associated with a render area
 * \param gg render area
 * \returns pointer to render area BitMap
 */
struct RastPort *ami_plot_ra_get_rastport(struct gui_globals *gg);

/**
 * Get a drawing BitMap associated with a render area
 * \param gg render area
 * \returns pointer to render area BitMap
 */
struct BitMap *ami_plot_ra_get_bitmap(struct gui_globals *gg);

/**
 * Get size of BitMap associated with a render area
 * \param gg render area
 * \param width updated to BitMap width
 * \param height updated to BitMap height
 */
void ami_plot_ra_get_size(struct gui_globals *gg, int *width, int *height);

/**
 * Set a list of shared pens for a render area to use
 * Only relevant for palette-mapped screens
 * \param gg render area
 * \param pen_list allocated by ami_AllocMinList()
 */
void ami_plot_ra_set_pen_list(struct gui_globals *gg, struct MinList *pen_list);

#endif

