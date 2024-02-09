/*
 * Copyright 2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
 * Amiga core window interface.
 *
 * Provides interface for core renderers to the Amiga Intuition drawable area.
 *
 * This module is an object that must be encapsulated. Client users
 * should embed a struct ami_corewindow at the beginning of their
 * context for this display surface, fill in relevant data and then
 * call ami_corewindow_init()
 *
 * The Amiga core window structure requires the callback for draw, key and
 * mouse operations.
 */

#include "amiga/os3support.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utils/log.h"
#include "utils/utils.h"
#include "utils/messages.h"
#include "utils/utf8.h"
#include "netsurf/keypress.h"
#include "netsurf/mouse.h"
#include "netsurf/plot_style.h"

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layout.h>
#include <proto/utility.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/scroller.h>
#include <gadgets/space.h>
#include <intuition/icclass.h>
#include <reaction/reaction_macros.h>

#include "amiga/corewindow.h"
#include "amiga/drag.h"
#include "amiga/memory.h"
#include "amiga/misc.h"
#include "amiga/object.h"
#include "amiga/schedule.h"
#include "amiga/utf8.h"

static void
ami_cw_scroller_top(struct ami_corewindow *ami_cw,
		    ULONG *restrict x,
		    ULONG *restrict y)
{
	ULONG xs = 0;
	ULONG ys = 0;

	if(ami_cw->scroll_x_visible == true) {
		GetAttr(SCROLLER_Top, ami_cw->objects[GID_CW_HSCROLL], &xs);
	}

	if(ami_cw->scroll_y_visible == true) {
		GetAttr(SCROLLER_Top, ami_cw->objects[GID_CW_VSCROLL], &ys);
	}

	*x = xs;
	*y = ys;
}

static void
ami_cw_window_size(struct ami_corewindow *ami_cw, int *width, int *height)
{
	struct IBox *bbox;

	if(ami_gui_get_space_box((Object *)ami_cw->objects[GID_CW_DRAW], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	*width = bbox->Width;
	*height = bbox->Height;

	ami_gui_free_space_box(bbox);
}


/**
 * Convert co-ordinates relative to space.gadget
 * into document co-ordinates
 *
 * @param ami_cw core window
 * @param x co-ordinate, will be updated to new x co-ordinate
 * @param y co-ordinate, will be updated to new y co-ordinate
 */
static void
ami_cw_coord_amiga_to_ns(struct ami_corewindow *ami_cw, int *restrict x, int *restrict y)
{
	ULONG xs = 0;
	ULONG ys = 0;

	ami_cw_scroller_top(ami_cw, &xs, &ys);

	*x = *x + xs;
	*y = *y + ys;
}

/**
 * check if mouse has moved since position was stored
 * @param ami_cw corewindow
 * @param x current x position
 * @param y current y position
 * @param click true to check since last click, false since last drag (press)
 * @return true if it has, false otherwise
 */
static bool
ami_cw_mouse_moved(struct ami_corewindow *ami_cw, int x, int y, bool click)
{
	int mx, my;

	if(click == true) {
		mx = ami_cw->mouse_x_click;
		my = ami_cw->mouse_y_click;
	} else {
		mx = ami_cw->drag_x_start;
		my = ami_cw->drag_y_start;
	}

	if(abs(x - mx) > 5) return true;
	if(abs(y - my) > 5) return true;
	return false;
}

/* get current mouse position in the draw area, adjusted for scroll.
 * @return true if the mouse was in the draw area and co-ordinates updated
 */
static bool
ami_cw_mouse_pos(struct ami_corewindow *ami_cw, int *restrict x, int *restrict y)
{
	int16 xm, ym;
	ULONG xs, ys;
	struct IBox *bbox;

	xm = ami_cw->win->MouseX;
	ym = ami_cw->win->MouseY;

	if(ami_gui_get_space_box((Object *)ami_cw->objects[GID_CW_DRAW], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return false;
	}

	xm -= bbox->Left;
	ym -= bbox->Top;

	if((xm < 0) || (ym < 0) || (xm > bbox->Width) || (ym > bbox->Height))
		return false;

	ami_gui_free_space_box(bbox);
	ami_cw_scroller_top(ami_cw, &xs, &ys);

	xm += xs;
	ym += ys;
	*x = xm;
	*y = ym;

	return true;
}

/* handle keypress */
static void
ami_cw_key(struct ami_corewindow *ami_cw, int nskey)
{
	ami_cw->key(ami_cw, nskey);

	switch(nskey) {
		case NS_KEY_COPY_SELECTION:
			/* if we've copied a selection we need to clear it - style guide rules */
			ami_cw->key(ami_cw, NS_KEY_CLEAR_SELECTION);
		break;

		/* we may need to deal with scroll-related keys here */
	}
}


/**
 * Redraw functions
 *
 * This is slightly over-engineered as it was taken from the main
 * browser/old tree redraws and supports deferred drawing of
 * rectangles and tiling
 */

/**
 * Redraw an area of a core window
 *
 * \param ami_cw An Amiga core window structure
 * \param r rect (in document co-ordinates)
 */

static void
ami_cw_redraw_rect(struct ami_corewindow *ami_cw, struct rect *r)
{
	struct IBox *bbox;
	ULONG pos_x, pos_y;
	struct rect draw_rect;
	int tile_size_x;
	int tile_size_y;
	int tile_x, tile_y, tile_w, tile_h;
	int x = r->x0;
	int y = r->y0;
	int width = r->x1 - r->x0;
	int height = r->y1 - r->y0;

	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &amiplot,
		.priv = ami_cw->gg
	};

	if(ami_gui_get_space_box((Object *)ami_cw->objects[GID_CW_DRAW], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	ami_cw_scroller_top(ami_cw, &pos_x, &pos_y);

	if(x - (LONG)pos_x + width > bbox->Width) width = bbox->Width - (x - pos_x);
	if(y - (LONG)pos_y + height > bbox->Height) height = bbox->Height - (y - pos_y);

	if(x < (LONG)pos_x) {
		width -= pos_x - x;
		x = pos_x;
	}

	if(y < (LONG)pos_y) {
		height -= pos_y - y;
		y = pos_y;
	}

	ami_plot_ra_get_size(ami_cw->gg, &tile_size_x, &tile_size_y);

	for(tile_y = y; tile_y < (y + height); tile_y += tile_size_y) {
		tile_h = tile_size_y;
		if(((y + height) - tile_y) < tile_size_y)
			tile_h = (y + height) - tile_y;

		for(tile_x = x; tile_x < (x + width); tile_x += tile_size_x) {
			tile_w = tile_size_x;
			if(((x + width) - tile_x) < tile_size_x)
				tile_w = (x + width) - tile_x;

			draw_rect.x0 = tile_x; // was -
			draw_rect.y0 = tile_y; // was -
			draw_rect.x1 = tile_x + tile_w;
			draw_rect.y1 = tile_y + tile_h;

			ami_cw->draw(ami_cw, -tile_x, -tile_y, &draw_rect, &ctx);

#ifdef __amigaos4__
			BltBitMapTags(BLITA_SrcType, BLITT_BITMAP, 
					BLITA_Source, ami_plot_ra_get_bitmap(ami_cw->gg),
					BLITA_SrcX, 0,
					BLITA_SrcY, 0,
					BLITA_DestType, BLITT_RASTPORT, 
					BLITA_Dest, ami_cw->win->RPort,
					BLITA_DestX, bbox->Left + tile_x - pos_x,
					BLITA_DestY, bbox->Top + tile_y - pos_y,
					BLITA_Width, tile_w,
					BLITA_Height, tile_h,
					TAG_DONE);
#else
			BltBitMapRastPort(ami_plot_ra_get_bitmap(ami_cw->gg), 0, 0,
					ami_cw->win->RPort, bbox->Left + tile_x - pos_x, bbox->Top + tile_y - pos_y,
					tile_w, tile_h, 0xC0);
#endif
		}
	}

	ami_gui_free_space_box(bbox);
	ami_clearclipreg(ami_cw->gg);
}


/**
 * Draw the deferred rectangles
 *
 * \param ami_cw An Amiga core window structure to queue redraw
 * \param draw set to false to just delete the queue
 */
static void ami_cw_redraw_queue(struct ami_corewindow *ami_cw, bool draw)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct rect *rect;
	
	if(IsMinListEmpty(ami_cw->deferred_rects)) return;

	if(draw == false) {
		NSLOG(netsurf, INFO, "Ignoring deferred box redraw queue");
	} // else should probably show busy pointer

	node = (struct nsObject *)GetHead((struct List *)ami_cw->deferred_rects);

	do {
		if(draw == true) {
			rect = (struct rect *)node->objstruct;
			ami_cw_redraw_rect(ami_cw, rect);
		}
		nnode = (struct nsObject *)GetSucc((struct Node *)node);
		ami_memory_itempool_free(ami_cw->deferred_rects_pool, node->objstruct, sizeof(struct rect));
		DelObjectNoFree(node);
	} while((node = nnode));
}

static void
ami_cw_redraw_cb(void *p)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)p;

	ami_cw_redraw_queue(ami_cw, true);
}

/**
 * Queue a redraw of a rectangle
 *
 * @param ami_cw the core window to redraw
 * @param r the rectangle (in doc coords) to redraw, or NULL for full window
 */

static void
ami_cw_redraw(struct ami_corewindow *ami_cw, const struct rect *restrict r)
{
	struct nsObject *nsobj;
	struct rect *restrict deferred_rect;
	struct rect new_rect;

	if(r == NULL) {
		struct IBox *bbox;
		if(ami_gui_get_space_box((Object *)ami_cw->objects[GID_CW_DRAW], &bbox) != NSERROR_OK) {
			amiga_warn_user("NoMemory", "");
			return;
		}

		new_rect.x0 = 0;
		new_rect.y0 = 0;
		ami_cw_coord_amiga_to_ns(ami_cw, &new_rect.x0, &new_rect.y0);
		new_rect.x1 = new_rect.x0 + bbox->Width;
		new_rect.y1 = new_rect.y0 + bbox->Height;

		ami_gui_free_space_box(bbox);

		r = &new_rect;
	}

	if(ami_gui_window_update_box_deferred_check(ami_cw->deferred_rects, r,
			ami_cw->deferred_rects_pool)) {
		deferred_rect = ami_memory_itempool_alloc(ami_cw->deferred_rects_pool, sizeof(struct rect));
		CopyMem(r, deferred_rect, sizeof(struct rect));
		nsobj = AddObject(ami_cw->deferred_rects, AMINS_RECT);
		nsobj->objstruct = deferred_rect;
	} else {
		NSLOG(netsurf, INFO,
		      "Ignoring duplicate or subset of queued box redraw");
	}
	ami_schedule(1, ami_cw_redraw_cb, ami_cw);
}

static void ami_cw_simplerefresh(struct ami_corewindow *ami_cw)
{
	struct rect r;
	struct RegionRectangle *regrect;

	BeginRefresh(ami_cw->win);

	r.x0 = ami_cw->win->RPort->Layer->DamageList->bounds.MinX;
	r.x1 = ami_cw->win->RPort->Layer->DamageList->bounds.MaxX;
	r.y0 = ami_cw->win->RPort->Layer->DamageList->bounds.MinY;
	r.y1 = ami_cw->win->RPort->Layer->DamageList->bounds.MaxY;

	ami_cw_coord_amiga_to_ns(ami_cw, &r.x0, &r.y0);
	ami_cw_coord_amiga_to_ns(ami_cw, &r.y0, &r.y1);

	regrect = ami_cw->win->RPort->Layer->DamageList->RegionRectangle;

	ami_cw_redraw(ami_cw, &r); /* queue redraw */

	while(regrect) {
		r.x0 = regrect->bounds.MinX;
		r.x1 = regrect->bounds.MaxX;
		r.y0 = regrect->bounds.MinY;
		r.y1 = regrect->bounds.MaxY;
		ami_cw_coord_amiga_to_ns(ami_cw, &r.x0, &r.y0);
		ami_cw_coord_amiga_to_ns(ami_cw, &r.y0, &r.y1);

		regrect = regrect->Next;

		ami_cw_redraw(ami_cw, &r); /* queue redraw */
	}

	EndRefresh(ami_cw->win, TRUE);
}

static void
ami_cw_toggle_scrollbar(struct ami_corewindow *ami_cw, bool vert, bool visible)
{
	Object *scroller;
	Object *layout;
	ULONG tag;

	if(vert == true) {
		if(visible == ami_cw->scroll_y_visible) {
			return;
		} else {
			scroller = ami_cw->objects[GID_CW_VSCROLL];
			layout = ami_cw->objects[GID_CW_VSCROLLLAYOUT];
			tag = WINDOW_VertProp;
			ami_cw->scroll_y_visible = visible;
		}
	} else {
		if(visible == ami_cw->scroll_x_visible) {
			return;
		} else {
			scroller = ami_cw->objects[GID_CW_HSCROLL];
			layout = ami_cw->objects[GID_CW_HSCROLLLAYOUT];
			tag = WINDOW_HorizProp;
			ami_cw->scroll_x_visible = visible;
		}
	}

	if(visible == true) {
		if(ami_cw->in_border_scroll == true) {
			SetAttrs(ami_cw->objects[GID_CW_WIN],
				tag, 1,
				TAG_DONE);
		} else {
#ifdef __amigaos4__
			IDoMethod(layout, LM_ADDCHILD, ami_cw->win, scroller, NULL);
#else
			SetAttrs(layout, LAYOUT_AddChild, scroller, TAG_DONE);
#endif
		}
	} else {
		if(ami_cw->in_border_scroll == true) {
			SetAttrs(ami_cw->objects[GID_CW_WIN],
				tag, -1,
				TAG_DONE);
		} else {
#ifdef __amigaos4__
			IDoMethod(layout, LM_REMOVECHILD, ami_cw->win, scroller);
#else
			SetAttrs(layout, LAYOUT_RemoveChild, scroller, TAG_DONE);
#endif
		}
	}

	if(ami_cw->in_border_scroll == false) {
		FlushLayoutDomainCache((struct Gadget *)ami_cw->objects[GID_CW_WIN]);
		RethinkLayout((struct Gadget *)ami_cw->objects[GID_CW_WIN],
					ami_cw->win, NULL, TRUE);
	}

	/* probably need to redraw here */
	ami_cw_redraw(ami_cw, NULL);
}

static void
ami_cw_close(void *w)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)w;

	ami_cw->close(ami_cw);
}

HOOKF(void, ami_cw_idcmp_hook, Object *, object, struct IntuiMessage *) 
{
	struct ami_corewindow *ami_cw = hook->h_Data;
	struct IntuiWheelData *wheel;
	ULONG gid = GetTagData( GA_ID, 0, msg->IAddress ); 

	switch(msg->Class)
	{
		case IDCMP_IDCMPUPDATE:
			switch(gid) 
			{
 				case GID_CW_HSCROLL: 
 				case GID_CW_VSCROLL:
					ami_cw_redraw(ami_cw, NULL);
 				break; 
			} 
		break;
#ifdef __amigaos4__
		case IDCMP_EXTENDEDMOUSE:
			if(msg->Code == IMSGCODE_INTUIWHEELDATA)
			{
				wheel = (struct IntuiWheelData *)msg->IAddress;

				//ami_tree_scroll(twin, (wheel->WheelX * 20), (wheel->WheelY * 20));
			}
		break;
#endif

		case IDCMP_SIZEVERIFY:
		break;

		case IDCMP_REFRESHWINDOW:
			ami_cw_simplerefresh(ami_cw);
		break;

		default:
			NSLOG(netsurf, INFO,
			      "IDCMP hook unhandled event: %ld", msg->Class);
		break;
	}
} 

/**
 * Drag start
 */
static void
ami_cw_drag_start(struct ami_corewindow *ami_cw, int x, int y)
{
	if(ami_cw->dragging == true) return;

	ami_cw->dragging = true;
	ami_cw->drag_x_start = x;
	ami_cw->drag_y_start = y;

	switch(ami_cw->drag_status) {
		case CORE_WINDOW_DRAG_SELECTION:
		break;

		case CORE_WINDOW_DRAG_MOVE:
			ami_drag_icon_show(ami_cw->win, "project");
		break;

		default:
		break;
	}
}

/**
 * Drag progress
 */
static void
ami_cw_drag_progress(struct ami_corewindow *ami_cw, int x, int y)
{
	if(ami_cw->dragging == false) return;

	switch(ami_cw->drag_status) {
		case CORE_WINDOW_DRAG_SELECTION:
		break;

		case CORE_WINDOW_DRAG_MOVE:
			ami_drag_icon_move();
		break;

		default:
		break;
	}
}

/**
 * Drag end
 */
static void
ami_cw_drag_end(struct ami_corewindow *ami_cw, int x, int y)
{
	if(ami_cw->dragging == false) return;
	struct Screen *scrn = ami_gui_get_screen();

	switch(ami_cw->drag_status) {
		case CORE_WINDOW_DRAG_SELECTION:
		break;

		case CORE_WINDOW_DRAG_MOVE:
			ami_drag_icon_close(ami_cw->win);
			if((ami_cw != ami_window_at_pointer(AMINS_COREWINDOW)) && (ami_cw->drag_end != NULL)) {
				ami_cw->drag_end(ami_cw, scrn->MouseX, scrn->MouseY);
			}
		break;

		default:
		break;
	}

	ami_cw->drag_status = CORE_WINDOW_DRAG_NONE;
	ami_cw->dragging = false;
}

/**
 * User has resized window
 */

static void
ami_cw_newsize(struct ami_corewindow *ami_cw)
{
	int win_w, win_h;
	ami_cw_window_size(ami_cw, &win_w, &win_h);

	if(ami_cw->objects[GID_CW_HSCROLL] != NULL) {
		RefreshSetGadgetAttrs((struct Gadget *)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
			SCROLLER_Visible, win_w,
		TAG_DONE);
	}	

	if(ami_cw->objects[GID_CW_VSCROLL] != NULL) {
		RefreshSetGadgetAttrs((struct Gadget *)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
			SCROLLER_Visible, win_h,
		TAG_DONE);
	}

	ami_cw_redraw(ami_cw, NULL);
}

/**
 * Main event loop for our core window
 *
 * \return TRUE if window destroyed
 */
static BOOL
ami_cw_event(void *w)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)w;

	ULONG result;
	ULONG storage;
	uint16 code;
	struct InputEvent *ie;
	int nskey;
	int key_state = 0;
	struct timeval curtime;
	int x = 0, y = 0;

	while((result = RA_HandleInput(ami_cw->objects[GID_CW_WIN], &code)) != WMHI_LASTMSG) {
		if(ami_cw->close_window == true) {
			ami_cw_close(ami_cw);
			return TRUE;
		}

		switch(result & WMHI_CLASSMASK) {
			case WMHI_MOUSEMOVE:
				if(ami_cw_mouse_pos(ami_cw, &x, &y) == true) {
					if(ami_cw_mouse_moved(ami_cw, x, y, false)) {
						if(ami_cw->mouse_state & BROWSER_MOUSE_PRESS_1) {
							/* Start button 1 drag */
							ami_cw->mouse(ami_cw, BROWSER_MOUSE_DRAG_1, x, y);
							/* Replace PRESS with HOLDING and declare drag in progress */
							ami_cw->mouse_state = BROWSER_MOUSE_HOLDING_1 | BROWSER_MOUSE_DRAG_ON;
						} else if(ami_cw->mouse_state & BROWSER_MOUSE_PRESS_2) {
							/* Start button 2 drag */
							ami_cw->mouse(ami_cw, BROWSER_MOUSE_DRAG_2, x, y);
							/* Replace PRESS with HOLDING and declare drag in progress */
							ami_cw->mouse_state = BROWSER_MOUSE_HOLDING_2 | BROWSER_MOUSE_DRAG_ON;
						}
						key_state = ami_gui_get_quals(ami_cw->objects[GID_CW_WIN]);
						ami_cw->mouse(ami_cw, ami_cw->mouse_state | key_state, x, y);
						if(ami_cw->mouse_state & BROWSER_MOUSE_DRAG_ON) {
							ami_cw_drag_start(ami_cw, x, y);
						}
					} else {
						key_state = ami_gui_get_quals(ami_cw->objects[GID_CW_WIN]);
						ami_cw->mouse(ami_cw, ami_cw->mouse_state | key_state, x, y);
					}
				}
				ami_cw_drag_progress(ami_cw, x, y);
			break;

			case WMHI_MOUSEBUTTONS:
				if(ami_cw_mouse_pos(ami_cw, &x, &y) == true) {
					key_state = ami_gui_get_quals(ami_cw->objects[GID_CW_WIN]);
					switch(code) {
						case SELECTDOWN:
							ami_cw->mouse_state = BROWSER_MOUSE_PRESS_1;
							ami_cw->drag_x_start = x;
							ami_cw->drag_y_start = y;
						break;

						case MIDDLEDOWN:
							ami_cw->mouse_state = BROWSER_MOUSE_PRESS_2;
							ami_cw->drag_x_start = x;
							ami_cw->drag_y_start = y;
						break;

						case SELECTUP:
							if(ami_cw->mouse_state & BROWSER_MOUSE_PRESS_1) {
								CurrentTime((ULONG *)&curtime.tv_sec, (ULONG *)&curtime.tv_usec);

								ami_cw->mouse_state = BROWSER_MOUSE_CLICK_1;

								if(ami_cw->lastclick.tv_sec) {
									if((ami_cw_mouse_moved(ami_cw, x, y, true) == false) &&
											(DoubleClick(ami_cw->lastclick.tv_sec,
												ami_cw->lastclick.tv_usec,
												curtime.tv_sec, curtime.tv_usec)))
										ami_cw->mouse_state |= BROWSER_MOUSE_DOUBLE_CLICK;
								}

								ami_cw->mouse_x_click = x;
								ami_cw->mouse_y_click = y;

								if(ami_cw->mouse_state & BROWSER_MOUSE_DOUBLE_CLICK) {
									ami_cw->lastclick.tv_sec = 0;
									ami_cw->lastclick.tv_usec = 0;
								} else {
									ami_cw->lastclick.tv_sec = curtime.tv_sec;
									ami_cw->lastclick.tv_usec = curtime.tv_usec;
								}
							}

							ami_cw->mouse(ami_cw, ami_cw->mouse_state | key_state, x, y);
							ami_cw->mouse_state = BROWSER_MOUSE_HOVER;
						break;

						case MIDDLEUP:
							if(ami_cw->mouse_state & BROWSER_MOUSE_PRESS_2)
								ami_cw->mouse_state = BROWSER_MOUSE_CLICK_2;

							ami_cw->mouse(ami_cw, ami_cw->mouse_state | key_state, x, y);
							ami_cw->mouse_state = BROWSER_MOUSE_HOVER;
						break;
					}

					if(ami_cw->mouse_state == BROWSER_MOUSE_HOVER) {
						ami_cw_drag_end(ami_cw, x, y);
					}

					ami_cw->mouse(ami_cw, ami_cw->mouse_state | key_state, x, y);
				} else {
					/* event is happening away from our corewindow area */
					switch(code) {
						case SELECTUP:
						case MIDDLEUP:
							ami_cw->mouse_state = BROWSER_MOUSE_HOVER;
						break;

						default:
						break;
					}

					if(ami_cw->mouse_state == BROWSER_MOUSE_HOVER) {
						ami_cw_drag_end(ami_cw, x, y);
						ami_cw->mouse(ami_cw, ami_cw->mouse_state | key_state,
							ami_cw->drag_x_start, ami_cw->drag_y_start); // placate core
					}
				}
			break;

			case WMHI_RAWKEY:
				storage = result & WMHI_GADGETMASK;

				GetAttr(WINDOW_InputEvent, ami_cw->objects[GID_CW_WIN], (ULONG *)&ie);
				nskey = ami_key_to_nskey(storage, ie);

				ami_cw_key(ami_cw, nskey);
			break;

			case WMHI_NEWSIZE:
				ami_cw_newsize(ami_cw);
			break;

			case WMHI_CLOSEWINDOW:
				ami_cw_close(ami_cw);
				return TRUE;
			break;

			default:
				/* pass the event to the window owner */
				if(ami_cw->event != NULL)
					if(ami_cw->event(ami_cw, result) == TRUE) {
						return TRUE;
					}
			break;
		}
	};

	return FALSE;
}

static const struct ami_win_event_table ami_cw_table = {
	ami_cw_event,
	ami_cw_close,
};


/**
 * callback from core to request an invalidation of a amiga core window area.
 *
 * The specified area of the window should now be considered
 *  out of date. If the area is NULL the entire window must be
 *  invalidated.
 *
 * \param[in] cw The core window to invalidate.
 * \param[in] r area to redraw or NULL for the entire window area.
 * \return NSERROR_OK on success or appropriate error code.
 */
static nserror
ami_cw_invalidate_area(struct core_window *cw, const struct rect *r)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;

	ami_cw_redraw(ami_cw, r);

	return NSERROR_OK;
}


static nserror
ami_cw_get_window_dimensions(const struct core_window *cw,
		int *width, int *height)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;

	ami_cw_window_size(ami_cw, width, height);
	return NSERROR_OK;
}


static nserror
ami_cw_update_size(struct core_window *cw, int width, int height)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;
	int win_w, win_h;

	ami_cw_window_size(ami_cw, &win_w, &win_h);

	if(width == -1) {
		ami_cw_toggle_scrollbar(ami_cw, false, false);
	} else {
		ami_cw_toggle_scrollbar(ami_cw, false, true);
		RefreshSetGadgetAttrs((struct Gadget *)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
			SCROLLER_Total, (ULONG)width,
			SCROLLER_Visible, win_w,
		TAG_DONE);		
	}

	if(height == -1) {
		ami_cw_toggle_scrollbar(ami_cw, true, false);
	} else {
		ami_cw_toggle_scrollbar(ami_cw, true, true);
		RefreshSetGadgetAttrs((struct Gadget *)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
			SCROLLER_Total, height,
			SCROLLER_Visible, win_h,
		TAG_DONE);		
	}
	return NSERROR_OK;
}


static nserror
ami_cw_get_scroll(const struct core_window *cw, int *x, int *y)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;
	ULONG win_x0, win_y0;

	ami_cw_scroller_top(ami_cw, &win_x0, &win_y0);

	*x = win_x0;
	*y = win_y0;
	return NSERROR_OK;
}


static nserror
ami_cw_set_scroll(struct core_window *cw, int x, int y)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;

	if(ami_cw->scroll_y_visible == true) {
		RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
				SCROLLER_Top, y,
				TAG_DONE);
	}

	if(ami_cw->scroll_x_visible == true) {
		RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
				SCROLLER_Top, x,
				TAG_DONE);
	}

	/* probably need to redraw here */
	ami_cw_redraw(ami_cw, NULL);
	return NSERROR_OK;
}


static nserror
ami_cw_drag_status(struct core_window *cw, core_window_drag_status ds)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)cw;
	ami_cw->drag_status = ds;
	return NSERROR_OK;
}


struct core_window_callback_table ami_cw_cb_table = {
        .invalidate = ami_cw_invalidate_area,
        .update_size = ami_cw_update_size,
        .set_scroll = ami_cw_set_scroll,
        .get_scroll = ami_cw_get_scroll,
        .get_window_dimensions = ami_cw_get_window_dimensions,
        .drag_status = ami_cw_drag_status
};

/* exported function documented example/corewindow.h */
nserror ami_corewindow_init(struct ami_corewindow *ami_cw)
{
	/* setup the core window callback table */
	ami_cw->cb_table = &ami_cw_cb_table;
	ami_cw->drag_status = CORE_WINDOW_DRAG_NONE;

	/* clear some vars */
	ami_cw->mouse_state = BROWSER_MOUSE_HOVER;
	ami_cw->lastclick.tv_sec = 0;
	ami_cw->lastclick.tv_usec = 0;
	ami_cw->scroll_x_visible = true;
	ami_cw->scroll_y_visible = true;
	ami_cw->in_border_scroll = false;
	ami_cw->dragging = false;

	/* allocate drawing area etc */
	ami_cw->gg = ami_plot_ra_alloc(100, 100, false, true); // force tiles to save memory

	ami_cw->deferred_rects = NewObjList();
	ami_cw->deferred_rects_pool = ami_memory_itempool_create(sizeof(struct rect));

	/* add the core window to our window list so we process events */
	ami_gui_win_list_add(ami_cw, AMINS_COREWINDOW, &ami_cw_table);

	/* set up the IDCMP hook for event processing (extended mouse, scrollbars) */
	ami_cw->idcmp_hook.h_Entry = (void *)ami_cw_idcmp_hook;
	ami_cw->idcmp_hook.h_Data = ami_cw;

	/* open the window */
	ami_cw->win = (struct Window *)RA_OpenWindow(ami_cw->objects[GID_CW_WIN]);

	/* attach the scrollbars for event processing _if they are in the window border_ */
	if(ami_cw->objects[GID_CW_HSCROLL] == NULL) {
		GetAttr(WINDOW_HorizObject, ami_cw->objects[GID_CW_WIN],
					(ULONG *)&ami_cw->objects[GID_CW_HSCROLL]);

		RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_HSCROLL], ami_cw->win, NULL,
			GA_ID, GID_CW_HSCROLL,
			ICA_TARGET, ICTARGET_IDCMP,
			TAG_DONE);

		ami_cw->in_border_scroll = true;
	}

	if(ami_cw->objects[GID_CW_VSCROLL] == NULL) {
		GetAttr(WINDOW_VertObject, ami_cw->objects[GID_CW_WIN],
					(ULONG *)&ami_cw->objects[GID_CW_VSCROLL]);

		RefreshSetGadgetAttrs((APTR)ami_cw->objects[GID_CW_VSCROLL], ami_cw->win, NULL,
			GA_ID, GID_CW_VSCROLL,
			ICA_TARGET, ICTARGET_IDCMP,
			TAG_DONE);

		ami_cw->in_border_scroll = true;
	}

	return NSERROR_OK;
}

/* exported interface documented in example/corewindow.h */
nserror ami_corewindow_fini(struct ami_corewindow *ami_cw)
{
	/* remove any pending redraws */
	ami_schedule(-1, ami_cw_redraw_cb, ami_cw);
	FreeObjList(ami_cw->deferred_rects);
	ami_memory_itempool_delete(ami_cw->deferred_rects_pool);

	/* destroy the window */
	ami_cw->win = NULL;
	DisposeObject(ami_cw->objects[GID_CW_WIN]);

#if 0
	/* ensure our scrollbars are destroyed */
	/* it appears these are disposed anyway,
	 * even if the gadgets are no longer attached to the window */
	if(ami_cw->in_border_scroll == false) {
		if(ami_cw->scroll_x_visible == false) {
			DisposeObject(ami_cw->objects[GID_CW_HSCROLL]);
		}
		if(ami_cw->scroll_y_visible == false) {
			DisposeObject(ami_cw->objects[GID_CW_VSCROLL]);
		}
	}
#endif

	/* release off-screen bitmap stuff */
	ami_plot_ra_free(ami_cw->gg);

	/* free the window title */
	ami_utf8_free(ami_cw->wintitle);

	/* remove the core window from our window list */
	ami_gui_win_list_remove(ami_cw);

	return NSERROR_OK;
}

