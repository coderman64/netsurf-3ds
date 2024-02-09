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

#include "amiga/rtg.h"

struct BitMap *ami_rtg_allocbitmap(ULONG width, ULONG height, ULONG depth,
	ULONG flags, struct BitMap *friend, RGBFTYPE format)
{
	if(P96Base == NULL) {
		return AllocBitMap(width, height, depth, flags, friend);
	} else {
		return p96AllocBitMap(width, height, depth, flags, friend, format);
	}
}

void ami_rtg_freebitmap(struct BitMap *bm)
{
	if(P96Base == NULL) {
		return FreeBitMap(bm);
	} else {
		return p96FreeBitMap(bm);
	}
}

void ami_rtg_writepixelarray(UBYTE *pixdata, struct BitMap *bm,
	ULONG width, ULONG height, ULONG bpr, ULONG format)
{
	struct RastPort trp;

	InitRastPort(&trp);
	trp.BitMap = bm;

	/* This requires P96 or gfx.lib v54 currently */
	if(P96Base == NULL) {
#ifdef __amigaos4__
		if(GfxBase->LibNode.lib_Version >= 54) {
			WritePixelArray(pixdata, 0, 0, bpr, PIXF_R8G8B8A8, &trp, 0, 0, width, height);
		}
#endif
	} else {
		struct RenderInfo ri;

		ri.Memory = pixdata;
		ri.BytesPerRow = bpr;
		ri.RGBFormat = format;

		p96WritePixelArray((struct RenderInfo *)&ri, 0, 0, &trp, 0, 0, width, height);
	}
}

