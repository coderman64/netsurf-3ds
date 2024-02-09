/*
 * Copyright 2009 Vincent Sanders <vince@simtec.co.uk>
 * Copyright 2010 Michael Drake <tlsa@netsurf-browser.org>
 *
 * This file is part of libnsfb, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#include <stdbool.h>
#include <stdlib.h>

#include "libnsfb.h"
#include "libnsfb_plot.h"
#include "libnsfb_plot_util.h"

#include "nsfb.h"
#include "plot.h"

#define UNUSED __attribute__((unused))


/**
 * Get the address of a logical location on the framebuffer
 */
static inline uint32_t *get_xy_loc(nsfb_t *nsfb, int x, int y)
{
        return (void *)(nsfb->ptr + (y * nsfb->linelen) + (x << 2));
}

#ifdef NSFB_BE_BYTE_ORDER

/**
 * convert a 32bpp big endian pixel value to netsurf colour
 *
 * \param nsfb The framebuffer
 * \param pixel The pixel value
 * \return The netsurf colour value.
 */
static inline nsfb_colour_t pixel_to_colour(UNUSED nsfb_t *nsfb, uint32_t pixel)
{
        /** \todo fix this conversion as it is probably wrong */
        return (pixel >> 8) & ~0xFF000000U;
}

/**
 * convert a colour value to a big endian 32bpp pixel value
 *
 * \param nsfb The framebuffer
 * \param c The framebuffer colour
 * \return A pixel value ready for screen output.
 */
static inline uint32_t colour_to_pixel(UNUSED nsfb_t *nsfb, nsfb_colour_t c)
{
        return ((c & 0xFF) << 24) | ((c & 0xFF00) << 8) | ((c & 0xFF0000) >> 8);
}

#else

/**
 * convert a 32bpp little endian pixel value to netsurf colour
 *
 * \param nsfb The framebuffer
 * \param pixel The pixel value
 * \return The netsurf colour value.
 */
static inline nsfb_colour_t pixel_to_colour(UNUSED nsfb_t *nsfb, uint32_t pixel)
{
        return pixel | 0xFF000000U;
}


/**
 * convert a colour value to a little endian 32bpp pixel value
 *
 * \param nsfb The framebuffer
 * \param c The netsurf colour
 * \return A pixel value ready for screen output.
 */
static inline uint32_t colour_to_pixel(UNUSED nsfb_t *nsfb, nsfb_colour_t c)
{
        return c;
}

#endif

#define PLOT_TYPE uint32_t
#define PLOT_LINELEN(ll) ((ll) >> 2)

#include "32bpp-common.c"

const nsfb_plotter_fns_t _nsfb_32bpp_xbgr8888_plotters = {
        .line = line,
        .fill = fill,
        .point = point,
        .bitmap = bitmap,
        .bitmap_tiles = bitmap_tiles,
        .glyph8 = glyph8,
        .glyph1 = glyph1,
        .readrect = readrect,
};

/*
 * Local Variables:
 * c-basic-offset:8
 * End:
 */
