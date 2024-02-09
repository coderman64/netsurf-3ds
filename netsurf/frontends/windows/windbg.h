/*
 * Copyright 2010 Vincent Sanders <vince@netsurf-browser.org>
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

#ifndef NETSURF_WINDOWS_WINDBG_H
#define NETSURF_WINDOWS_WINDBG_H

#include "utils/log.h"

const char *msg_num_to_name(int msg);
void win_perror(const char *lpszFunction);

/**
 * log windows message
 *
 * log a windows message with symbols converted to human redable
 */
#define LOG_WIN_MSG(h, m, w, l)						\
	if (((m) != WM_SETCURSOR) &&					\
	    ((m) != WM_MOUSEMOVE) &&					\
	    ((m) != WM_NCHITTEST) &&					\
	    ((m) != WM_ENTERIDLE))					\
		NSLOG(netsurf, DEBUG,					\
		      "%s, hwnd %p, w 0x%x, l 0x%Ix",			\
		      msg_num_to_name(m), h, w, l)

#endif
