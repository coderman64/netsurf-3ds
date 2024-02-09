%{
/* This is a bison parser for libnslog's filter syntax
 *
 * This file is part of libnslog.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>
 *
 */

#include "nslog/nslog.h"
#include <assert.h>

#include "filter-parser.h"
#include "filter-lexer.h"

static void filter_error(YYLTYPE *loc, nslog_filter_t **output, const char *msg)
{
	(void)loc;
	(void)output;
	(void)msg;
}

%}

%locations
%pure-parser
%parse-param { nslog_filter_t **output }

%union {
	char *patt;
	nslog_level level;
	nslog_filter_t *filter;
}

%destructor {
	nslog_filter_unref($$);
} <filter>

%token <patt> T_PATTERN
%token <level> T_LEVEL

%token T_CATEGORY_SPECIFIER
%token T_FILENAME_SPECIFIER
%token T_LEVEL_SPECIFIER
%token T_DIRNAME_SPECIFIER
%token T_FUNCNAME_SPECIFIER

%token T_OP_AND
%token T_OP_OR

%type <filter> level_filter
%type <filter> category_filter
%type <filter> filename_filter
%type <filter> dirname_filter
%type <filter> funcname_filter
%type <filter> basic_filter

%type <filter> and_filter
%type <filter> or_filter
%type <filter> xor_filter
%type <filter> binary_filter
%type <filter> not_filter

%type <filter> filter
%type <filter> toplevel

%start toplevel

%%

 /*
   part ::= [^: \t\n]+
level-name ::= 'DEEPDEBUG' | 'DD' | 'DEBUG' | 'VERBOSE' | 'CHAT' |
               'WARNING' | 'WARN' | 'ERROR' | 'ERR' | 'CRITICAL' | 'CRIT'

category-filter ::= 'cat:' part
level-filter ::= 'level:' level-name
file-filter ::= 'file:' part
dir-filter ::= 'dir:' dir

factor ::= category-filter | level-filter | file-filter | dir-filter |
           '(' expression ')'

op ::= '&&' | '||' | '^' | 'and' | 'or' | 'xor' | 'eor'

term ::= factor {op factor}

expression ::= term | '!' term
 */

level_filter:
	T_LEVEL_SPECIFIER T_LEVEL
	{
		if (nslog_filter_level_new($2, &$$) != NSLOG_NO_ERROR) {
			YYABORT ;
		}
	}
	;

category_filter:
	T_CATEGORY_SPECIFIER T_PATTERN
	{
		if (nslog_filter_category_new($2, &$$) != NSLOG_NO_ERROR) {
			YYABORT ;
		}
	}
	;

filename_filter:
	T_FILENAME_SPECIFIER T_PATTERN
	{
		if (nslog_filter_filename_new($2, &$$) != NSLOG_NO_ERROR) {
			YYABORT ;
		}
	}
	;

dirname_filter:
	T_DIRNAME_SPECIFIER T_PATTERN
	{
		if (nslog_filter_dirname_new($2, &$$) != NSLOG_NO_ERROR) {
			YYABORT ;
		}
	}
	;

funcname_filter:
	T_FUNCNAME_SPECIFIER T_PATTERN
	{
		if (nslog_filter_funcname_new($2, &$$) != NSLOG_NO_ERROR) {
			YYABORT ;
		}
	}
	;

basic_filter:
	level_filter
	|
	category_filter
	|
	filename_filter
	|
	dirname_filter
	|
	funcname_filter
	;

and_filter:
	'(' filter T_OP_AND filter ')'
	{
		if (nslog_filter_and_new($2, $4, &$$) != NSLOG_NO_ERROR) {
			nslog_filter_unref($2);
			nslog_filter_unref($4);
			YYABORT ;
		}
		nslog_filter_unref($2);
		nslog_filter_unref($4);
	}
	;

or_filter:
	'(' filter T_OP_OR filter ')'
	{
		if (nslog_filter_or_new($2, $4, &$$) != NSLOG_NO_ERROR) {
			nslog_filter_unref($2);
			nslog_filter_unref($4);
			YYABORT ;
		}
		nslog_filter_unref($2);
		nslog_filter_unref($4);
	}
	;

xor_filter:
	'(' filter '^' filter ')'
	{
		if (nslog_filter_xor_new($2, $4, &$$) != NSLOG_NO_ERROR) {
			nslog_filter_unref($2);
			nslog_filter_unref($4);
			YYABORT ;
		}
		nslog_filter_unref($2);
		nslog_filter_unref($4);
	}
	;

binary_filter:
	and_filter
	|
	or_filter
	|
	xor_filter
	;

not_filter:
	'!' filter
	{
		if (nslog_filter_not_new($2, &$$) != NSLOG_NO_ERROR) {
			nslog_filter_unref($2);
			YYABORT ;
		}
		nslog_filter_unref($2);
	}
	;

filter:
	not_filter
	|
	binary_filter
	|
	basic_filter
	;

toplevel:
	filter
	{
		$$ = *output = nslog_filter_ref($1);
	}
	|
	error
	{
		(void)yylloc;
		$$ = NULL;
		YYABORT ;
	}
	;
