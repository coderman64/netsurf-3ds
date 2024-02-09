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
 * NetSurf Logging Filters
 */

#include "nslog_internal.h"

#include "filter-parser.h"

/* Ensure compatability with bison 2.6 and later */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED && defined FILTER_STYPE_IS_DECLARED
#define YYSTYPE FILTER_STYPE
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED && defined FILTER_LTYPE_IS_DECLARED
#define YYLTYPE FILTER_LTYPE
#endif

#include "filter-lexer.h"

typedef enum {
	/* Fundamentals */
	NSLFK_CATEGORY = 0,
	NSLFK_LEVEL,
	NSLFK_FILENAME,
	NSLFK_DIRNAME,
	NSLFK_FUNCNAME,
	/* logical operations */
	NSLFK_AND,
	NSLFK_OR,
	NSLFK_XOR,
	NSLFK_NOT,
} nslog_filter_kind;

struct nslog_filter_s {
	nslog_filter_kind kind;
	int refcount;
	union {
		struct {
			char *ptr;
			int len;
		} str;
		nslog_level level;
		nslog_filter_t *unary_input;
		struct {
			nslog_filter_t *input1;
			nslog_filter_t *input2;
		} binary;
	} params;
};

static nslog_filter_t *nslog__active_filter = NULL;

nslog_error nslog_filter_category_new(const char *catname,
				      nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_CATEGORY;
	ret->refcount = 1;
	ret->params.str.ptr = strdup(catname);
	ret->params.str.len = strlen(catname);
	if (ret->params.str.ptr == NULL) {
		free(ret);
		return NSLOG_NO_MEMORY;
	}
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_level_new(nslog_level level,
				   nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_LEVEL;
	ret->refcount = 1;
	ret->params.level = level;
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_filename_new(const char *filename,
				      nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_FILENAME;
	ret->refcount = 1;
	ret->params.str.ptr = strdup(filename);
	ret->params.str.len = strlen(filename);
	if (ret->params.str.ptr == NULL) {
		free(ret);
		return NSLOG_NO_MEMORY;
	}
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_dirname_new(const char *dirname,
				     nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_DIRNAME;
	ret->refcount = 1;
	ret->params.str.ptr = strdup(dirname);
	ret->params.str.len = strlen(dirname);
	if (ret->params.str.ptr == NULL) {
		free(ret);
		return NSLOG_NO_MEMORY;
	}
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_funcname_new(const char *funcname,
				      nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_FUNCNAME;
	ret->refcount = 1;
	ret->params.str.ptr = strdup(funcname);
	ret->params.str.len = strlen(funcname);
	if (ret->params.str.ptr == NULL) {
		free(ret);
		return NSLOG_NO_MEMORY;
	}
	*filter = ret;
	return NSLOG_NO_ERROR;
}


nslog_error nslog_filter_and_new(nslog_filter_t *left,
				 nslog_filter_t *right,
				 nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_AND;
	ret->refcount = 1;
	ret->params.binary.input1 = nslog_filter_ref(left);
	ret->params.binary.input2 = nslog_filter_ref(right);
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_or_new(nslog_filter_t *left,
				nslog_filter_t *right,
				nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_OR;
	ret->refcount = 1;
	ret->params.binary.input1 = nslog_filter_ref(left);
	ret->params.binary.input2 = nslog_filter_ref(right);
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_xor_new(nslog_filter_t *left,
				 nslog_filter_t *right,
				 nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_XOR;
	ret->refcount = 1;
	ret->params.binary.input1 = nslog_filter_ref(left);
	ret->params.binary.input2 = nslog_filter_ref(right);
	*filter = ret;
	return NSLOG_NO_ERROR;
}

nslog_error nslog_filter_not_new(nslog_filter_t *input,
				 nslog_filter_t **filter)
{
	nslog_filter_t *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NSLOG_NO_MEMORY;
	ret->kind = NSLFK_NOT;
	ret->refcount = 1;
	ret->params.unary_input = nslog_filter_ref(input);
	*filter = ret;
	return NSLOG_NO_ERROR;
}


nslog_filter_t *nslog_filter_ref(nslog_filter_t *filter)
{
	if (filter != NULL)
		filter->refcount++;

	return filter;
}

nslog_filter_t *nslog_filter_unref(nslog_filter_t *filter)
{
	if (filter != NULL && filter->refcount-- == 1) {
		switch(filter->kind) {
		case NSLFK_CATEGORY:
		case NSLFK_FILENAME:
		case NSLFK_DIRNAME:
		case NSLFK_FUNCNAME:
			free(filter->params.str.ptr);
			break;
		case NSLFK_AND:
		case NSLFK_OR:
		case NSLFK_XOR:
			nslog_filter_unref(filter->params.binary.input1);
			nslog_filter_unref(filter->params.binary.input2);
			break;
		case NSLFK_NOT:
			nslog_filter_unref(filter->params.unary_input);
			break;
		default:
			/* Nothing to do for the other kind(s) */
			break;
		}
		free(filter);
	}

	return NULL;
}

nslog_error nslog_filter_set_active(nslog_filter_t *filter,
				    nslog_filter_t **prev)
{
	if (prev != NULL)
		*prev = nslog__active_filter;
	else
		nslog_filter_unref(nslog__active_filter);

	nslog__active_filter = nslog_filter_ref(filter);

	return NSLOG_NO_ERROR;
}

static bool _nslog__filter_matches(nslog_entry_context_t *ctx,
				   nslog_filter_t *filter)
{
	switch (filter->kind) {
	case NSLFK_CATEGORY:
		if (filter->params.str.len > ctx->category->namelen)
			return false;
		if (ctx->category->name[filter->params.str.len] != '\0' &&
		    ctx->category->name[filter->params.str.len] != '/')
			return false;
		return (strncmp(filter->params.str.ptr,
				ctx->category->name,
				filter->params.str.len) == 0);

	case NSLFK_LEVEL:
		return (ctx->level >= filter->params.level);
	case NSLFK_FILENAME:
		if (filter->params.str.len > ctx->filenamelen)
			return false;
		if ((filter->params.str.len == ctx->filenamelen) &&
		    (strcmp(filter->params.str.ptr, ctx->filename) == 0))
			return true;
		if ((ctx->filename[ctx->filenamelen - filter->params.str.len - 1] == '/')
		    && (strcmp(filter->params.str.ptr,
			       ctx->filename + ctx->filenamelen - filter->params.str.len) == 0))
			return true;
		return false;
	case NSLFK_DIRNAME:
		if (filter->params.str.len >= ctx->filenamelen)
			return false;
		if ((ctx->filename[filter->params.str.len] == '/')
		    && (strncmp(filter->params.str.ptr,
				ctx->filename,
				filter->params.str.len) == 0))
			return true;
		return false;
	case NSLFK_FUNCNAME:
		return (filter->params.str.len == ctx->funcnamelen &&
			strcmp(ctx->funcname, filter->params.str.ptr) == 0);
	case NSLFK_AND:
		return (_nslog__filter_matches(ctx, filter->params.binary.input1)
			&&
			_nslog__filter_matches(ctx, filter->params.binary.input2));
	case NSLFK_OR:
		return (_nslog__filter_matches(ctx, filter->params.binary.input1)
			||
			_nslog__filter_matches(ctx, filter->params.binary.input2));
	case NSLFK_XOR:
		return (_nslog__filter_matches(ctx, filter->params.binary.input1)
			^
			_nslog__filter_matches(ctx, filter->params.binary.input2));
	case NSLFK_NOT:
		return !_nslog__filter_matches(ctx, filter->params.unary_input);
	default:
		/* unknown */
		assert("Unknown filter kind" == NULL);
		return false;
	}
}

bool nslog__filter_matches(nslog_entry_context_t *ctx)
{
	if (nslog__active_filter == NULL)
		return true;
	return _nslog__filter_matches(ctx, nslog__active_filter);
}

char *nslog_filter_sprintf(nslog_filter_t *filter)
{
	char *ret = NULL;
	switch (filter->kind) {
	case NSLFK_CATEGORY:
		ret = calloc(filter->params.str.len + 5, 1);
		sprintf(ret, "cat:%s", filter->params.str.ptr);
		break;
	case NSLFK_LEVEL: {
		const char *lvl = nslog_level_name(filter->params.level);
		ret = calloc(strlen(lvl) + 5, 1);
		sprintf(ret, "lvl:%s", lvl);
		break;
	}
	case NSLFK_FILENAME:
		ret = calloc(filter->params.str.len + 6, 1);
		sprintf(ret, "file:%s", filter->params.str.ptr);
		break;
	case NSLFK_DIRNAME:
		ret = calloc(filter->params.str.len + 5, 1);
		sprintf(ret, "dir:%s", filter->params.str.ptr);
		break;
	case NSLFK_FUNCNAME:
		ret = calloc(filter->params.str.len + 6, 1);
		sprintf(ret, "func:%s", filter->params.str.ptr);
		break;
	case NSLFK_AND:
	case NSLFK_OR:
	case NSLFK_XOR: {
		char *left = nslog_filter_sprintf(filter->params.binary.input1);
		char *right = nslog_filter_sprintf(filter->params.binary.input2);
		const char *op =
			(filter->kind == NSLFK_AND) ? "&&" :
			(filter->kind == NSLFK_OR) ? "||" : "^";
		ret = calloc(strlen(left) + strlen(right) + 7, 1);
		sprintf(ret, "(%s %s %s)", left, op, right);
		free(left);
		free(right);
		break;
	}
	case NSLFK_NOT: {
		char *input = nslog_filter_sprintf(filter->params.unary_input);
		ret = calloc(strlen(input) + 2, 1);
		sprintf(ret, "!%s", input);
		free(input);
		break;
	}
	default:
		assert("Unexpected kind" == NULL);
		return strdup("***ERROR***");
	}
	return ret;
}

nslog_error nslog_filter_from_text(const char *input,
				   nslog_filter_t **output)
{
	int ret;
	YY_BUFFER_STATE buffer = filter__scan_string((char *)input);
	filter_push_buffer_state(buffer);
	ret = filter_parse(output);
	filter_lex_destroy();
	switch (ret) {
	case 0:
		return NSLOG_NO_ERROR;
	case 2:
		return NSLOG_NO_MEMORY;
	}
	return NSLOG_PARSE_ERROR;
}
