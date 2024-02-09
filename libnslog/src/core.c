/*
 * Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>
 *
 * This file is part of libnslog.
 *
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * NetSurf Logging Core
 */

#include "nslog_internal.h"

static bool nslog__corked = true;

static struct nslog_cork_chain {
	struct nslog_cork_chain *next;
	nslog_entry_context_t context;
	char message[0]; /* NUL terminated */
} *nslog__cork_chain = NULL, *nslog__cork_chain_last = NULL;

static nslog_callback nslog__cb = NULL;
static void *nslog__cb_ctx = NULL;

static nslog_category_t *nslog__all_categories = NULL;

const char *nslog_level_name(nslog_level level)
{
	switch (level) {
	case NSLOG_LEVEL_DEEPDEBUG:
		return "DEEPDEBUG";
	case NSLOG_LEVEL_DEBUG:
		return "DEBUG";
	case NSLOG_LEVEL_VERBOSE:
		return "VERBOSE";
	case NSLOG_LEVEL_INFO:
		return "INFO";
	case NSLOG_LEVEL_WARNING:
		return "WARNING";
	case NSLOG_LEVEL_ERROR:
		return "ERROR";
	case NSLOG_LEVEL_CRITICAL:
		return "CRITICAL";
	};

	return "**UNKNOWN**";
}

const char *nslog_short_level_name(nslog_level level)
{
	switch (level) {
	case NSLOG_LEVEL_DEEPDEBUG:
		return "DDBG";
	case NSLOG_LEVEL_DEBUG:
		return "DBG ";
	case NSLOG_LEVEL_VERBOSE:
		return "VERB";
	case NSLOG_LEVEL_INFO:
		return "INFO";
	case NSLOG_LEVEL_WARNING:
		return "WARN";
	case NSLOG_LEVEL_ERROR:
		return "ERR ";
	case NSLOG_LEVEL_CRITICAL:
		return "CRIT";
	};

	return "?UNK";
}


static void nslog__normalise_category(nslog_category_t *cat)
{
	if (cat->name != NULL)
		return;
	if (cat->parent == NULL) {
		cat->name = strdup(cat->cat_name);
		cat->namelen = strlen(cat->name);
	} else {
		nslog__normalise_category(cat->parent);
		int bufsz = strlen(cat->parent->name) + strlen(cat->cat_name) + 2 /* a slash and a NUL */;
		cat->name = malloc(bufsz);
		snprintf(cat->name, bufsz, "%s/%s", cat->parent->name, cat->cat_name);
		cat->namelen = bufsz - 1;
	}
	cat->next = nslog__all_categories;
	nslog__all_categories = cat;
}

static void nslog__log_corked(nslog_entry_context_t *ctx,
			      int measured_len,
			      const char *fmt,
			      va_list args)
{
	/* If corked, we need to store a copy */
	struct nslog_cork_chain *newcork = calloc(sizeof(struct nslog_cork_chain) + measured_len + 1, 1);
	if (newcork == NULL) {
		/* Wow, something went wrong */
		return;
	}
	newcork->context = *ctx;
	vsnprintf(newcork->message, measured_len + 1, fmt, args);
	if (nslog__cork_chain == NULL) {
		nslog__cork_chain = nslog__cork_chain_last = newcork;
	} else {
		nslog__cork_chain_last->next = newcork;
		nslog__cork_chain_last = newcork;
	}
}

static void nslog__log_uncorked(nslog_entry_context_t *ctx,
				const char *fmt,
				va_list args)
{
	/* TODO: Add filtering here */
	if (nslog__cb != NULL) {
		if (ctx->category->name == NULL) {
			nslog__normalise_category(ctx->category);
		}
		if (nslog__filter_matches(ctx))
			(*nslog__cb)(nslog__cb_ctx, ctx, fmt, args);
	}
}

void nslog__log(nslog_entry_context_t *ctx,
		const char *pattern,
		...)
{
	va_list ap;
	va_start(ap, pattern);
	if (nslog__corked) {
		va_list ap2;
		va_copy(ap2, ap);
		int slen = vsnprintf(NULL, 0, pattern, ap);
		va_end(ap);
		nslog__log_corked(ctx, slen, pattern, ap2);
		va_end(ap2);
	} else {
		nslog__log_uncorked(ctx, pattern, ap);
		va_end(ap);
	}
}

nslog_error nslog_set_render_callback(nslog_callback cb, void *context)
{
	nslog__cb = cb;
	nslog__cb_ctx = context;

	return NSLOG_NO_ERROR;
}


static void __nslog__deliver_corked_entry(nslog_entry_context_t *ctx,
					  const char *fmt,
					  ...)
{
	va_list args;
	va_start(args, fmt);
	if (nslog__cb != NULL) {
		(*nslog__cb)(nslog__cb_ctx, ctx, fmt, args);
	}
	va_end(args);
}

nslog_error nslog_uncork()
{
	if (nslog__corked) {
		while (nslog__cork_chain != NULL) {
			struct nslog_cork_chain *ent = nslog__cork_chain;
			nslog__cork_chain = ent->next;
			if (ent->context.category->name == NULL) {
				nslog__normalise_category(ent->context.category);
			}
			if (nslog__filter_matches(&ent->context))
				__nslog__deliver_corked_entry(&ent->context,
							      "%s", ent->message);
			free(ent);
		}
		nslog__corked = false;
		return NSLOG_NO_ERROR;
	} else {
		return NSLOG_UNCORKED;
	}
}

void nslog_cleanup()
{
	nslog_category_t *cat = nslog__all_categories;
	(void)nslog_uncork();
	(void)nslog_filter_set_active(NULL, NULL);
	while (cat != NULL) {
		nslog_category_t *nextcat = cat->next;
		free(cat->name);
		cat->name = NULL;
		cat->namelen = 0;
		cat->next = NULL;
		cat = nextcat;
	}
}
