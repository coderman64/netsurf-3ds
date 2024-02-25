/*
 * Copyright 2009 Vincent Sanders <vince@simtec.co.uk>
 *
 * This file is part of libnsfb, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#include <3ds.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include "3ds/os.h"
#include "3ds/services/gsplcd.h"
#include "SDL/SDL_events.h"
#include "SDL/SDL_timer.h"
#include "SDL/SDL_video.h"
#include "libnsfb.h"
#include "libnsfb_event.h"
#include "libnsfb_plot.h"
#include "libnsfb_plot_util.h"

#include "malloc.h"
#include "nsfb.h"
#include "surface.h"
#include "palette.h"
#include "plot.h"
#include "cursor.h"

enum nsfb_key_code_e sdl_nsfb_map[] = {
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_BACKSPACE,
    NSFB_KEY_TAB,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_CLEAR,
    NSFB_KEY_RETURN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_PAUSE,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_ESCAPE,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_SPACE,
    NSFB_KEY_EXCLAIM,
    NSFB_KEY_QUOTEDBL,
    NSFB_KEY_HASH,
    NSFB_KEY_DOLLAR,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_AMPERSAND,
    NSFB_KEY_QUOTE,
    NSFB_KEY_LEFTPAREN,
    NSFB_KEY_RIGHTPAREN,
    NSFB_KEY_ASTERISK,
    NSFB_KEY_PLUS,
    NSFB_KEY_COMMA,
    NSFB_KEY_MINUS,
    NSFB_KEY_PERIOD,
    NSFB_KEY_SLASH,
    NSFB_KEY_0,
    NSFB_KEY_1,
    NSFB_KEY_2,
    NSFB_KEY_3,
    NSFB_KEY_4,
    NSFB_KEY_5,
    NSFB_KEY_6,
    NSFB_KEY_7,
    NSFB_KEY_8,
    NSFB_KEY_9,
    NSFB_KEY_COLON,
    NSFB_KEY_SEMICOLON,
    NSFB_KEY_LESS,
    NSFB_KEY_EQUALS,
    NSFB_KEY_GREATER,
    NSFB_KEY_QUESTION,
    NSFB_KEY_AT,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_LEFTBRACKET,
    NSFB_KEY_BACKSLASH,
    NSFB_KEY_RIGHTBRACKET,
    NSFB_KEY_CARET,
    NSFB_KEY_UNDERSCORE,
    NSFB_KEY_BACKQUOTE,
    NSFB_KEY_a,
    NSFB_KEY_b,
    NSFB_KEY_c,
    NSFB_KEY_d,
    NSFB_KEY_e,
    NSFB_KEY_f,
    NSFB_KEY_g,
    NSFB_KEY_h,
    NSFB_KEY_i,
    NSFB_KEY_j,
    NSFB_KEY_k,
    NSFB_KEY_l,
    NSFB_KEY_m,
    NSFB_KEY_n,
    NSFB_KEY_o,
    NSFB_KEY_p,
    NSFB_KEY_q,
    NSFB_KEY_r,
    NSFB_KEY_s,
    NSFB_KEY_t,
    NSFB_KEY_u,
    NSFB_KEY_v,
    NSFB_KEY_w,
    NSFB_KEY_x,
    NSFB_KEY_y,
    NSFB_KEY_z,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_DELETE,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_KP0,
    NSFB_KEY_KP1,
    NSFB_KEY_KP2,
    NSFB_KEY_KP3,
    NSFB_KEY_KP4,
    NSFB_KEY_KP5,
    NSFB_KEY_KP6,
    NSFB_KEY_KP7,
    NSFB_KEY_KP8,
    NSFB_KEY_KP9,
    NSFB_KEY_KP_PERIOD,
    NSFB_KEY_KP_DIVIDE,
    NSFB_KEY_KP_MULTIPLY,
    NSFB_KEY_KP_MINUS,
    NSFB_KEY_KP_PLUS,
    NSFB_KEY_KP_ENTER,
    NSFB_KEY_KP_EQUALS,
    NSFB_KEY_UP,
    NSFB_KEY_DOWN,
    NSFB_KEY_RIGHT,
    NSFB_KEY_LEFT,
    NSFB_KEY_INSERT,
    NSFB_KEY_HOME,
    NSFB_KEY_END,
    NSFB_KEY_PAGEUP,
    NSFB_KEY_PAGEDOWN,
    NSFB_KEY_F1,
    NSFB_KEY_F2,
    NSFB_KEY_F3,
    NSFB_KEY_F4,
    NSFB_KEY_F5,
    NSFB_KEY_F6,
    NSFB_KEY_F7,
    NSFB_KEY_F8,
    NSFB_KEY_F9,
    NSFB_KEY_F10,
    NSFB_KEY_F11,
    NSFB_KEY_F12,
    NSFB_KEY_F13,
    NSFB_KEY_F14,
    NSFB_KEY_F15,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_UNKNOWN,
    NSFB_KEY_NUMLOCK,
    NSFB_KEY_CAPSLOCK,
    NSFB_KEY_SCROLLOCK,
    NSFB_KEY_RSHIFT,
    NSFB_KEY_LSHIFT,
    NSFB_KEY_RCTRL,
    NSFB_KEY_LCTRL,
    NSFB_KEY_RALT,
    NSFB_KEY_LALT,
    NSFB_KEY_RMETA,
    NSFB_KEY_LMETA,
    NSFB_KEY_LSUPER,
    NSFB_KEY_RSUPER,
    NSFB_KEY_MODE,
    NSFB_KEY_COMPOSE,
    NSFB_KEY_HELP,
    NSFB_KEY_PRINT,
    NSFB_KEY_SYSREQ,
    NSFB_KEY_BREAK,
    NSFB_KEY_MENU,
    NSFB_KEY_POWER,
    NSFB_KEY_EURO,
    NSFB_KEY_UNDO,
};

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))


Uint32 last_update = 0;
nsfb_bbox_t updaterect = {-1,-1,-1,-1};

void SDL_ShouldUpdate(SDL_Surface* sdl_screen,Sint32 x, Sint32 y, Uint32 w, Uint32 h){

    if(updaterect.x0 == -1){
        updaterect.x0 = x;
        updaterect.x1 = x+w;
        updaterect.y0 = y;
        updaterect.y1 = y+h;
    }

    updaterect.x0 = min(updaterect.x0,x);
    updaterect.y0 = min(updaterect.y0,y);
    updaterect.x1 = max(updaterect.x1,(Sint32)(x+w));
    updaterect.y1 = max(updaterect.y1,(Sint32)(y+h));

    if(SDL_GetTicks()-last_update < 16){
        SDL_Delay(max(16-(SDL_GetTicks()-last_update),0));
    }

    if(updaterect.x0 != -1 && (SDL_GetTicks()-last_update > 16 || SDL_GetTicks() < last_update)){
        SDL_UpdateRect(sdl_screen,
                    updaterect.x0,
                    updaterect.y0,
                    updaterect.x1 - updaterect.x0,
                    updaterect.y1 - updaterect.y0);
        updaterect.x0 = -1;
        last_update = SDL_GetTicks();
    }
}


static void
set_palette(nsfb_t *nsfb)
{
    SDL_Surface *sdl_screen = nsfb->surface_priv;
    SDL_Color palette[256];
    int loop = 0;

    /* Get libnsfb palette */
    nsfb_palette_generate_nsfb_8bpp(nsfb->palette);

    /* Create SDL palette from nsfb palette */
    for (loop = 0; loop < 256; loop++) {
        palette[loop].r = (nsfb->palette->data[loop]      ) & 0xFF;
        palette[loop].g = (nsfb->palette->data[loop] >>  8) & 0xFF;
        palette[loop].b = (nsfb->palette->data[loop] >> 16) & 0xFF;
        palette[loop].unused = 0; /* Suppress valgrind uninitialised values */
    }

    /* Set SDL palette */
    SDL_SetColors(sdl_screen, palette, 0, 256);

}

static bool 
sdlcopy(nsfb_t *nsfb, nsfb_bbox_t *srcbox, nsfb_bbox_t *dstbox)
{
    SDL_Rect src;
    SDL_Rect dst;
    SDL_Surface *sdl_screen = nsfb->surface_priv;
    nsfb_bbox_t allbox;
    struct nsfb_cursor_s *cursor = nsfb->cursor;

    nsfb_plot_add_rect(srcbox, dstbox, &allbox);

    /* clear the cursor if its within the region to be altered */
    if ((cursor != NULL) &&
        (cursor->plotted == true) &&
        (nsfb_plot_bbox_intersect(&allbox, &cursor->loc))) {
        nsfb_cursor_clear(nsfb, cursor);
    }

    src.x = srcbox->x0;
    src.y = srcbox->y0;
    src.w = srcbox->x1 - srcbox->x0;
    src.h = srcbox->y1 - srcbox->y0;
 
    dst.x = dstbox->x0;
    dst.y = dstbox->y0;
    dst.w = dstbox->x1 - dstbox->x0;
    dst.h = dstbox->y1 - dstbox->y0;
 
    SDL_BlitSurface(sdl_screen, &src, sdl_screen , &dst);

    if ((cursor != NULL) && 
        (cursor->plotted == false)) {
        nsfb_cursor_plot(nsfb, cursor);
    }


    SDL_ShouldUpdate(sdl_screen, dst.x, dst.y, dst.w, dst.h);
    // SDL_UpdateRect(sdl_screen, dst.x, dst.y, dst.w, dst.h);

    return true;

}

static int sdl_set_geometry(nsfb_t *nsfb, int width, int height,
        enum nsfb_format_e format)
{
    SDL_Surface *sdl_screen;

    nsfb->width = width;
    nsfb->height = height;
    nsfb->format = format;

    /* select default sw plotters for format */
    select_plotters(nsfb);

    nsfb->plotter_fns->copy = sdlcopy;

    if (nsfb->surface_priv != NULL) {
        sdl_screen = SDL_SetVideoMode(nsfb->width,
                                      nsfb->height,
                                      nsfb->bpp,
                                      SDL_SWSURFACE | SDL_RESIZABLE);

        if (sdl_screen == NULL ) {
            fprintf(stderr, "Unable to resize video: %s\n", SDL_GetError());
            return -1;
        }

        nsfb->surface_priv = sdl_screen;
        nsfb->ptr = sdl_screen->pixels;
        nsfb->linelen = sdl_screen->pitch;
    }

    return 0;
}

static u32 *SOC_buffer = NULL;

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

void failExit(const char *fmt);

void socShutdown(void);

static int sdl_initialise(nsfb_t *nsfb)
{
    SDL_Surface *sdl_screen;
    SDL_PixelFormat *sdl_fmt;
    enum nsfb_format_e fmt;

    if (nsfb->surface_priv != NULL)
        return -1;

    /* sanity checked depth. */
    if ((nsfb->bpp != 32) && (nsfb->bpp != 16) && (nsfb->bpp != 8))
        return -1;


    /* initialise SDL library */
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0 ) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return -1;
    }

    sdl_screen = SDL_SetVideoMode(nsfb->width,
                                  nsfb->height,
                                  nsfb->bpp,
                                  SDL_SWSURFACE | SDL_BOTTOMSCR | SDL_CONSOLETOP);


    if (sdl_screen == NULL ) {
        fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
        return -1;
    }

    // initialize 3DS socket service
	// from 3ds socket example:
	// https://github.com/devkitPro/3ds-examples/blob/master/network/sockets/source/sockets.c#L70-L84
	
    int ret;

	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
        fprintf(stderr,"FAILED TO INIT 3DS SOCKET SERVICE!");
	}

	// Now intialise soc:u service
	if (R_FAILED(ret = socInit(SOC_buffer, SOC_BUFFERSIZE))) {
        fprintf(stderr,"FAILED TO INIT 3DS SOCKET SERVICE! socInit");
	}

	// register socShutdown to run at exit
	atexit(socShutdown);

    /* find out what pixel format we got */
    sdl_fmt = sdl_screen->format;

    switch (sdl_fmt->BitsPerPixel) {
    case 32:
        if (sdl_fmt->Rshift == 0)
            fmt = NSFB_FMT_XBGR8888;
        else
            fmt = NSFB_FMT_XRGB8888;
        break;

    default:
        fmt = nsfb->format;
        break;
    }

    /* If we didn't get what we asked for, reselect plotters */
    if (nsfb->format != fmt) {
        nsfb->format = fmt;

        if (sdl_set_geometry(nsfb, nsfb->width, nsfb->height,
                nsfb->format) != 0) {
            return -1;
        }
    }

    nsfb->surface_priv = sdl_screen;

    if (nsfb->bpp == 8) {
        nsfb_palette_new(&nsfb->palette, nsfb->width);
        set_palette(nsfb);
    }

    nsfb->ptr = sdl_screen->pixels;
    nsfb->linelen = sdl_screen->pitch;

    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(300, 50);

    return 0;
}

void socShutdown(void) {
//---------------------------------------------------------------------------------
	printf("waiting for socExit...\n");
	socExit();
}

static int sdl_finalise(nsfb_t *nsfb)
{
    nsfb=nsfb;
    SDL_Quit();
    return 0;
}

static uint32_t wakeeventtimer(uint32_t ival, void *param)
{
    SDL_Event event;
    ival = ival;
    param = param;

    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = 0;
    event.user.data2 = 0;
    
    SDL_PushEvent(&event);

    return 0;
}

static bool sdl_input(nsfb_t *nsfb, nsfb_event_t *event, int timeout)
{
    int got_event;
    SDL_Event sdlevent;

    nsfb = nsfb; /* unused */
    
    if (timeout == 0) {
        SDL_Delay(0);
        got_event = SDL_PollEvent(&sdlevent);
    } else {
        if (timeout > 0) {
            /* setup wake timer to ensure the wait event below exits no later
             * than when the timeout has occoured.
             */
            SDL_TimerID tid;
            tid = SDL_AddTimer(timeout, wakeeventtimer, NULL);
            got_event = SDL_WaitEvent(&sdlevent);
            if ((got_event == 0) || (sdlevent.type != SDL_USEREVENT)) {
                SDL_RemoveTimer(tid);
            }
        } else {
	    got_event = SDL_WaitEvent(&sdlevent);
        }
    }

    u32 k = hidKeysDown();

    if(k & KEY_START){
        event->type = NSFB_EVENT_CONTROL;
        event->value.controlcode = NSFB_CONTROL_QUIT;
        return true;
    }

    /* Do nothing if there was no event */
    if (got_event == 0) {
        return false;
    }

    event->type = NSFB_EVENT_NONE;

    switch (sdlevent.type) {
    case SDL_KEYDOWN:
	event->type = NSFB_EVENT_KEY_DOWN;
	event->value.keycode = sdl_nsfb_map[sdlevent.key.keysym.sym];
	break;

    case SDL_KEYUP:
	event->type = NSFB_EVENT_KEY_UP;
	event->value.keycode = sdl_nsfb_map[sdlevent.key.keysym.sym];
	break;

    case SDL_MOUSEBUTTONDOWN:
	event->type = NSFB_EVENT_KEY_DOWN;

	switch (sdlevent.button.button) {

	case SDL_BUTTON_LEFT:
	    event->value.keycode = NSFB_KEY_MOUSE_1;
	    break;

	case SDL_BUTTON_MIDDLE:
	    event->value.keycode = NSFB_KEY_MOUSE_2;
	    break;

	case SDL_BUTTON_RIGHT:
	    event->value.keycode = NSFB_KEY_MOUSE_3;
	    break;

	case SDL_BUTTON_WHEELUP:
	    event->value.keycode = NSFB_KEY_MOUSE_4;
	    break;

	case SDL_BUTTON_WHEELDOWN:
	    event->value.keycode = NSFB_KEY_MOUSE_5;
	    break;
	}
	break;

    case SDL_MOUSEBUTTONUP:
	event->type = NSFB_EVENT_KEY_UP;

	switch (sdlevent.button.button) {

	case SDL_BUTTON_LEFT:
	    event->value.keycode = NSFB_KEY_MOUSE_1;
	    break;

	case SDL_BUTTON_MIDDLE:
	    event->value.keycode = NSFB_KEY_MOUSE_2;
	    break;

	case SDL_BUTTON_RIGHT:
	    event->value.keycode = NSFB_KEY_MOUSE_3;
	    break;

	case SDL_BUTTON_WHEELUP:
	    event->value.keycode = NSFB_KEY_MOUSE_4;
	    break;

	case SDL_BUTTON_WHEELDOWN:
	    event->value.keycode = NSFB_KEY_MOUSE_5;
	    break;
	}
	break;

    case SDL_MOUSEMOTION:
	event->type = NSFB_EVENT_MOVE_ABSOLUTE;
	event->value.vector.x = sdlevent.motion.x;
	event->value.vector.y = sdlevent.motion.y;
	event->value.vector.z = 0;
	break;

    case SDL_QUIT:
	event->type = NSFB_EVENT_CONTROL;
	event->value.controlcode = NSFB_CONTROL_QUIT;
	break;

    case SDL_USEREVENT:
	event->type = NSFB_EVENT_CONTROL;
	event->value.controlcode = NSFB_CONTROL_TIMEOUT;
	break;

    case SDL_VIDEORESIZE:
	event->type = NSFB_EVENT_RESIZE;
	event->value.resize.w = sdlevent.resize.w;
	event->value.resize.h = sdlevent.resize.h;
	break;
    }

    return true;
}

static int sdl_claim(nsfb_t *nsfb, nsfb_bbox_t *box)
{
    struct nsfb_cursor_s *cursor = nsfb->cursor;

    if ((cursor != NULL) &&
        (cursor->plotted == true) &&
        (nsfb_plot_bbox_intersect(box, &cursor->loc))) {
        nsfb_cursor_clear(nsfb, cursor);
    }
    return 0;
}


static int
sdl_cursor(nsfb_t *nsfb, struct nsfb_cursor_s *cursor)
{
    SDL_Surface *sdl_screen = nsfb->surface_priv;
    nsfb_bbox_t redraw;
    nsfb_bbox_t fbarea;

    if ((cursor != NULL) && (cursor->plotted == true)) {
        nsfb_bbox_t loc_shift = cursor->loc;
        loc_shift.x0 -= cursor->hotspot_x;
        loc_shift.y0 -= cursor->hotspot_y;
        loc_shift.x1 -= cursor->hotspot_x;
        loc_shift.y1 -= cursor->hotspot_y;

        nsfb_plot_add_rect(&cursor->savloc, &loc_shift, &redraw);

        /* screen area */
        fbarea.x0 = 0;
        fbarea.y0 = 0;
        fbarea.x1 = nsfb->width;
        fbarea.y1 = nsfb->height;

        nsfb_plot_clip(&fbarea, &redraw);

        nsfb_cursor_clear(nsfb, cursor);

        nsfb_cursor_plot(nsfb, cursor);
        SDL_ShouldUpdate(sdl_screen,
                    redraw.x0,
                    redraw.y0,
                    redraw.x1 - redraw.x0,
                    redraw.y1 - redraw.y0);


    }
    return true;
}


static int sdl_update(nsfb_t *nsfb, nsfb_bbox_t *box)
{
    SDL_Surface *sdl_screen = nsfb->surface_priv;
    struct nsfb_cursor_s *cursor = nsfb->cursor;

    if ((cursor != NULL) &&
	(cursor->plotted == false)) {
        nsfb_cursor_plot(nsfb, cursor);
    }

    SDL_ShouldUpdate(sdl_screen,
                   box->x0,
                   box->y0,
                   box->x1 - box->x0,
                   box->y1 - box->y0);
    // SDL_UpdateRect(sdl_screen,
    //                box->x0,
    //                box->y0,
    //                box->x1 - box->x0,
    //                box->y1 - box->y0);

    return 0;
}

const nsfb_surface_rtns_t sdl_rtns = {
    .initialise = sdl_initialise,
    .finalise = sdl_finalise,
    .input = sdl_input,
    .claim = sdl_claim,
    .update = sdl_update,
    .cursor = sdl_cursor,
    .geometry = sdl_set_geometry,
};

NSFB_SURFACE_DEF(sdl, NSFB_SURFACE_SDL, &sdl_rtns)

/*
 * Local variables:
 *  c-basic-offset: 4
 *  tab-width: 8
 * End:
 */
