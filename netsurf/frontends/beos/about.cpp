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

#define __STDBOOL_H__	1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "desktop/version.h"
#include "utils/log.h"
#include "testament.h"
#include "utils/useragent.h"
#include "curl/curlver.h"
#include "netsurf/clipboard.h"
}
#include "beos/about.h"
#include "beos/scaffolding.h"
#include "beos/window.h"

#include <private/interface/AboutWindow.h>
#include <Application.h>
#include <Invoker.h>
#include <String.h>


/**
 * Creates the about alert
 */
void nsbeos_about(struct gui_window *gui)
{
	BString text;
	text << "Netsurf  : " << user_agent_string() << "\n";
	text << "Version  : " << netsurf_version << "\n";
	text << "Build ID : " << WT_REVID << "\n";
	text << "Date     : " << WT_COMPILEDATE << "\n";
	text << "cURL     : " << LIBCURL_VERSION << "\n";

	BAboutWindow *alert = new BAboutWindow("About NetSurf", "application/x-vnd.NetSurf");
	alert->AddExtraInfo(text);
	alert->Show();
	//TODO: i18n-ize
}
