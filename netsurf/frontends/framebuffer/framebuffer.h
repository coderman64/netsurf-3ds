/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
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

/**
 * \file
 * framebuffer interface.
 */

#ifndef NETSURF_FB_FRAMEBUFFER_H
#define NETSURF_FB_FRAMEBUFFER_H

extern const struct plotter_table fb_plotters;

nsfb_t *framebuffer_initialise(const char *fename, int width, int height, int bpp);
bool framebuffer_resize(nsfb_t *nsfb, int width, int height, int bpp);
void framebuffer_finalise(void);
bool framebuffer_set_cursor(struct fbtk_bitmap *bm);

/** Set framebuffer surface to render into
 *
 * @return return old surface
 */
nsfb_t *framebuffer_set_surface(nsfb_t *new_nsfb);

#endif
