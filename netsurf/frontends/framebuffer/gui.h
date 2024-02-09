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

#ifndef NETSURF_FB_GUI_H
#define NETSURF_FB_GUI_H


struct fbtk_widget_s;

typedef struct fb_cursor_s fb_cursor_t;

/* bounding box */
typedef struct nsfb_bbox_s bbox_t;

struct gui_window {
	struct browser_window *bw;

	struct fbtk_widget_s *window;
	struct fbtk_widget_s *back;
	struct fbtk_widget_s *forward;
	struct fbtk_widget_s *history;
	struct fbtk_widget_s *stop;
	struct fbtk_widget_s *reload;
	struct fbtk_widget_s *close;
	struct fbtk_widget_s *url;
	struct fbtk_widget_s *status;
	struct fbtk_widget_s *throbber;
	struct fbtk_widget_s *hscroll;
	struct fbtk_widget_s *vscroll;
	struct fbtk_widget_s *browser;
	struct fbtk_widget_s *toolbar;
	struct fbtk_widget_s *bottom_right;

	int throbber_index;

	struct gui_window *next;
	struct gui_window *prev;
};


extern struct gui_window *window_list;

void gui_resize(struct fbtk_widget_s *root, int width, int height);

#endif /* NETSURF_FB_GUI_H */

/*
 * Local Variables:
 * c-basic-offset:8
 * End:
 */
