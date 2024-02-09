/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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
 * win32 plotter implementation.
 */

#include "utils/config.h"
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <windows.h>

#include "utils/log.h"
#include "utils/utf8.h"
#include "netsurf/mouse.h"
#include "netsurf/window.h"
#include "netsurf/plotters.h"

#include "windows/bitmap.h"
#include "windows/font.h"
#include "windows/gui.h"
#include "windows/plot.h"

HDC plot_hdc;

/** currently set clipping rectangle */
static RECT plot_clip;


/**
 * bitmap helper to plot a solid block of colour
 *
 * \param col colour to plot with
 * \param x the x coordinate to plot at
 * \param y the y coordinate to plot at
 * \param width the width of block to plot
 * \param height the height to plot
 * \return NSERROR_OK on sucess else error code.
 */
static nserror
plot_block(COLORREF col, int x, int y, int width, int height)
{
	HRGN clipregion;
	HGDIOBJ original = NULL;

	/* Bail early if we can */
	if ((x >= plot_clip.right) ||
	    ((x + width) < plot_clip.left) ||
	    (y >= plot_clip.bottom) ||
	    ((y + height) < plot_clip.top)) {
		/* Image completely outside clip region */
		return NSERROR_OK;
	}

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	SelectClipRgn(plot_hdc, clipregion);

	/* Saving the original pen object */
	original = SelectObject(plot_hdc,GetStockObject(DC_PEN));

	SelectObject(plot_hdc, GetStockObject(DC_PEN));
	SelectObject(plot_hdc, GetStockObject(DC_BRUSH));
	SetDCPenColor(plot_hdc, col);
	SetDCBrushColor(plot_hdc, col);
	Rectangle(plot_hdc, x, y, width, height);

	SelectObject(plot_hdc,original); /* Restoring the original pen object */

	DeleteObject(clipregion);

	return NSERROR_OK;

}


/**
 * plot an alpha blended bitmap
 *
 * blunt force truma way of achiving alpha blended plotting
 *
 * \param hdc drawing cotext
 * \param bitmap bitmap to render
 * \param x x coordinate to plot at
 * \param y y coordinate to plot at
 * \param width The width to plot the bitmap into
 * \param height The height to plot the bitmap into
 * \return NSERROR_OK on success else appropriate error code.
 */
static nserror
plot_alpha_bitmap(HDC hdc,
		  struct bitmap *bitmap,
		  int x, int y,
		  int width, int height)
{
#ifdef WINDOWS_GDI_ALPHA_WORKED
	BLENDFUNCTION blnd = {  AC_SRC_OVER, 0, 0xff, AC_SRC_ALPHA };
	HDC bmihdc;
	bool bltres;
	bmihdc = CreateCompatibleDC(hdc);
	SelectObject(bmihdc, bitmap->windib);
	bltres = AlphaBlend(hdc,
			    x, y,
			    width, height,
			    bmihdc,
			    0, 0,
			    bitmap->width, bitmap->height,
			    blnd);
	DeleteDC(bmihdc);
	if (!bltres) {
		return NSERROR_INVALID;
	}
#else
	HDC Memhdc;
	BITMAPINFOHEADER bmih;
	int v, vv, vi, h, hh, width4, transparency;
	unsigned char alpha;
	bool isscaled = false; /* set if the scaled bitmap requires freeing */
	BITMAP MemBM;
	BITMAPINFO *bmi;
	HBITMAP MemBMh;

	NSLOG(plot, DEEPDEBUG, "%p bitmap %d,%d width %d height %d",
		 bitmap, x, y, width, height);
	NSLOG(plot, DEEPDEBUG, "clipped %ld,%ld to %ld,%ld",
		 plot_clip.left, plot_clip.top,
		 plot_clip.right, plot_clip.bottom);

	Memhdc = CreateCompatibleDC(hdc);
	if (Memhdc == NULL) {
		return NSERROR_INVALID;
	}

	if ((bitmap->width != width) ||
	    (bitmap->height != height)) {
		NSLOG(plot, DEEPDEBUG, "scaling from %d,%d to %d,%d",
			 bitmap->width, bitmap->height, width, height);
		bitmap = bitmap_scale(bitmap, width, height);
		if (bitmap == NULL) {
			return NSERROR_INVALID;
		}
		isscaled = true;
	}

	bmi = (BITMAPINFO *) malloc(sizeof(BITMAPINFOHEADER) +
				    (bitmap->width * bitmap->height * 4));
	if (bmi == NULL) {
		DeleteDC(Memhdc);
		return NSERROR_INVALID;
	}

	MemBMh = CreateCompatibleBitmap(hdc, bitmap->width, bitmap->height);
	if (MemBMh == NULL){
		free(bmi);
		DeleteDC(Memhdc);
		return NSERROR_INVALID;
	}

	/* save 'background' data for alpha channel work */
	SelectObject(Memhdc, MemBMh);
	BitBlt(Memhdc, 0, 0, bitmap->width, bitmap->height, hdc, x, y, SRCCOPY);
	GetObject(MemBMh, sizeof(BITMAP), &MemBM);

	bmih.biSize = sizeof(bmih);
	bmih.biWidth = bitmap->width;
	bmih.biHeight = bitmap->height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 4 * bitmap->height * bitmap->width;
	bmih.biXPelsPerMeter = 3600; /* 100 dpi */
	bmih.biYPelsPerMeter = 3600;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	bmi->bmiHeader = bmih;

	GetDIBits(hdc, MemBMh, 0, bitmap->height, bmi->bmiColors, bmi,
		  DIB_RGB_COLORS);

	/* then load 'foreground' bits from bitmap->pixdata */

	width4 = bitmap->width * 4;
	for (v = 0, vv = 0, vi = (bitmap->height - 1) * width4;
	     v < bitmap->height;
	     v++, vv += bitmap->width, vi -= width4) {
		for (h = 0, hh = 0; h < bitmap->width; h++, hh += 4) {
			alpha = bitmap->pixdata[vi + hh + 3];
/* multiplication of alpha value; subject to profiling could be optional */
			if (alpha == 0xFF) {
				bmi->bmiColors[vv + h].rgbBlue =
					bitmap->pixdata[vi + hh + 2];
				bmi->bmiColors[vv + h].rgbGreen =
					bitmap->pixdata[vi + hh + 1];
				bmi->bmiColors[vv + h].rgbRed =
					bitmap->pixdata[vi + hh];
			} else if (alpha > 0) {
				transparency = 0x100 - alpha;
				bmi->bmiColors[vv + h].rgbBlue =
					(bmi->bmiColors[vv + h].rgbBlue
					 * transparency +
					 (bitmap->pixdata[vi + hh + 2]) *
					 alpha) >> 8;
				bmi->bmiColors[vv + h].rgbGreen =
					(bmi->bmiColors[vv + h].
					 rgbGreen
					 * transparency +
					 (bitmap->pixdata[vi + hh + 1]) *
					 alpha) >> 8;
				bmi->bmiColors[vv + h].rgbRed =
					(bmi->bmiColors[vv + h].rgbRed
					 * transparency +
					 bitmap->pixdata[vi + hh]
					 * alpha) >> 8;
			}
		}
	}
	SetDIBitsToDevice(hdc, x, y, bitmap->width, bitmap->height,
			  0, 0, 0, bitmap->height,
			  (const void *) bmi->bmiColors,
			  bmi, DIB_RGB_COLORS);

	if (isscaled && bitmap && bitmap->pixdata) {
		free(bitmap->pixdata);
		free(bitmap);
	}

	free(bmi);
	DeleteObject(MemBMh);
	DeleteDC(Memhdc);
#endif

	return NSERROR_OK;
}


/**
 * Internal bitmap plotting
 *
 * \param bitmap The bitmap to plot
 * \param x x coordinate to plot at
 * \param y y coordinate to plot at
 * \param width The width to plot the bitmap into
 * \param height The height to plot the bitmap into
 * \return NSERROR_OK on success else appropriate error code.
 */
static nserror
plot_bitmap(struct bitmap *bitmap, int x, int y, int width, int height)
{
	HRGN clipregion;
	nserror res = NSERROR_OK;

	/* Bail early if we can */
	if ((x >= plot_clip.right) ||
	    ((x + width) < plot_clip.left) ||
	    (y >= plot_clip.bottom) ||
	    ((y + height) < plot_clip.top)) {
		/* Image completely outside clip region */
		return NSERROR_OK;
	}

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	SelectClipRgn(plot_hdc, clipregion);

	if (bitmap->opaque) {
		int bltres;
		/* opaque bitmap */
		if ((bitmap->width == width) &&
		    (bitmap->height == height)) {
			/* unscaled */
			bltres = SetDIBitsToDevice(plot_hdc,
						   x, y,
						   width, height,
						   0, 0,
						   0,
						   height,
						   bitmap->pixdata,
						   (BITMAPINFO *)bitmap->pbmi,
						   DIB_RGB_COLORS);
		} else {
			/* scaled */
			SetStretchBltMode(plot_hdc, COLORONCOLOR);
			bltres = StretchDIBits(plot_hdc,
					       x, y,
					       width, height,
					       0, 0,
					       bitmap->width, bitmap->height,
					       bitmap->pixdata,
					       (BITMAPINFO *)bitmap->pbmi,
					       DIB_RGB_COLORS,
					       SRCCOPY);


		}
		/* check to see if GDI operation failed */
		if (bltres == 0) {
			res = NSERROR_INVALID;
		}
		NSLOG(plot, DEEPDEBUG, "bltres = %d", bltres);
	} else {
		/* Bitmap with alpha.*/
		res = plot_alpha_bitmap(plot_hdc, bitmap, x, y, width, height);
	}

	DeleteObject(clipregion);

	return res;
}


/**
 * \brief Sets a clip rectangle for subsequent plot operations.
 *
 * \param ctx The current redraw context.
 * \param clip The rectangle to limit all subsequent plot
 *              operations within.
 * \return NSERROR_OK on success else error code.
 */
static nserror clip(const struct redraw_context *ctx, const struct rect *clip)
{
	NSLOG(plot, DEEPDEBUG, "clip %d,%d to %d,%d", clip->x0, clip->y0, clip->x1, clip->y1);

	plot_clip.left = clip->x0;
	plot_clip.top = clip->y0;
	plot_clip.right = clip->x1 + 1; /* co-ordinates are exclusive */
	plot_clip.bottom = clip->y1 + 1; /* co-ordinates are exclusive */

	return NSERROR_OK;
}


/**
 * Plots an arc
 *
 * plot an arc segment around (x,y), anticlockwise from angle1
 *  to angle2. Angles are measured anticlockwise from
 *  horizontal, in degrees.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the arc plot.
 * \param x The x coordinate of the arc.
 * \param y The y coordinate of the arc.
 * \param radius The radius of the arc.
 * \param angle1 The start angle of the arc.
 * \param angle2 The finish angle of the arc.
 * \return NSERROR_OK on success else error code.
 */
static nserror
arc(const struct redraw_context *ctx,
    const plot_style_t *style,
    int x, int y,
    int radius, int angle1, int angle2)
{
	NSLOG(plot, DEEPDEBUG, "arc centre %d,%d radius %d from %d to %d", x, y, radius,
		 angle1, angle2);

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	HRGN clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	COLORREF col = (DWORD)(style->stroke_colour & 0x00FFFFFF);
	HPEN pen = CreatePen(PS_GEOMETRIC | PS_SOLID, 1, col);
	if (pen == NULL) {
		DeleteObject(clipregion);
		return NSERROR_INVALID;
	}
	HGDIOBJ penbak = SelectObject(plot_hdc, (HGDIOBJ) pen);
	if (penbak == NULL) {
		DeleteObject(clipregion);
		DeleteObject(pen);
		return NSERROR_INVALID;
	}

	int q1, q2;
	double a1=1.0, a2=1.0, b1=1.0, b2=1.0;
	q1 = (int) ((angle1 + 45) / 90) - 45;
	q2 = (int) ((angle2 + 45) / 90) - 45;
	while (q1 > 4)
		q1 -= 4;
	while (q2 > 4)
		q2 -= 4;
	while (q1 <= 0)
		q1 += 4;
	while (q2 <= 0)
		q2 += 4;
	angle1 = ((angle1 + 45) % 90) - 45;
	angle2 = ((angle2 + 45) % 90) - 45;

	switch(q1) {
	case 1:
		a1 = 1.0;
		b1 = -tan((M_PI / 180) * angle1);
		break;
	case 2:
		b1 = -1.0;
		a1 = -tan((M_PI / 180) * angle1);
		break;
	case 3:
		a1 = -1.0;
		b1 = tan((M_PI / 180) * angle1);
		break;
	case 4:
		b1 = 1.0;
		a1 = tan((M_PI / 180) * angle1);
		break;
	}

	switch(q2) {
	case 1:
		a2 = 1.0;
		b2 = -tan((M_PI / 180) * angle2);
		break;
	case 2:
		b2 = -1.0;
		a2 = -tan((M_PI / 180) * angle2);
		break;
	case 3:
		a2 = -1.0;
		b2 = tan((M_PI / 180) * angle2);
		break;
	case 4:
		b2 = 1.0;
		a2 = tan((M_PI / 180) * angle2);
		break;
	}

	SelectClipRgn(plot_hdc, clipregion);

	Arc(plot_hdc, x - radius, y - radius, x + radius, y + radius,
	    x + (int)(a1 * radius), y + (int)(b1 * radius),
	    x + (int)(a2 * radius), y + (int)(b2 * radius));

	SelectClipRgn(plot_hdc, NULL);
	pen = SelectObject(plot_hdc, penbak);
	DeleteObject(clipregion);
	DeleteObject(pen);

	return NSERROR_OK;
}


/**
 * Plots a circle
 *
 * Plot a circle centered on (x,y), which is optionally filled.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the circle plot.
 * \param x x coordinate of circle centre.
 * \param y y coordinate of circle centre.
 * \param radius circle radius.
 * \return NSERROR_OK on success else error code.
 */
static nserror
disc(const struct redraw_context *ctx,
     const plot_style_t *style,
     int x, int y, int radius)
{
	NSLOG(plot, DEEPDEBUG, "disc at %d,%d radius %d", x, y, radius);

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	HRGN clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	COLORREF col = (DWORD)((style->fill_colour | style->stroke_colour)
			       & 0x00FFFFFF);
	HPEN pen = CreatePen(PS_GEOMETRIC | PS_SOLID, 1, col);
	if (pen == NULL) {
		DeleteObject(clipregion);
		return NSERROR_INVALID;
	}
	HGDIOBJ penbak = SelectObject(plot_hdc, (HGDIOBJ) pen);
	if (penbak == NULL) {
		DeleteObject(clipregion);
		DeleteObject(pen);
		return NSERROR_INVALID;
	}
	HBRUSH brush = CreateSolidBrush(col);
	if (brush == NULL) {
		DeleteObject(clipregion);
		SelectObject(plot_hdc, penbak);
		DeleteObject(pen);
		return NSERROR_INVALID;
	}
	HGDIOBJ brushbak = SelectObject(plot_hdc, (HGDIOBJ) brush);
	if (brushbak == NULL) {
		DeleteObject(clipregion);
		SelectObject(plot_hdc, penbak);
		DeleteObject(pen);
		DeleteObject(brush);
		return NSERROR_INVALID;
	}

	SelectClipRgn(plot_hdc, clipregion);

	if (style->fill_type == PLOT_OP_TYPE_NONE) {
		Arc(plot_hdc, x - radius, y - radius, x + radius, y + radius,
		    x - radius, y - radius,
		    x - radius, y - radius);
	} else {
		Ellipse(plot_hdc, x - radius, y - radius, x + radius, y + radius);
	}

	SelectClipRgn(plot_hdc, NULL);
	pen = SelectObject(plot_hdc, penbak);
	brush = SelectObject(plot_hdc, brushbak);
	DeleteObject(clipregion);
	DeleteObject(pen);
	DeleteObject(brush);

	return NSERROR_OK;
}


/**
 * Plots a line
 *
 * plot a line from (x0,y0) to (x1,y1). Coordinates are at
 *  centre of line width/thickness.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the line plot.
 * \param line A rectangle defining the line to be drawn
 * \return NSERROR_OK on success else error code.
 */
static nserror
line(const struct redraw_context *ctx,
     const plot_style_t *style,
     const struct rect *line)
{
	NSLOG(plot, DEEPDEBUG, "from %d,%d to %d,%d",
	      line->x0, line->y0, line->x1, line->y1);

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	HRGN clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	COLORREF col = (DWORD)(style->stroke_colour & 0x00FFFFFF);
	/* windows 0x00bbggrr */
	DWORD penstyle = PS_GEOMETRIC |
		((style->stroke_type == PLOT_OP_TYPE_DOT) ? PS_DOT :
		 (style->stroke_type == PLOT_OP_TYPE_DASH) ? PS_DASH:
		 0);
	LOGBRUSH lb = {BS_SOLID, col, 0};
	HPEN pen = ExtCreatePen(penstyle,
			plot_style_fixed_to_int(style->stroke_width),
			&lb, 0, NULL);
	if (pen == NULL) {
		DeleteObject(clipregion);
		return NSERROR_INVALID;
	}
	HGDIOBJ bak = SelectObject(plot_hdc, (HGDIOBJ) pen);
	if (bak == NULL) {
		DeleteObject(pen);
		DeleteObject(clipregion);
		return NSERROR_INVALID;
	}

	SelectClipRgn(plot_hdc, clipregion);

	MoveToEx(plot_hdc, line->x0, line->y0, (LPPOINT) NULL);

	LineTo(plot_hdc, line->x1, line->y1);

	SelectClipRgn(plot_hdc, NULL);
	pen = SelectObject(plot_hdc, bak);

	DeleteObject(pen);
	DeleteObject(clipregion);

	return NSERROR_OK;
}


/**
 * Plots a rectangle.
 *
 * The rectangle can be filled an outline or both controlled
 *  by the plot style The line can be solid, dotted or
 *  dashed. Top left corner at (x0,y0) and rectangle has given
 *  width and height.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the rectangle plot.
 * \param rect A rectangle defining the line to be drawn
 * \return NSERROR_OK on success else error code.
 */
static nserror
rectangle(const struct redraw_context *ctx,
	  const plot_style_t *style,
	  const struct rect *rect)
{
	NSLOG(plot, DEEPDEBUG, "rectangle from %d,%d to %d,%d",
		 rect->x0, rect->y0, rect->x1, rect->y1);

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	HRGN clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	COLORREF pencol = (DWORD)(style->stroke_colour & 0x00FFFFFF);
	DWORD penstyle = PS_GEOMETRIC |
		(style->stroke_type == PLOT_OP_TYPE_DOT ? PS_DOT :
		 (style->stroke_type == PLOT_OP_TYPE_DASH ? PS_DASH :
		  (style->stroke_type == PLOT_OP_TYPE_NONE ? PS_NULL :
		   0)));
	LOGBRUSH lb = {BS_SOLID, pencol, 0};
	LOGBRUSH lb1 = {BS_SOLID, style->fill_colour, 0};
	if (style->fill_type == PLOT_OP_TYPE_NONE)
		lb1.lbStyle = BS_HOLLOW;

	HPEN pen = ExtCreatePen(penstyle,
			plot_style_fixed_to_int(style->stroke_width),
			&lb, 0, NULL);
	if (pen == NULL) {
		return NSERROR_INVALID;
	}
	HGDIOBJ penbak = SelectObject(plot_hdc, (HGDIOBJ) pen);
	if (penbak == NULL) {
		DeleteObject(pen);
		return NSERROR_INVALID;
	}
	HBRUSH brush = CreateBrushIndirect(&lb1);
	if (brush  == NULL) {
		SelectObject(plot_hdc, penbak);
		DeleteObject(pen);
		return NSERROR_INVALID;
	}
	HGDIOBJ brushbak = SelectObject(plot_hdc, (HGDIOBJ) brush);
	if (brushbak == NULL) {
		SelectObject(plot_hdc, penbak);
		DeleteObject(pen);
		DeleteObject(brush);
		return NSERROR_INVALID;
	}

	SelectClipRgn(plot_hdc, clipregion);

	/* windows GDI call coordinates are inclusive */
	Rectangle(plot_hdc, rect->x0, rect->y0, rect->x1 + 1, rect->y1 + 1);

	pen = SelectObject(plot_hdc, penbak);
	brush = SelectObject(plot_hdc, brushbak);
	SelectClipRgn(plot_hdc, NULL);
	DeleteObject(pen);
	DeleteObject(brush);
	DeleteObject(clipregion);

	return NSERROR_OK;
}


/**
 * Plot a polygon
 *
 * Plots a filled polygon with straight lines between
 * points. The lines around the edge of the ploygon are not
 * plotted. The polygon is filled with the non-zero winding
 * rule.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the polygon plot.
 * \param p verticies of polygon
 * \param n number of verticies.
 * \return NSERROR_OK on success else error code.
 */
static nserror
polygon(const struct redraw_context *ctx,
	const plot_style_t *style,
	const int *p,
	unsigned int n)
{
	NSLOG(plot, DEEPDEBUG, "polygon %d points", n);

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	POINT points[n];
	unsigned int i;
	HRGN clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	COLORREF pencol = (DWORD)(style->fill_colour & 0x00FFFFFF);
	COLORREF brushcol = (DWORD)(style->fill_colour & 0x00FFFFFF);
	HPEN pen = CreatePen(PS_GEOMETRIC | PS_NULL, 1, pencol);
	if (pen == NULL) {
		DeleteObject(clipregion);
		return NSERROR_INVALID;
	}
	HPEN penbak = SelectObject(plot_hdc, pen);
	if (penbak == NULL) {
		DeleteObject(clipregion);
		DeleteObject(pen);
		return NSERROR_INVALID;
	}
	HBRUSH brush = CreateSolidBrush(brushcol);
	if (brush == NULL) {
		DeleteObject(clipregion);
		SelectObject(plot_hdc, penbak);
		DeleteObject(pen);
		return NSERROR_INVALID;
	}
	HBRUSH brushbak = SelectObject(plot_hdc, brush);
	if (brushbak == NULL) {
		DeleteObject(clipregion);
		SelectObject(plot_hdc, penbak);
		DeleteObject(pen);
		DeleteObject(brush);
		return NSERROR_INVALID;
	}
	SetPolyFillMode(plot_hdc, WINDING);
	for (i = 0; i < n; i++) {
		points[i].x = (long) p[2 * i];
		points[i].y = (long) p[2 * i + 1];

		NSLOG(plot, DEEPDEBUG, "%ld,%ld ", points[i].x, points[i].y);
	}

	SelectClipRgn(plot_hdc, clipregion);

	if (n >= 2) {
		Polygon(plot_hdc, points, n);
	}

	SelectClipRgn(plot_hdc, NULL);

	pen = SelectObject(plot_hdc, penbak);
	brush = SelectObject(plot_hdc, brushbak);
	DeleteObject(clipregion);
	DeleteObject(pen);
	DeleteObject(brush);

	return NSERROR_OK;
}


/**
 * Plots a path.
 *
 * Path plot consisting of cubic Bezier curves. Line and fill colour is
 *  controlled by the plot style.
 *
 * \param ctx The current redraw context.
 * \param pstyle Style controlling the path plot.
 * \param p elements of path
 * \param n nunber of elements on path
 * \param transform A transform to apply to the path.
 * \return NSERROR_OK on success else error code.
 */
static nserror
path(const struct redraw_context *ctx,
     const plot_style_t *pstyle,
     const float *p,
     unsigned int n,
     const float transform[6])
{
	NSLOG(plot, DEEPDEBUG, "path unimplemented");
	return NSERROR_OK;
}


/**
 * Plot a bitmap
 *
 * Tiled plot of a bitmap image. (x,y) gives the top left
 * coordinate of an explicitly placed tile. From this tile the
 * image can repeat in all four directions -- up, down, left
 * and right -- to the extents given by the current clip
 * rectangle.
 *
 * The bitmap_flags say whether to tile in the x and y
 * directions. If not tiling in x or y directions, the single
 * image is plotted. The width and height give the dimensions
 * the image is to be scaled to.
 *
 * \param ctx The current redraw context.
 * \param bitmap The bitmap to plot
 * \param x The x coordinate to plot the bitmap
 * \param y The y coordiante to plot the bitmap
 * \param width The width of area to plot the bitmap into
 * \param height The height of area to plot the bitmap into
 * \param bg the background colour to alpha blend into
 * \param flags the flags controlling the type of plot operation
 * \return NSERROR_OK on success else error code.
 */
static nserror
bitmap(const struct redraw_context *ctx,
       struct bitmap *bitmap,
       int x, int y,
       int width,
       int height,
       colour bg,
       bitmap_flags_t flags)
{
	int xf,yf;
	bool repeat_x = (flags & BITMAPF_REPEAT_X);
	bool repeat_y = (flags & BITMAPF_REPEAT_Y);

	/* Bail early if we can */

	NSLOG(plot, DEEPDEBUG, "Plotting %p at %d,%d by %d,%d",bitmap, x,y,width,height);

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "Passed null bitmap!");
		return NSERROR_OK;
	}

	/* check if nothing to plot */
	if (width == 0 || height == 0)
		return NSERROR_OK;

	/* x and y define coordinate of top left of of the initial explicitly
	 * placed tile. The width and height are the image scaling and the
	 * bounding box defines the extent of the repeat (which may go in all
	 * four directions from the initial tile).
	 */

	if (!(repeat_x || repeat_y)) {
		/* Not repeating at all, so just plot it */
		if ((bitmap->width == 1) && (bitmap->height == 1)) {
			if ((*(bitmap->pixdata + 3) & 0xff) == 0) {
				return NSERROR_OK;
			}
			return plot_block((*(COLORREF *)bitmap->pixdata) & 0xffffff,
					  x,
					  y,
					  x + width,
					  y + height);

		} else {
			return plot_bitmap(bitmap, x, y, width, height);
		}
	}

	/* Optimise tiled plots of 1x1 bitmaps by replacing with a flat fill
	 * of the area.  Can only be done when image is fully opaque. */
	if ((bitmap->width == 1) && (bitmap->height == 1)) {
		if ((*(COLORREF *)bitmap->pixdata & 0xff000000) != 0) {
			return plot_block((*(COLORREF *)bitmap->pixdata) & 0xffffff,
					  plot_clip.left,
					  plot_clip.top,
					  plot_clip.right,
					  plot_clip.bottom);
		}
	}

	/* Optimise tiled plots of bitmaps scaled to 1x1 by replacing with
	 * a flat fill of the area.  Can only be done when image is fully
	 * opaque.
	 */
	if ((width == 1) && (height == 1)) {
		if (bitmap->opaque) {
			/** TODO: Currently using top left pixel. Maybe centre
			 *        pixel or average value would be better. */
			return plot_block((*(COLORREF *)bitmap->pixdata) & 0xffffff,
					  plot_clip.left,
					  plot_clip.top,
					  plot_clip.right,
					  plot_clip.bottom);
		}
	}

	NSLOG(plot, DEEPDEBUG, "Tiled plotting %d,%d by %d,%d", x, y, width, height);
	NSLOG(plot, DEEPDEBUG, "clipped %ld,%ld to %ld,%ld",
		 plot_clip.left, plot_clip.top,
		 plot_clip.right, plot_clip.bottom);

	/* get left most tile position */
	if (repeat_x) {
		for (; x > plot_clip.left; x -= width);
	}

	/* get top most tile position */
	if (repeat_y) {
		for (; y > plot_clip.top; y -= height);
	}

	NSLOG(plot, DEEPDEBUG, "repeat from %d,%d to %ld,%ld",
		 x, y, plot_clip.right, plot_clip.bottom);

	/* tile down and across to extents */
	for (xf = x; xf < plot_clip.right; xf += width) {
		for (yf = y; yf < plot_clip.bottom; yf += height) {

			plot_bitmap(bitmap, xf, yf, width, height);
			if (!repeat_y)
				break;
		}
		if (!repeat_x)
			break;
	}
	return NSERROR_OK;
}


/**
 * Text plotting.
 *
 * \param ctx The current redraw context.
 * \param fstyle plot style for this text
 * \param x x coordinate
 * \param y y coordinate
 * \param text UTF-8 string to plot
 * \param length length of string, in bytes
 * \return NSERROR_OK on success else error code.
 */
static nserror
text(const struct redraw_context *ctx,
     const struct plot_font_style *fstyle,
     int x,
     int y,
     const char *text,
     size_t length)
{
	NSLOG(plot, DEEPDEBUG, "words %s at %d,%d", text, x, y);

	/* ensure the plot HDC is set */
	if (plot_hdc == NULL) {
		NSLOG(netsurf, INFO, "HDC not set on call to plotters");
		return NSERROR_INVALID;
	}

	HRGN clipregion = CreateRectRgnIndirect(&plot_clip);
	if (clipregion == NULL) {
		return NSERROR_INVALID;
	}

	HFONT fontbak, font = get_font(fstyle);
	if (font == NULL) {
		DeleteObject(clipregion);
		return NSERROR_INVALID;
	}
	int wlen;
	SIZE s;
	LPWSTR wstring;
	fontbak = (HFONT) SelectObject(plot_hdc, font);
	GetTextExtentPoint(plot_hdc, text, length, &s);

	SelectClipRgn(plot_hdc, clipregion);

	SetTextAlign(plot_hdc, TA_BASELINE | TA_LEFT);
	if ((fstyle->background & 0xFF000000) != 0x01000000) {
		/* 100% alpha */
		SetBkColor(plot_hdc, (DWORD) (fstyle->background & 0x00FFFFFF));
	}
	SetBkMode(plot_hdc, TRANSPARENT);
	SetTextColor(plot_hdc, (DWORD) (fstyle->foreground & 0x00FFFFFF));

	wlen = MultiByteToWideChar(CP_UTF8, 0, text, length, NULL, 0);
	wstring = malloc(2 * (wlen + 1));
	if (wstring == NULL) {
		return NSERROR_INVALID;
	}
	MultiByteToWideChar(CP_UTF8, 0, text, length, wstring, wlen);
	TextOutW(plot_hdc, x, y, wstring, wlen);

	SelectClipRgn(plot_hdc, NULL);
	free(wstring);
	font = SelectObject(plot_hdc, fontbak);
	DeleteObject(clipregion);
	DeleteObject(font);

	return NSERROR_OK;
}


/**
 * win32 API plot operation table
 */
const struct plotter_table win_plotters = {
	.rectangle = rectangle,
	.line = line,
	.polygon = polygon,
	.clip = clip,
	.text = text,
	.disc = disc,
	.arc = arc,
	.bitmap = bitmap,
	.path = path,
	.option_knockout = true,
};
