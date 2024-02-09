/*
 * Copyright 2015 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
 * Abstract RTG functions for newer/older/non-P96 systems
 */

#ifndef AMIGA_RTG_H
#define AMIGA_RTG_H 1
#include <proto/graphics.h>
#include <proto/Picasso96API.h>

/* Wrappers for Alloc/FreeBitMap */
struct BitMap *ami_rtg_allocbitmap(ULONG width, ULONG height, ULONG depth,
	ULONG flags, struct BitMap *friend, RGBFTYPE format);
void ami_rtg_freebitmap(struct BitMap *bm);

/* WritePixelArray wrapper.  This isn't entirely (at all) equivalent to p96WPA */
void ami_rtg_writepixelarray(UBYTE *pixdata, struct BitMap *bm,
	ULONG width, ULONG height, ULONG bpr, ULONG format);
#endif

