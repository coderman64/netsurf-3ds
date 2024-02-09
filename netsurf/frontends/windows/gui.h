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

#ifndef NETSURF_WINDOWS_GUI_H
#define NETSURF_WINDOWS_GUI_H

struct gui_window;

/**
 * win32 application instance handle.
 *
 * This handle is set in the main windows entry point.
 */
extern HINSTANCE hinst;

/**
 * path to where all user config files are held.
 */
extern char *G_config_path;

/**
 * resource search path vector.
 */
extern char **G_resource_pathv;

/* bounding box */
typedef struct bbox_s {
        int x0;
        int y0;
        int x1;
        int y1;
} bbox_t;

/**
 * Run the win32 message loop with scheduling
 */
void win32_run(void);

/**
 * cause the main message loop to exit
 */
void win32_set_quit(bool q);

/**
 * Warn the user of an event.
 *
 * \param[in] warning A warning looked up in the message translation table
 * \param[in] detail Additional text to be displayed or NULL.
 * \return NSERROR_OK on success or error code if there was a
 *           faliure displaying the message to the user.
 */
nserror win32_warning(const char *warning, const char *detail);

/**
 * Warn the user of an unexpected nserror.
 *
 * \param[in] error The nserror to report
 * \param[in] detail Additional text to be displayed or NULL.
 * \return NSERROR_OK on success or error code if there was a
 *           faliure displaying the message to the user.
 */
nserror win32_report_nserror(nserror error, const char *detail);

/**
 * add a modeless dialog to the special handling list
 */
nserror nsw32_add_dialog(HWND hwndDlg);

/**
 * remove a modeless dialog from the special handling list
 */
nserror nsw32_del_dialog(HWND hwndDlg);


#endif 
