/*
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
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

#ifndef NETSURF_BEOS_WINDOW_H
#define NETSURF_BEOS_WINDOW_H 1

#include <View.h>
#include <Window.h>
#include <NetPositive.h>

extern struct gui_window_table *beos_window_table;
extern struct gui_clipboard_table *beos_clipboard_table;

struct gui_window;
struct browser_window;
struct beos_scaffolding;

class NSBrowserFrameView : public BView {
public:
	NSBrowserFrameView(BRect frame, struct gui_window *gui);
	virtual	~NSBrowserFrameView();

	virtual void MessageReceived(BMessage *message);
	virtual void Draw(BRect updateRect);

        //virtual void FrameMoved(BPoint new_location);
	virtual void FrameResized(float new_width, float new_height);

	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage *msg);

private:
	struct gui_window *fGuiWindow;
};

/**
 * Process beos messages into browser operations.
 *
 * \param message The beos message to process.
 */
void nsbeos_dispatch_event(BMessage *message);

/**
 * Cause all windows to be reflowed.
 */
void nsbeos_reflow_all_windows(void);

/**
 * Get containing scaffold of a beos gui window
 *
 * \param g gui window to find scaffold of.
 * \return The containing scaffold.
 */
struct beos_scaffolding *nsbeos_get_scaffold(struct gui_window *g);

struct browser_window *nsbeos_get_browser_for_gui(struct gui_window *g);

int nsbeos_gui_window_update_targets(struct gui_window *g);

void nsbeos_window_destroy_browser(struct gui_window *g);

struct browser_window *nsbeos_get_browser_window(struct gui_window *g);

#endif /* NETSURF_BEOS_WINDOW_H */
