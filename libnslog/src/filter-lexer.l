%{

/* This is a lexer for libnslog filter syntax
 *
 * This file is part of libnslog.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "nslog/nslog.h"

#include "filter-parser.h"

/* Ensure compatability with bison 2.6 and later */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED && defined FILTER_STYPE_IS_DECLARED
#define YYSTYPE FILTER_STYPE
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED && defined FILTER_LTYPE_IS_DECLARED
#define YYLTYPE FILTER_LTYPE
#endif

/* Ensure we use yylloc to silence "variable `yylloc` set but not used" warning */
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;
%}


/* lexer options */
%option never-interactive
%option bison-bridge
%option bison-locations
%option warn
%option prefix="filter_"
%option nounput
%option noinput
%option noyywrap

whitespace	[ \t]+

pattern		[^ \t:|&)^!]+

%x		st_patt

%%

{whitespace}	{ /* nothing */ }

level:		{ return T_LEVEL_SPECIFIER; }
lvl:		{ return T_LEVEL_SPECIFIER; }

cat:		{ BEGIN(st_patt); return T_CATEGORY_SPECIFIER; }
category:	{ BEGIN(st_patt); return T_CATEGORY_SPECIFIER; }
file:		{ BEGIN(st_patt); return T_FILENAME_SPECIFIER; }
filename:	{ BEGIN(st_patt); return T_FILENAME_SPECIFIER; }
dir:		{ BEGIN(st_patt); return T_DIRNAME_SPECIFIER; }
dirname:	{ BEGIN(st_patt); return T_DIRNAME_SPECIFIER; }
func:		{ BEGIN(st_patt); return T_FUNCNAME_SPECIFIER; }
funcname:	{ BEGIN(st_patt); return T_FUNCNAME_SPECIFIER; }

"&&"		{ return T_OP_AND; }
"||"		{ return T_OP_OR; }

DEEPDEBUG	{ yylval->level = NSLOG_LEVEL_DEEPDEBUG; return T_LEVEL; }
DDEBUG		{ yylval->level = NSLOG_LEVEL_DEEPDEBUG; return T_LEVEL; }
DD		{ yylval->level = NSLOG_LEVEL_DEEPDEBUG; return T_LEVEL; }
DEBUG		{ yylval->level = NSLOG_LEVEL_DEBUG; return T_LEVEL; }
DBG		{ yylval->level = NSLOG_LEVEL_DEBUG; return T_LEVEL; }
VERBOSE		{ yylval->level = NSLOG_LEVEL_VERBOSE; return T_LEVEL; }
CHAT		{ yylval->level = NSLOG_LEVEL_VERBOSE; return T_LEVEL; }
INFO		{ yylval->level = NSLOG_LEVEL_INFO; return T_LEVEL; }
WARNING		{ yylval->level = NSLOG_LEVEL_WARNING; return T_LEVEL; }
WARN		{ yylval->level = NSLOG_LEVEL_WARNING; return T_LEVEL; }
ERROR		{ yylval->level = NSLOG_LEVEL_ERROR; return T_LEVEL; }
ERR		{ yylval->level = NSLOG_LEVEL_ERROR; return T_LEVEL; }
CRITICAL	{ yylval->level = NSLOG_LEVEL_CRITICAL; return T_LEVEL; }
CRIT		{ yylval->level = NSLOG_LEVEL_CRITICAL; return T_LEVEL; }

<st_patt>{pattern}	{ yylval->patt = yytext; BEGIN(INITIAL); return T_PATTERN; }

.		{ return (int) yytext[0]; }
