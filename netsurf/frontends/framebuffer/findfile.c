/*
 * Copyright 2008 Daniel Silverstone <dsilvers@netsurf-browser.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/filepath.h"
#include "utils/log.h"

#include "framebuffer/findfile.h"

char **respaths; /** resource search path vector */

#define MAX_LANGV_SIZE 32

/**
 * goes through the environment in appropriate order to find configured language
 *
 * \return language to use or "C" if nothing appropriate is set
 */
static const char *get_language_env(void)
{
	const char *envstr;

	envstr = getenv("LANGUAGE");
	if ((envstr != NULL) && (envstr[0] != 0)) {
		return envstr;
	}

	envstr = getenv("LC_ALL");
	if ((envstr != NULL) && (envstr[0] != 0)) {
		return envstr;
	}

	envstr = getenv("LC_MESSAGES");
	if ((envstr != NULL) && (envstr[0] != 0)) {
		return envstr;
	}

	envstr = getenv("LANG");
	if ((envstr != NULL) && (envstr[0] != 0)) {
		return envstr;
	}

	return "C";
}

/**
 * build a string vector of language names
 */
static char **get_language_names(void)
{
	char **langv; /* output string vector of languages */
	int langc; /* count of languages in vector */
	const char *envlang; /* colon separated list of languages from environment */
	int lstart = 0; /* offset to start of current language */
	int lunder = 0; /* offset to underscore in current language */
	int lend = 0; /* offset to end of current language */
	char *nlang;
	
	langv = calloc(MAX_LANGV_SIZE + 2, sizeof(char *));
	if (langv == NULL) {
		return NULL;
	}

	envlang = get_language_env();
	
	for (langc = 0; langc < MAX_LANGV_SIZE; langc++) {
		/* work through envlang splitting on : */
		while ((envlang[lend] != 0) &&
		       (envlang[lend] != ':') &&
		       (envlang[lend] != '.')) {
			if (envlang[lend] == '_') {
				lunder = lend;
			}
			lend++;
		}
		/* place language in string vector */
		nlang = malloc(lend - lstart + 1);
		memcpy(nlang, envlang + lstart, lend - lstart);
		nlang[lend - lstart] = 0;
		langv[langc] = nlang;

		/* add language without specialisation to vector */
		if (lunder != lstart) {
			nlang = malloc(lunder - lstart + 1);
			memcpy(nlang, envlang + lstart, lunder - lstart);
			nlang[lunder - lstart] = 0;
			langv[++langc] = nlang;
		}

		/* if we stopped at the dot, move to the colon delimiter */
		while ((envlang[lend] != 0) &&
		       (envlang[lend] != ':')) {
			lend++;
		}
		if (envlang[lend] == 0) {
			/* reached end of environment language list */
			break;
		}
		lend++;
		lstart = lunder = lend;
	}
	return langv;
}

/**
 * Create an array of valid paths to search for resources.
 *
 * The idea is that all the complex path computation to find resources
 * is performed here, once, rather than every time a resource is
 * searched for.
 */
char **
fb_init_resource_path(const char *resource_path)
{
	char **pathv; /* resource path string vector */
	char **respath; /* resource paths vector */
	char **langv;

	pathv = filepath_path_to_strvec(resource_path);

	langv = get_language_names();

	respath = filepath_generate(pathv, (const char * const *)langv);

	filepath_free_strvec(langv);

	filepath_free_strvec(pathv);

	return respath;
}



/*
 * Local Variables:
 * c-basic-offset: 8
 * End:
 */

