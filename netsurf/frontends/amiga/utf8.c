/*
 * Copyright 2008-2021 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <proto/codesets.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "utils/nsoption.h"
#include "utils/utf8.h"
#include "netsurf/utf8.h"

#include "amiga/utf8.h"

static nserror ami_utf8_codesets(const char *string, size_t len, char **result, bool to_local)
{
	char *out;
	ULONG utf8_tag = CSA_SourceCodeset, local_tag = CSA_DestCodeset, len_tag = CSA_SourceLen;
	static struct codeset *utf8_cs = NULL;
	static struct codeset *local_cs = NULL;

	if(local_cs == NULL) local_cs = CodesetsFind(NULL,
#ifdef __amigaos4__
						CSA_MIBenum, nsoption_int(local_codeset),
#else
						NULL,
#endif
					TAG_DONE);

     if(utf8_cs == NULL) utf8_cs = CodesetsFind(NULL,
                           CSA_MIBenum, CS_MIBENUM_UTF_8,
                           TAG_DONE);

	if(to_local == false) {
		local_tag = CSA_SourceCodeset;
		utf8_tag = CSA_DestCodeset;
	}

	if(len == 0) len_tag = TAG_IGNORE;

	out = CodesetsConvertStr(CSA_Source, string,
						len_tag, len,
#ifdef __amigaos4__
						local_tag, local_cs,
#endif
						utf8_tag, utf8_cs,
						CSA_MapForeignChars, TRUE,
						TAG_DONE);

	if(out != NULL) {
		*result = strdup(out);
		CodesetsFreeA(out, NULL);
	} else {
		return NSERROR_BAD_ENCODING;
	}

	return NSERROR_OK;
}

nserror utf8_from_local_encoding(const char *string, size_t len, char **result)
{
	if(__builtin_expect((CodesetsBase == NULL), 0)) {
		return utf8_from_enc(string, nsoption_charp(local_charset), len, result, NULL);
	} else {
		return ami_utf8_codesets(string, len, result, false);
	}
}

nserror utf8_to_local_encoding(const char *string, size_t len, char **result)
{
	if(__builtin_expect((CodesetsBase == NULL), 0)) {
		nserror err = NSERROR_NOMEM;
		char *local_charset = ASPrintf("%s//IGNORE", nsoption_charp(local_charset));
		if(local_charset) {
			err = utf8_to_enc(string, local_charset, len, result);
			FreeVec(local_charset);
		}
		return err;
	} else {
		return ami_utf8_codesets(string, len, result, true);
	}
}

void ami_utf8_free(char *ptr)
{
	if(ptr) free(ptr);
}

char *ami_utf8_easy(const char *string)
{
	char *localtext;
	if(utf8_to_local_encoding(string, strlen(string), &localtext) == NSERROR_OK) {
		return localtext;
	} else {
		return strdup(string);
	}
}

char *ami_to_utf8_easy(const char *string)
{
	char *localtext;

	if(utf8_from_local_encoding(string, strlen(string), &localtext) == NSERROR_OK) {
		return localtext;
	} else {
		return strdup(string);
	}
}


static struct gui_utf8_table utf8_table = {
	.utf8_to_local = utf8_to_local_encoding,
	.local_to_utf8 = utf8_from_local_encoding,
};

struct gui_utf8_table *amiga_utf8_table = &utf8_table;

