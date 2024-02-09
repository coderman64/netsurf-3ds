/*
 * Copyright 2011 Daniel Silverstone <dsilvers@digital-scurf.org>
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
#include <limits.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "utils/config.h"
#include "utils/sys_time.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/filepath.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "netsurf/misc.h"
#include "netsurf/netsurf.h"
#include "netsurf/url_db.h"
#include "netsurf/cookie_db.h"
#include "content/fetch.h"
#include "content/backing_store.h"

#include "monkey/output.h"
#include "monkey/dispatch.h"
#include "monkey/browser.h"
#include "monkey/401login.h"
#include "monkey/filetype.h"
#include "monkey/fetch.h"
#include "monkey/schedule.h"
#include "monkey/bitmap.h"
#include "monkey/layout.h"

/** maximum number of languages in language vector */
#define LANGV_SIZE 32
/** maximum length of all strings in language vector */
#define LANGS_SIZE 4096

/** resource search path vector */
char **respaths;

static bool monkey_done = false;

/**
 * Cause an abnormal program termination.
 *
 * \note This never returns and is intended to terminate without any cleanup.
 *
 * \param error The message to display to the user.
 */
static void die(const char * const error)
{
	moutf(MOUT_DIE, "%s", error);
	exit(EXIT_FAILURE);
}

/**
 * obtain language from environment
 *
 * start with GNU extension LANGUAGE environment variable and then try
 * POSIX variables LC_ALL, LC_MESSAGES and LANG
 *
 */
static const char *get_language(void)
{
	const char *lang;

	lang = getenv("LANGUAGE");
	if ((lang != NULL) && (lang[0] != '\0')) {
		return lang;
	}

	lang = getenv("LC_ALL");
	if ((lang != NULL) && (lang[0] != '\0')) {
		return lang;
	}

	lang = getenv("LC_MESSAGES");
	if ((lang != NULL) && (lang[0] != '\0')) {
		return lang;
	}

	lang = getenv("LANG");
	if ((lang != NULL) && (lang[0] != '\0')) {
		return lang;
	}

	return NULL;
}


/**
 * provide a string vector of languages in preference order
 *
 * environment variables are processed to aquire a colon separated
 * list of languages which are converted into a string vector. The
 * vector will always have the C language as its last entry.
 *
 * This implementation creates an internal static representation of
 * the vector when first called and returns that for all subsequent
 * calls. i.e. changing the environment does not change the returned
 * vector on repeated calls.
 *
 * If the environment variables have more than LANGV_SIZE languages or
 * LANGS_SIZE bytes of data the results list will be curtailed.
 */
static const char * const *get_languagev(void)
{
	static const char *langv[LANGV_SIZE];
	int langidx = 0; /* index of next entry in vector */
	static char langs[LANGS_SIZE];
	char *curp; /* next language parameter in langs string */
	const char *lange; /* language from environment variable */
	int lang_len;
	char *cln; /* colon in lange */

	/* return cached vector */
	if (langv[0] != NULL) {
		return &langv[0];
	}

	curp = &langs[0];

	lange = get_language();

	if (lange != NULL) {
		lang_len = strlen(lange) + 1;
		if (lang_len < (LANGS_SIZE - 2)) {
			memcpy(curp, lange, lang_len);
			while ((curp[0] != 0) &&
			       (langidx < (LANGV_SIZE - 2))) {
				/* avoid using strchrnul as it is not portable */
				cln = strchr(curp, ':');
				if (cln == NULL) {
					langv[langidx++] = curp;
					curp += lang_len;
					break;
				} else {
					if ((cln - curp) > 1) {
						/* only place non empty entries in vector */
						langv[langidx++] = curp;
					}
					*cln++ = 0; /* null terminate */
					lang_len -= (cln - curp);
					curp = cln;
				}
			}
		}
	}

	/* ensure C language is present */
	langv[langidx++] = curp;
	*curp++ = 'C';
	*curp++ = 0;
	langv[langidx] = NULL;

	return &langv[0];
}

/**
 * Create an array of valid paths to search for resources.
 *
 * The idea is that all the complex path computation to find resources
 * is performed here, once, rather than every time a resource is
 * searched for.
 *
 * \param resource_path A shell style colon separated path list
 * \return A string vector of valid paths where resources can be found
 */
static char **
nsmonkey_init_resource(const char *resource_path)
{
	const char * const *langv;
	char **pathv; /* resource path string vector */
	char **respath; /* resource paths vector */

	pathv = filepath_path_to_strvec(resource_path);

	langv = get_languagev();

	respath = filepath_generate(pathv, langv);

	filepath_free_strvec(pathv);

	return respath;
}

static void monkey_quit(void)
{
	urldb_save_cookies(nsoption_charp(cookie_jar));
	urldb_save(nsoption_charp(url_file));
	monkey_fetch_filetype_fin();
}

static nserror gui_launch_url(struct nsurl *url)
{
	moutf(MOUT_GENERIC, "LAUNCH URL %s", nsurl_access(url));
	return NSERROR_OK;
}

static nserror gui_present_cookies(const char *search_term)
{
	if (search_term != NULL) {
		moutf(MOUT_GENERIC, "PRESENT_COOKIES %s", search_term);
	} else {
		moutf(MOUT_GENERIC, "PRESENT_COOKIES");
	}
	return NSERROR_OK;
}

static void quit_handler(int argc, char **argv)
{
	monkey_done = true;
}

static void monkey_options_handle_command(int argc, char **argv)
{
	nsoption_commandline(&argc, argv, nsoptions);
}

/**
 * Set option defaults for monkey frontend
 *
 * @param defaults The option table to update.
 * @return error status.
 */
static nserror set_defaults(struct nsoption_s *defaults)
{
	/* Set defaults for absent option strings */
	nsoption_setnull_charp(cookie_file, strdup("~/.netsurf/Cookies"));
	nsoption_setnull_charp(cookie_jar, strdup("~/.netsurf/Cookies"));
	nsoption_setnull_charp(url_file, strdup("~/.netsurf/URLs"));

	return NSERROR_OK;
}


/**
 * Ensures output logging stream is correctly configured
 */
static bool nslog_stream_configure(FILE *fptr)
{
	/* set log stream to be non-buffering */
	setbuf(fptr, NULL);

	return true;
}

static struct gui_misc_table monkey_misc_table = {
	.schedule = monkey_schedule,

	.quit = monkey_quit,
	.launch_url = gui_launch_url,
	.login = gui_401login_open,
	.present_cookies = gui_present_cookies,
};

static void monkey_run(void)
{
	fd_set read_fd_set, write_fd_set, exc_fd_set;
	int max_fd;
	int rdy_fd;
	int schedtm;
	struct timeval tv;
	struct timeval* timeout;

	while (!monkey_done) {

		/* discover the next scheduled event time */
		schedtm = monkey_schedule_run();

		/* clears fdset */
		fetch_fdset(&read_fd_set, &write_fd_set, &exc_fd_set, &max_fd);

		/* add stdin to the set */
		if (max_fd < 0) {
			max_fd = 0;
		}
		FD_SET(0, &read_fd_set);
		FD_SET(0, &exc_fd_set);

		/* setup timeout */
		switch (schedtm) {
		case -1:
			NSLOG(netsurf, INFO, "Iterate blocking");
			moutf(MOUT_GENERIC, "POLL BLOCKING");
			timeout = NULL;
			break;

		case 0:
			NSLOG(netsurf, INFO, "Iterate immediate");
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			timeout = &tv;
			break;

		default:
			NSLOG(netsurf, INFO, "Iterate non-blocking");
			moutf(MOUT_GENERIC, "POLL TIMED %d", schedtm);
			tv.tv_sec = schedtm / 1000; /* miliseconds to seconds */
			tv.tv_usec = (schedtm % 1000) * 1000; /* remainder to microseconds */
			timeout = &tv;
			break;
		}

		rdy_fd = select(max_fd + 1,
				&read_fd_set,
				&write_fd_set,
				&exc_fd_set,
				timeout);
		if (rdy_fd < 0) {
			NSLOG(netsurf, CRITICAL, "Unable to select: %s", strerror(errno));
			monkey_done = true;
		} else if (rdy_fd > 0) {
			if (FD_ISSET(0, &read_fd_set)) {
				monkey_process_command();
			}
		}
	}
}

#if (!defined(NDEBUG) && defined(HAVE_EXECINFO))
#include <execinfo.h>
static void *backtrace_buffer[4096];

void
__assert_fail(const char *__assertion, const char *__file,
	      unsigned int __line, const char *__function)
{
	int frames;
	fprintf(stderr,
		"MONKEY: Assertion failure!\n"
		"%s:%d: %s: Assertion `%s` failed.\n",
		__file, __line, __function, __assertion);

	frames = backtrace(&backtrace_buffer[0], 4096);
	if (frames > 0 && frames < 4096) {
		fprintf(stderr, "Backtrace:\n");
		fflush(stderr);
		backtrace_symbols_fd(&backtrace_buffer[0], frames, 2);
	}

        abort();
}

static void
signal_handler(int sig)
{
	int frames;
	fprintf(stderr, "Caught signal %s (%d)\n",
		((sig == SIGSEGV) ? "SIGSEGV" :
		 ((sig == SIGILL) ? "SIGILL" :
		  ((sig == SIGFPE) ? "SIGFPE" :
		   ((sig == SIGBUS) ? "SIGBUS" :
		    "unknown signal")))),
		sig);
	frames = backtrace(&backtrace_buffer[0], 4096);
	if (frames > 0 && frames < 4096) {
		fprintf(stderr, "Backtrace:\n");
		fflush(stderr);
		backtrace_symbols_fd(&backtrace_buffer[0], frames, 2);
	}

        abort();
}

#endif

int
main(int argc, char **argv)
{
	char *messages;
	char *options;
	char buf[PATH_MAX];
	nserror ret;
	struct netsurf_table monkey_table = {
		.misc = &monkey_misc_table,
		.window = monkey_window_table,
		.download = monkey_download_table,
		.fetch = monkey_fetch_table,
		.bitmap = monkey_bitmap_table,
		.layout = monkey_layout_table,
                .llcache = filesystem_llcache_table,
	};

#if (!defined(NDEBUG) && defined(HAVE_EXECINFO))
	/* Catch segfault, illegal instructions and fp exceptions */
	signal(SIGSEGV, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGFPE, signal_handler);
	/* It's unlikely, but SIGBUS could happen on some platforms */
	signal(SIGBUS, signal_handler);
#endif

	ret = netsurf_register(&monkey_table);
	if (ret != NSERROR_OK) {
		die("NetSurf operation table failed registration");
	}

	/* Unbuffer stdin/out/err */
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	/* Prep the search paths */
	respaths = nsmonkey_init_resource("${HOME}/.netsurf/:${NETSURFRES}:"MONKEY_RESPATH":./frontends/monkey/res");

	/* initialise logging. Not fatal if it fails but not much we can do
	 * about it either.
	 */
	nslog_init(nslog_stream_configure, &argc, argv);

	/* user options setup */
	ret = nsoption_init(set_defaults, &nsoptions, &nsoptions_default);
	if (ret != NSERROR_OK) {
		die("Options failed to initialise");
	}
	options = filepath_find(respaths, "Choices");
	nsoption_read(options, nsoptions);
	free(options);
	nsoption_commandline(&argc, argv, nsoptions);

	messages = filepath_find(respaths, "Messages");
	ret = messages_add_from_file(messages);
	if (ret != NSERROR_OK) {
		NSLOG(netsurf, INFO, "Messages failed to load");
	}

	/* common initialisation */
	ret = netsurf_init(NULL);
	free(messages);
	if (ret != NSERROR_OK) {
		die("NetSurf failed to initialise");
	}

	filepath_sfinddef(respaths, buf, "mime.types", "/etc/");
	monkey_fetch_filetype_init(buf);

	urldb_load(nsoption_charp(url_file));
	urldb_load_cookies(nsoption_charp(cookie_file));

	/* Free resource paths now we're done finding resources */
	for (char **s = respaths; *s != NULL; s++) {
		free(*s);
	}
	free(respaths);

	ret = monkey_register_handler("QUIT", quit_handler);
	if (ret != NSERROR_OK) {
		die("quit handler failed to register");
	}

	ret = monkey_register_handler("WINDOW", monkey_window_handle_command);
	if (ret != NSERROR_OK) {
		die("window handler failed to register");
	}

	ret = monkey_register_handler("OPTIONS", monkey_options_handle_command);
	if (ret != NSERROR_OK) {
		die("options handler failed to register");
	}

	ret = monkey_register_handler("LOGIN", monkey_login_handle_command);
	if (ret != NSERROR_OK) {
		die("login handler failed to register");
	}


	moutf(MOUT_GENERIC, "STARTED");
	monkey_run();

	moutf(MOUT_GENERIC, "CLOSING_DOWN");
	monkey_kill_browser_windows();

	netsurf_exit();
	moutf(MOUT_GENERIC, "FINISHED");

	/* finalise options */
	nsoption_finalise(nsoptions, nsoptions_default);

	/* finalise logging */
	nslog_finalise();

	/* And free any monkey-specific bits */
	monkey_free_handlers();

	return 0;
}
