/*
 * Copyright 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include "amiga/os3support.h"

#include <proto/exec.h>
#include <proto/utility.h>

#include "utils/nsoption.h"
#include "amiga/nsoption.h"

static char *current_user_options = NULL;

nserror ami_nsoption_read(void)
{
	return nsoption_read(current_user_options, NULL);
}

nserror ami_nsoption_write(void)
{
	return nsoption_write(current_user_options, NULL, NULL);
}

nserror ami_nsoption_set_location(const char *current_user_dir)
{
	nserror err = NSERROR_OK;

	ami_nsoption_free();

	current_user_options = ASPrintf("%s/Choices", current_user_dir);
	if(current_user_options == NULL)
		err = NSERROR_NOMEM;

	return err;
}

void ami_nsoption_free(void)
{
	if(current_user_options != NULL)
		FreeVec(current_user_options);

	current_user_options = NULL;
}

