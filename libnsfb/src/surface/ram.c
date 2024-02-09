/*
 * Copyright 2009 Vincent Sanders <vince@simtec.co.uk>
 *
 * This file is part of libnsfb, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnsfb.h"
#include "libnsfb_plot.h"
#include "libnsfb_event.h"

#include "nsfb.h"
#include "surface.h"
#include "plot.h"

#define UNUSED(x) ((x) = (x))

static int ram_defaults(nsfb_t *nsfb)
{
    nsfb->width = 0;
    nsfb->height = 0;
    nsfb->format = NSFB_FMT_ABGR8888;

    /* select default sw plotters for bpp */
    select_plotters(nsfb);

    return 0;
}


static int ram_initialise(nsfb_t *nsfb)
{
    size_t size;
    uint8_t *fbptr;

    size = (nsfb->width * nsfb->height * nsfb->bpp) / 8;
    fbptr = realloc(nsfb->ptr, size);
    if (fbptr == NULL) {
        return -1;
    }

    nsfb->ptr = fbptr;
    nsfb->linelen = (nsfb->width * nsfb->bpp) / 8;

    return 0;
}

static int
ram_set_geometry(nsfb_t *nsfb, int width, int height, enum nsfb_format_e format)
{
    int startsize; 
    int endsize;

    int prev_width;
    int prev_height;
    enum nsfb_format_e prev_format;

    prev_width = nsfb->width;
    prev_height = nsfb->height;
    prev_format = nsfb->format;

    startsize = (nsfb->width * nsfb->height * nsfb->bpp) / 8;

    if (width > 0) {
	nsfb->width = width;
    }

    if (height > 0) {
	nsfb->height = height;
    }

    if (format != NSFB_FMT_ANY) {
	nsfb->format = format;
    }

    /* select soft plotters appropriate for format */
    select_plotters(nsfb);

    /* reallocate surface memory if necessary */
    endsize = (nsfb->width * nsfb->height * nsfb->bpp) / 8;
    if ((nsfb->ptr != NULL) && (startsize != endsize)) {
        uint8_t *fbptr;
        fbptr = realloc(nsfb->ptr, endsize);
        if (fbptr == NULL) {
            /* allocation failed so put everything back as it was */
            nsfb->width = prev_width;
            nsfb->height = prev_height;
            nsfb->format = prev_format;
            select_plotters(nsfb);

            return -1;
        }
        nsfb->ptr = fbptr;
    }

    nsfb->linelen = (nsfb->width * nsfb->bpp) / 8;

    return 0;
}


static int ram_finalise(nsfb_t *nsfb)
{
    free(nsfb->ptr);

    return 0;
}

static bool ram_input(nsfb_t *nsfb, nsfb_event_t *event, int timeout)
{
    UNUSED(nsfb);
    UNUSED(event);
    UNUSED(timeout);
    return false;
}

const nsfb_surface_rtns_t ram_rtns = {
    .defaults = ram_defaults,
    .initialise = ram_initialise,
    .finalise = ram_finalise,
    .input = ram_input,
    .geometry = ram_set_geometry,
};

NSFB_SURFACE_DEF(ram, NSFB_SURFACE_RAM, &ram_rtns)

/*
 * Local variables:
 *  c-basic-offset: 4
 *  tab-width: 8
 * End:
 */
