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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "utils/ring.h"
#include "utils/nsurl.h"

#include "monkey/output.h"
#include "monkey/401login.h"

struct monkey401 {
	struct monkey401 *r_next, *r_prev;
	uint32_t num;
	nserror (*cb)(struct nsurl*, const char *, const char *, const char *, void *);
	void *cbpw;
	char *username;
	char *password;
	char *realm;
	struct nsurl *url;
};

static struct monkey401 *m401_ring = NULL;
static uint32_t m401_ctr = 0;


nserror
gui_401login_open(struct nsurl *url,
		  const char *realm,
		  const char *username,
		  const char *password,
		  nserror (*cb)(struct nsurl *url,
				const char *realm,
				const char *username,
				const char *password,
				void *pw),
		  void *cbpw)
{
	struct monkey401 *m401_ctx;

	m401_ctx = calloc(sizeof(*m401_ctx), 1);
	if (m401_ctx == NULL) {
		return NSERROR_NOMEM;
	}
	m401_ctx->realm = strdup(realm);
	if (m401_ctx->realm == NULL) {
		free(m401_ctx);
		return NSERROR_NOMEM;
	}
	m401_ctx->url = nsurl_ref(url);
	m401_ctx->cb = cb;
	m401_ctx->cbpw = cbpw;
	m401_ctx->num = m401_ctr++;

	RING_INSERT(m401_ring, m401_ctx);

	if (username == NULL) {
		username = "";
	}

	if (password == NULL) {
		password = "";
	}

	moutf(MOUT_LOGIN, "OPEN LWIN %u URL %s", m401_ctx->num, nsurl_access(url));
	moutf(MOUT_LOGIN, "USER LWIN %u STR %s", m401_ctx->num, username);
	moutf(MOUT_LOGIN, "PASS LWIN %u STR %s", m401_ctx->num, password);
	moutf(MOUT_LOGIN, "REALM LWIN %u STR %s", m401_ctx->num, realm);

	return NSERROR_OK;
}

static struct monkey401 *
monkey_find_login_by_num(uint32_t login_num)
{
	struct monkey401 *ret = NULL;

	RING_ITERATE_START(struct monkey401, m401_ring, c_ring) {
		if (c_ring->num == login_num) {
			ret = c_ring;
			RING_ITERATE_STOP(m401_ring, c_ring);
		}
	} RING_ITERATE_END(m401_ring, c_ring);

	return ret;
}

static void free_login_context(struct monkey401 *m401_ctx) {
	moutf(MOUT_LOGIN, "DESTROY LWIN %u", m401_ctx->num);
	RING_REMOVE(m401_ring, m401_ctx);
	if (m401_ctx->username != NULL) {
		free(m401_ctx->username);
	}
	if (m401_ctx->password != NULL) {
		free(m401_ctx->password);
	}
	free(m401_ctx->realm);
	nsurl_unref(m401_ctx->url);
	free(m401_ctx);
}

static void
monkey_login_handle_go(int argc, char **argv)
{
	struct monkey401 *m401_ctx;

	if (argc != 3) {
		moutf(MOUT_ERROR, "LOGIN GO ARGS BAD");
		return;
	}

	m401_ctx = monkey_find_login_by_num(atoi(argv[2]));
	if (m401_ctx == NULL) {
		moutf(MOUT_ERROR, "LOGIN NUM BAD");
		return;
	}

	m401_ctx->cb(m401_ctx->url, m401_ctx->realm, m401_ctx->username, m401_ctx->password, m401_ctx->cbpw);
	
	free_login_context(m401_ctx);
}

static void
monkey_login_handle_destroy(int argc, char **argv)
{
	struct monkey401 *m401_ctx;

	if (argc != 3) {
		moutf(MOUT_ERROR, "LOGIN DESTROY ARGS BAD");
		return;
	}

	m401_ctx = monkey_find_login_by_num(atoi(argv[2]));
	if (m401_ctx == NULL) {
		moutf(MOUT_ERROR, "LOGIN NUM BAD");
		return;
	}

	free_login_context(m401_ctx);
}

static void
monkey_login_handle_username(int argc, char **argv)
{
	struct monkey401 *m401_ctx;

	if (argc != 4) {
		moutf(MOUT_ERROR, "LOGIN USERNAME ARGS BAD");
		return;
	}

	m401_ctx = monkey_find_login_by_num(atoi(argv[2]));
	if (m401_ctx == NULL) {
		moutf(MOUT_ERROR, "LOGIN NUM BAD");
		return;
	}

	if (m401_ctx->username != NULL) {
		free(m401_ctx->username);
	}

	m401_ctx->username = strdup(argv[3]);
}

static void
monkey_login_handle_password(int argc, char **argv)
{
	struct monkey401 *m401_ctx;

	if (argc != 4) {
		moutf(MOUT_ERROR, "LOGIN PASSWORD ARGS BAD");
		return;
	}

	m401_ctx = monkey_find_login_by_num(atoi(argv[2]));
	if (m401_ctx == NULL) {
		moutf(MOUT_ERROR, "LOGIN NUM BAD");
		return;
	}

	if (m401_ctx->password != NULL) {
		free(m401_ctx->password);
	}

	m401_ctx->password = strdup(argv[3]);
}

void
monkey_login_handle_command(int argc, char **argv)
{
	if (argc == 1)
		return;

	if (strcmp(argv[1], "USERNAME") == 0) {
		monkey_login_handle_username(argc, argv);
	} else if (strcmp(argv[1], "PASSWORD") == 0) {
		monkey_login_handle_password(argc, argv);
	} else if (strcmp(argv[1], "DESTROY") == 0) {
		monkey_login_handle_destroy(argc, argv);
	} else if (strcmp(argv[1], "GO") == 0) {
		monkey_login_handle_go(argc, argv);
	} else {
		moutf(MOUT_ERROR, "LOGIN COMMAND UNKNOWN %s\n", argv[1]);
	}
}
