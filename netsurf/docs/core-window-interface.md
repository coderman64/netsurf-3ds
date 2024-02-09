Core Window Interface
=====================

The NetSurf core provides an optional API to frontend implementations
which allows a number of "standard" window content interfaces to be
provided.

The currently available user interfaces are:

 - Cookies
 - Global history
 - Hotlist
 - SSL certificate view (obsolete, but used as an example here)
 - local history

Although not currently included in future additional user interfaces
will be available for the main browser render.

To be clear these are generic implementations of this functionality
that any frontend may use. Frontends are free to implement these
interfaces in any manner as they see fit, the corewindow interface
simply provides a default.

core window API
---------------

The API is fairly simple and simply involves passing a callback table
and context pointer to the interface element being constructed.

The header that defines the callback interface is netsurf/core_window.h 

The callback table contains five function pointer interfaces which the
frontend must implement for the core.

 - invalidate
      invalidate an area of a window

 - update_size
      Update the limits of the window 

 - scroll_visible
      Scroll the window to make area visible

 - get_window_dimensions
      Get window viewport dimensions

 - drag_status
      Inform corewindow owner of drag status

Each callback will be passed the context pointer for the corewindow
instance and the relevant additional information necessary to perform
the operation.

Each exported user interface element wraps this generic interface with
a concrete implementation. For example the SSL certificate viewer is
initialised with:

    nserror sslcert_viewer_init(struct core_window_callback_table *cw_t,
                                void *core_window_handle,
                                struct sslcert_session_data *ssl_d);

This call creates a context which will display and navigate the ssl
session data passed. The frontend must service the callbacks from the
core to provide the necessary interactions with the frontend windowing
system.

These actions should ideally use the standard frontend window
processing. So for the GTK frontend when the core calls the invalidate
operation it simply marks the area passed as damaged (using
gtk_widget_queue_draw_area()) and lets the standard expose event cause
the redraw to occur.

If the frontend needs to redraw an area of a window (perhaps an expose
event occurred or the window has had an area marked as invalid) it
must call the core window API wrappers implementation which will
perform the plot operations required to update an area of the window.

e.g in the case of ssl certificate viewer

    void sslcert_viewer_redraw(struct sslcert_session_data *ssl_d,
                               int x, int y, struct rect *clip,
                               const struct redraw_context *ctx);

would perform the plot operations for that SSL data window.

Usage
-----

The usage pattern that is expected is for a frontend to create a core
window implementation that implements the necessary five API in a
generic way and allows the frontend to provide the specific
specialisation for each of the user interface elements it wishes to
use (cookies, SSL viewer etc).

The GTK frontend for example:

has source corewindow.[ch] which implement the five core callbacks
using generic GTK operations (invalidate calls
gtk_widget_queue_draw_area() etc.) and then provides additional
operations on a GTK drawing area object to attach expose event
processing, keypress processing etc.

The GTK corewindow (not to be confused with the core window API
itself, this is purely the gtk wrapper) is used by ssl_cert.c which
creates a nsgtk_crtvrfy_window structure containing the
nsgtk_corewindow structure. It attaches actual GTK window handles to
this structure and populates elements of nsgtk_corewindow and then
calls sslcert_viewer_init() directly.

frontend skeleton
-----------------

An example core window implementation for a frontend ssl certificate
viewer is presented here. This implements the suggested usage above
and provides generic corewindow helpers.


frontends/example/corewindow.h
------------------------------

    /*
     * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
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
    
    #ifndef EXAMPLE_COREWINDOW_H
    #define EXAMPLE_COREWINDOW_H
    
    #include "netsurf/core_window.h"
    
    /**
     * example core window state
     */
    struct example_corewindow {
    
    
            /*
             * Any variables common to any frontend window would go here.
             * e.g. drawing area handles, toolkit pointers or other state
             */
            example_toolkit_widget *tk_widget;
    
    
    
            /** drag status set by core */
            core_window_drag_status drag_status;
    
            /** table of callbacks for core window operations */
            struct core_window_callback_table *cb_table;
    
            /**
             * callback to draw on drawable area of example core window
             *
             * \param example_cw The example core window structure.
             * \param r The rectangle of the window that needs updating.
             * \return NSERROR_OK on success otherwise apropriate error code
             */
            nserror (*draw)(struct example_corewindow *example_cw, struct rect *r);
    
            /**
             * callback for keypress on example core window
             *
             * \param example_cw The example core window structure.
             * \param nskey The netsurf key code.
             * \return NSERROR_OK if key processed,
             *         NSERROR_NOT_IMPLEMENTED if key not processed
             *         otherwise apropriate error code
             */
            nserror (*key)(struct example_corewindow *example_cw, uint32_t nskey);
    
            /**
             * callback for mouse event on example core window
             *
             * \param example_cw The example core window structure.
             * \param mouse_state mouse state
             * \param x location of event
             * \param y location of event
             * \return NSERROR_OK on sucess otherwise apropriate error code.
             */
            nserror (*mouse)(struct example_corewindow *example_cw, browser_mouse_state mouse_state, int x, int y);
    };
    
    /**
     * initialise elements of example core window.
     *
     * As a pre-requisite the draw, key and mouse callbacks must be defined
     *
     * \param example_cw A example core window structure to initialise
     * \return NSERROR_OK on successful initialisation otherwise error code.
     */
    nserror example_corewindow_init(struct example_corewindow *example_cw);
    
    /**
     * finalise elements of example core window.
     *
     * \param example_cw A example core window structure to initialise
     * \return NSERROR_OK on successful finalisation otherwise error code.
     */
    nserror example_corewindow_fini(struct example_corewindow *example_cw);
    
    #endif

frontends/example/corewindow.c
------------------------------

    /*
     * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
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
     * file
     * EXAMPLE generic core window interface.
     *
     * Provides interface for core renderers to the example toolkit drawable area.
     *
     * This module is an object that must be encapsulated. Client users
     * should embed a struct example_corewindow at the beginning of their
     * context for this display surface, fill in relevant data and then
     * call example_corewindow_init()
     *
     * The example core window structure requires the callback for draw, key and
     * mouse operations.
     */
    
    #include <assert.h>
    #include <string.h>
    #include <math.h>
    
    #include "utils/log.h"
    #include "utils/utils.h"
    #include "utils/messages.h"
    #include "utils/utf8.h"
    #include "netsurf/keypress.h"
    #include "netsurf/mouse.h"
    #include "netsurf/plot_style.h"
    
    /* extremely likely there will be additional headers required in a real frontend */
    #include "example/corewindow.h"
    
    
    /* toolkit event handlers that do generic things and call internal callbacks */
    
    
    static bool
    example_cw_mouse_press_event(toolkit_widget *widget, toolkit_button bt, int x, int y, void *ctx)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)ctx;
    
            example_cw->mouse(example_cw, state, x, y);
    
            return true;
    }
    
    static bool
    example_cw_keyrelease_event(toolkit_widget *widget, void *ctx)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)ctx;
    
            example_cw->key(example_cw, keycode);
    
            return true;
    }
    
    
    
    /* signal handler for toolkit window redraw */
    static bool
    example_cw_draw_event(toolkit_widget *widget,
                          toolkit_area *tk_area,
                          void *ctx)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)ctx;
            struct rect clip;
    
            clip.x0 = tk_area.x;
            clip.y0 = tk_area.y;
            clip.x1 = tk_area.width;
            clip.y1 = tk_area.height;
    
            example_cw->draw(example_cw, &clip);
    
            return true;
    }
    
    
    /**
     * callback from core to request a redraw
     */
    static nserror
    example_cw_invalidate(struct core_window *cw, const struct rect *r)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)cw;
    
            toolkit_widget_queue_draw_area(example_cw->widget,
                                           r->x0, r->y0,
                                           r->x1 - r->x0, r->y1 - r->y0);
    }
    
    
    static void
    example_cw_update_size(struct core_window *cw, int width, int height)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)cw;
    
            toolkit_widget_set_size_request(EXAMPLE_WIDGET(example_cw->drawing_area),
                                        width, height);
    }
    
    
    static void
    example_cw_scroll_visible(struct core_window *cw, const struct rect *r)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)cw;
    
            toolkit_scroll_widget(example_cw->widget, r);
    }
    
    
    static void
    example_cw_get_window_dimensions(struct core_window *cw, int *width, int *height)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)cw;
    
            *width = toolkit_get_widget_width(example_cw->widget);
            *height = toolkit_get_widget_height(example_cw->widget);
    }
    
    
    static void
    example_cw_drag_status(struct core_window *cw, core_window_drag_status ds)
    {
            struct example_corewindow *example_cw = (struct example_corewindow *)cw;
            example_cw->drag_status = ds;
    }
    
    
    struct core_window_callback_table example_cw_cb_table = {
            .invalidate = example_cw_invalidate,
            .update_size = example_cw_update_size,
            .scroll_visible = example_cw_scroll_visible,
            .get_window_dimensions = example_cw_get_window_dimensions,
            .drag_status = example_cw_drag_status
    };
    
    /* exported function documented example/corewindow.h */
    nserror example_corewindow_init(struct example_corewindow *example_cw)
    {
            /* setup the core window callback table */
            example_cw->cb_table = &example_cw_cb_table;
    
            /* frontend toolkit specific method of causing example_cw_draw_event to be called when a drawing operation is required */
            toolkit_connect_draw_event(example_cw->tk_widget,
                               example_cw_draw_event,
                               example_cw);
    
            /* frontend toolkit specific method of causing example_cw_button_press_event to be called when a button press occours */
            toolkit_connect_button_press_event(example_cw->tk_widget,
                                       example_cw_button_press_event,
                                       example_cw);
    
            /* frontend toolkit specific method of causing example_cw_button_release_event to be called when a button release occours */
            toolkit_connect_button_release_event(example_cw->tk_widget,
                                         example_cw_button_release_event,
                                         example_cw);
    
            /* frontend toolkit specific method of causing example_cw_motion_notify_event to be called when there is motion over the widget */
            toolkit_connect_motion_event(example_cw->tk_widget,
                                 example_cw_motion_notify_event,
                                 example_cw);
    
            /* frontend toolkit specific method of causing example_cw_key_press_event to be called when a key press occours */
            toolkit_connect_button_press_event(example_cw->tk_widget,
                                       example_cw_key_press_event,
                                       example_cw);
    
            /* frontend toolkit specific method of causing example_cw_key_release_event to be called when a key release occours */
            toolkit_connect_button_release_event(example_cw->tk_widget,
                                         example_cw_key_release_event,
                                         example_cw);
    
    
            return NSERROR_OK;
    }
    
    /* exported interface documented in example/corewindow.h */
    nserror example_corewindow_fini(struct example_corewindow *example_cw)
    {
            return NSERROR_OK;
    }


frontends/example/ssl_cert.h
----------------------------

    /*
     * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
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
    
    #ifndef NETSURF_EXAMPLE_SSL_CERT_H
    #define NETSURF_EXAMPLE_SSL_CERT_H 1
    
    struct nsurl;
    struct ssl_cert_info;
    
    /**
     * Prompt the user to verify a certificate with issuse.
     *
     * \param url The URL being verified.
     * \param certs The certificate to be verified
     * \param num The number of certificates to be verified.
     * \param cb Callback upon user decision.
     * \param cbpw Context pointer passed to cb
     * \return NSERROR_OK or error code if prompt creation failed.
     */
    nserror example_cert_verify(struct nsurl *url, const struct ssl_cert_info *certs, unsigned long num, nserror (*cb)(bool proceed, void *pw), void *cbpw);
    
    #endif

frontends/example/ssl_cert.c
----------------------------

    /*
     * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
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
     * file
     * Implementation of example certificate viewing using example core windows.
     */
    
    #include <stdint.h>
    #include <stdlib.h>
    
    #include "utils/log.h"
    #include "netsurf/keypress.h"
    #include "netsurf/plotters.h"
    #include "desktop/sslcert_viewer.h"
    
    #include "example/corewindow.h"
    
    
    /**
     * EXAMPLE certificate viewing window context
     */
    struct example_crtvrfy_window {
            /** example core window context */
            struct example_corewindow core;
    
            /** SSL certificate viewer context data */
            struct sslcert_session_data *ssl_data;
    };
    
    /**
     * destroy a previously created certificate view
     */
    static nserror example_crtvrfy_destroy(struct example_crtvrfy_window *crtvrfy_win)
    {
            nserror res;
    
            res = sslcert_viewer_fini(crtvrfy_win->ssl_data);
            if (res == NSERROR_OK) {
                    res = example_corewindow_fini(&crtvrfy_win->core);
                    toolkit_windown_destroy(crtvrfy_win->window);
                    free(crtvrfy_win);
            }
            return res;
    }
    
    static void
    example_crtvrfy_accept(ExampleButton *w, gpointer data)
    {
            struct example_crtvrfy_window *crtvrfy_win;
            crtvrfy_win = (struct example_crtvrfy_window *)data;
    
            sslcert_viewer_accept(crtvrfy_win->ssl_data);
    
            example_crtvrfy_destroy(crtvrfy_win);
    }
    
    static void
    example_crtvrfy_reject(ExampleWidget *w, gpointer data)
    {
            struct example_crtvrfy_window *crtvrfy_win;
            crtvrfy_win = (struct example_crtvrfy_window *)data;
    
            sslcert_viewer_reject(crtvrfy_win->ssl_data);
    
            example_crtvrfy_destroy(crtvrfy_win);
    }
    
    
    /**
     * callback for mouse action for certificate verify on core window
     *
     * \param example_cw The example core window structure.
     * \param mouse_state netsurf mouse state on event
     * \param x location of event
     * \param y location of event
     * \return NSERROR_OK on success otherwise apropriate error code
     */
    static nserror
    example_crtvrfy_mouse(struct example_corewindow *example_cw,
                        browser_mouse_state mouse_state,
                        int x, int y)
    {
            struct example_crtvrfy_window *crtvrfy_win;
            /* technically degenerate container of */
            crtvrfy_win = (struct example_crtvrfy_window *)example_cw;
    
            sslcert_viewer_mouse_action(crtvrfy_win->ssl_data, mouse_state, x, y);
    
            return NSERROR_OK;
    }
    
    /**
     * callback for keypress for certificate verify on core window
     *
     * \param example_cw The example core window structure.
     * \param nskey The netsurf key code
     * \return NSERROR_OK on success otherwise apropriate error code
     */
    static nserror
    example_crtvrfy_key(struct example_corewindow *example_cw, uint32_t nskey)
    {
            struct example_crtvrfy_window *crtvrfy_win;
    
            /* technically degenerate container of */
            crtvrfy_win = (struct example_crtvrfy_window *)example_cw;
    
            if (sslcert_viewer_keypress(crtvrfy_win->ssl_data, nskey)) {
                    return NSERROR_OK;
            }
            return NSERROR_NOT_IMPLEMENTED;
    }
    
    /**
     * callback on draw event for certificate verify on core window
     *
     * \param example_cw The example core window structure.
     * \param r The rectangle of the window that needs updating.
     * \return NSERROR_OK on success otherwise apropriate error code
     */
    static nserror
    example_crtvrfy_draw(struct example_corewindow *example_cw, struct rect *r)
    {
            struct redraw_context ctx = {
                    .interactive = true,
                    .background_images = true,
                    .plot = &example_plotters
            };
            struct example_crtvrfy_window *crtvrfy_win;
    
            /* technically degenerate container of */
            crtvrfy_win = (struct example_crtvrfy_window *)example_cw;
    
            sslcert_viewer_redraw(crtvrfy_win->ssl_data, 0, 0, r, &ctx);
    
            return NSERROR_OK;
    }
    
    /* exported interface documented in example/ssl_cert.h */
    nserror example_cert_verify(struct nsurl *url,
                            const struct ssl_cert_info *certs,
                            unsigned long num,
                            nserror (*cb)(bool proceed, void *pw),
                            void *cbpw)
    {
            struct example_crtvrfy_window *ncwin;
            nserror res;
    
            ncwin = malloc(sizeof(struct example_crtvrfy_window));
            if (ncwin == NULL) {
                    return NSERROR_NOMEM;
            }
    
            res = toolkit_create_window(&ncwin->window);
            if (res != NSERROR_OK) {
                    LOG("SSL UI builder init failed");
                    free(ncwin);
                    return res;
            }
    
            /* store the widget that the toolkit is drawing into */
            ncwin->core.widget = toolkit_get_widget(ncwin->window, "SSLDrawingArea"));
    
            /* would typicaly setup toolkit accept/reject buttons etc. here */
            toolkit_connect_button_press(ncwin->tk_accept_button,
                                         example_crtvrfy_accept,
                                         ncwin);
    
    
            /* initialise example core window */
            ncwin->core.draw = example_crtvrfy_draw;
            ncwin->core.key = example_crtvrfy_key;
            ncwin->core.mouse = example_crtvrfy_mouse;
    
            res = example_corewindow_init(&ncwin->core);
            if (res != NSERROR_OK) {
                    free(ncwin);
                    return res;
            }
    
            /* initialise certificate viewing interface */
            res = sslcert_viewer_create_session_data(num, url, cb, cbpw, certs,
                                               &ncwin->ssl_data);
            if (res != NSERROR_OK) {
                    free(ncwin);
                    return res;
            }
    
            res = sslcert_viewer_init(ncwin->core.cb_table,
                                      (struct core_window *)ncwin,
                                      ncwin->ssl_data);
            if (res != NSERROR_OK) {
                    free(ncwin);
                    return res;
            }
    
            toolkit_widget_show(ncwin->window);
    
            return NSERROR_OK;
    }
