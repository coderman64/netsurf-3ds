/* binding generator AST implementation for parser
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

/** @todo this currently stuffs everything in one global tree, not very nice
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "utils.h"
#include "nsgenbind-ast.h"
#include "options.h"

/**
 * standard IO handle for parse trace logging.
 */
static FILE *genbind_parsetracef;

/* parser and lexer interface */
extern int nsgenbind_debug;
extern int nsgenbind__flex_debug;
extern void nsgenbind_restart(FILE*);
extern int nsgenbind_parse(char *filename, struct genbind_node **genbind_ast);

/* terminal nodes have a value only */
struct genbind_node {
        enum genbind_node_type type;
        struct genbind_node *l;
        union {
                void *value;
                struct genbind_node *node;
                char *text;
                int number; /* node data is an integer */
        } r;
};

/* insert node(s) at beginning of a list */
struct genbind_node *
genbind_node_prepend(struct genbind_node *list, struct genbind_node *inst)
{
	struct genbind_node *end = inst;

	if (inst == NULL) {
		return list; /* no node to prepend - return existing list */
	}

	/* find end of inserted node list */
	while (end->l != NULL) {
		end = end->l;
	}

	end->l = list;

	return inst;
}

/* prepend list to a nodes list
 *
 * inserts a list into the beginning of a nodes r list
 *
 * CAUTION: if the \a node element is not a node type the node will not be added
 */
struct genbind_node *
genbind_node_add(struct genbind_node *node, struct genbind_node *list)
{
	if (node == NULL) {
		return list;
	}

	/* this does not use genbind_node_getnode() as it cannot
	 * determine between an empty node and a node which is not a
	 * list type
	 */
	switch (node->type) {
        case GENBIND_NODE_TYPE_BINDING:
        case GENBIND_NODE_TYPE_CLASS:
        case GENBIND_NODE_TYPE_PRIVATE:
        case GENBIND_NODE_TYPE_INTERNAL:
        case GENBIND_NODE_TYPE_PROPERTY:
        case GENBIND_NODE_TYPE_FLAGS:
        case GENBIND_NODE_TYPE_METHOD:
        case GENBIND_NODE_TYPE_PARAMETER:
                break;

	default:
                /* not a node type */
                return list;
	}

	node->r.node =	genbind_node_prepend(node->r.node, list);

	return node;
}

char *genbind_strapp(char *a, char *b)
{
        char *fullstr;
        int fulllen;
        fulllen = strlen(a) + strlen(b) + 1;
        fullstr = malloc(fulllen);
        snprintf(fullstr, fulllen, "%s%s", a, b);
        free(a);
        free(b);
        return fullstr;
}

struct genbind_node *
genbind_node_link(struct genbind_node *tgt, struct genbind_node *src)
{
        tgt->l = src;
        return tgt;
}


struct genbind_node *
genbind_new_node(enum genbind_node_type type, struct genbind_node *l, void *r)
{
        struct genbind_node *nn;
        nn = calloc(1, sizeof(struct genbind_node));
        nn->type = type;
        nn->l = l;
        nn->r.value = r;
        return nn;
}

struct genbind_node *
genbind_new_number_node(enum genbind_node_type type,
                        struct genbind_node *l,
                        int number)
{
        struct genbind_node *nn;
        nn = calloc(1, sizeof(struct genbind_node));
        nn->type = type;
        nn->l = l;
        nn->r.number = number;
        return nn;
}


/* exported interface defined in nsgenbind-ast.h */
int
genbind_node_foreach_type(struct genbind_node *node,
                           enum genbind_node_type type,
                           genbind_callback_t *cb,
                           void *ctx)
{
        int ret;

        if (node == NULL) {
                return -1;
        }
        if (node->l != NULL) {
                ret = genbind_node_foreach_type(node->l, type, cb, ctx);
                if (ret != 0) {
                        return ret;
                }
        }
        if (node->type == type) {
                return cb(node, ctx);
        }

        return 0;
}

static int genbind_enumerate_node(struct genbind_node *node, void *ctx)
{
	UNUSED(node);
        (*((int *)ctx))++;
        return 0;
}

/* exported interface defined in nsgenbind-ast.h */
int
genbind_node_enumerate_type(struct genbind_node *node,
                            enum genbind_node_type type)
{
        int count = 0;
        genbind_node_foreach_type(node,
                                  type,
                                  genbind_enumerate_node,
                                  &count);
        return count;
}

/* exported interface defined in nsgenbind-ast.h */
struct genbind_node *
genbind_node_find(struct genbind_node *node,
                  struct genbind_node *prev,
                  genbind_callback_t *cb,
                  void *ctx)
{
        struct genbind_node *ret;

        if ((node == NULL) || (node == prev)) {
                return NULL;
        }

        if (node->l != prev) {
                ret = genbind_node_find(node->l, prev, cb, ctx);
                if (ret != NULL) {
                        return ret;
                }
        }

        if (cb(node, ctx) != 0) {
                return node;
        }

        return NULL;
}

/* exported interface documented in nsgenbind-ast.h */
struct genbind_node *
genbind_node_find_type(struct genbind_node *node,
                       struct genbind_node *prev,
                       enum genbind_node_type type)
{
        return genbind_node_find(node,
                                 prev,
                                 genbind_cmp_node_type,
                                 (void *)type);
}

/* exported interface documented in nsgenbind-ast.h */
struct genbind_node *
genbind_node_find_type_ident(struct genbind_node *node,
                             struct genbind_node *prev,
                             enum genbind_node_type type,
                             const char *ident)
{
        struct genbind_node *found_node;
        struct genbind_node *ident_node;

        if (ident == NULL) {
                return NULL;
        }

        found_node = genbind_node_find_type(node, prev, type);

        while (found_node != NULL) {
                /* look for an ident node  */
                ident_node = genbind_node_find_type(
                        genbind_node_getnode(found_node),
                        NULL,
                        GENBIND_NODE_TYPE_IDENT);

                while (ident_node != NULL) {
                        /* check for matching text */
                        if (strcmp(ident_node->r.text, ident) == 0) {
                                return found_node;
                        }

                        ident_node = genbind_node_find_type(
                                genbind_node_getnode(found_node),
                                ident_node,
                                GENBIND_NODE_TYPE_IDENT);
                }


                /* look for next matching node */
                found_node = genbind_node_find_type(node, found_node, type);
        }
        return found_node;
}



/* exported interface documented in nsgenbind-ast.h */
struct genbind_node *
genbind_node_find_method(struct genbind_node *node,
                         struct genbind_node *prev,
                         enum genbind_method_type methodtype)
{
        struct genbind_node *res_node;

        res_node = genbind_node_find_type(
                genbind_node_getnode(node),
                prev, GENBIND_NODE_TYPE_METHOD);
        while (res_node != NULL) {
                struct genbind_node *type_node;
                enum genbind_method_type *type;

                type_node = genbind_node_find_type(
                        genbind_node_getnode(res_node),
                        NULL, GENBIND_NODE_TYPE_METHOD_TYPE);

                type = (enum genbind_method_type *)genbind_node_getint(type_node);
                if (*type == methodtype) {
                        break;
                }

                res_node = genbind_node_find_type(
                        genbind_node_getnode(node),
                        res_node, GENBIND_NODE_TYPE_METHOD);
        }

        return res_node;
}


/* exported interface documented in nsgenbind-ast.h */
struct genbind_node *
genbind_node_find_method_ident(struct genbind_node *node,
                               struct genbind_node *prev,
                               enum genbind_method_type nodetype,
                               const char *ident)
{
        struct genbind_node *res_node;
        char *method_ident;

        res_node = genbind_node_find_method(node, prev, nodetype);
        while (res_node != NULL) {
              method_ident = genbind_node_gettext(
                      genbind_node_find_type(
                              genbind_node_getnode(res_node),
                              NULL,
                              GENBIND_NODE_TYPE_IDENT));

              if ((ident != NULL) &&
                  (method_ident != NULL) &&
                  strcmp(ident, method_ident) == 0) {
                      break;
              }

              res_node = genbind_node_find_method(node, res_node, nodetype);
        }
        return res_node;
}


/* exported interface documented in nsgenbind-ast.h */
int genbind_cmp_node_type(struct genbind_node *node, void *ctx)
{
        if (node->type == (enum genbind_node_type)ctx)
                return 1;
        return 0;
}

char *genbind_node_gettext(struct genbind_node *node)
{
        if (node != NULL) {
                switch(node->type) {
                case GENBIND_NODE_TYPE_WEBIDL:
                case GENBIND_NODE_TYPE_STRING:
                case GENBIND_NODE_TYPE_IDENT:
                case GENBIND_NODE_TYPE_NAME:
                case GENBIND_NODE_TYPE_CDATA:
                case GENBIND_NODE_TYPE_FILE:
                        return node->r.text;

                default:
                        break;
                }
        }
        return NULL;
}

struct genbind_node *genbind_node_getnode(struct genbind_node *node)
{
        if (node != NULL) {
                switch(node->type) {
                case GENBIND_NODE_TYPE_BINDING:
                case GENBIND_NODE_TYPE_CLASS:
                case GENBIND_NODE_TYPE_PRIVATE:
                case GENBIND_NODE_TYPE_INTERNAL:
                case GENBIND_NODE_TYPE_PROPERTY:
                case GENBIND_NODE_TYPE_FLAGS:
                case GENBIND_NODE_TYPE_METHOD:
                case GENBIND_NODE_TYPE_PARAMETER:
                        return node->r.node;

                default:
                        break;
                }
        }
        return NULL;

}

int *genbind_node_getint(struct genbind_node *node)
{
        if (node != NULL) {
                switch(node->type) {
                case GENBIND_NODE_TYPE_METHOD_TYPE:
                case GENBIND_NODE_TYPE_LINE:
                case GENBIND_NODE_TYPE_MODIFIER:
                        return &node->r.number;

                default:
                        break;
                }
        }
        return NULL;
}

static const char *genbind_node_type_to_str(enum genbind_node_type type)
{
        switch(type) {
        case GENBIND_NODE_TYPE_IDENT:
                return "Ident";

        case GENBIND_NODE_TYPE_ROOT:
                return "Root";

        case GENBIND_NODE_TYPE_WEBIDL:
                return "webidl";

        case GENBIND_NODE_TYPE_STRING:
                return "String";

        case GENBIND_NODE_TYPE_BINDING:
                return "Binding";

        case GENBIND_NODE_TYPE_NAME:
                return "TypeName";

        case GENBIND_NODE_TYPE_LINE:
                return "Linenumber";

        case GENBIND_NODE_TYPE_FILE:
                return "Filename";

        case GENBIND_NODE_TYPE_PRIVATE:
                return "Private";

        case GENBIND_NODE_TYPE_INTERNAL:
                return "Internal";

        case GENBIND_NODE_TYPE_CLASS:
                return "Class";

        case GENBIND_NODE_TYPE_FLAGS:
                return "Flags";

        case GENBIND_NODE_TYPE_PROPERTY:
                return "Property";

        case GENBIND_NODE_TYPE_METHOD:
                return "Method";

        case GENBIND_NODE_TYPE_METHOD_TYPE:
                return "Type";

        case GENBIND_NODE_TYPE_PARAMETER:
                return "Parameter";

        case GENBIND_NODE_TYPE_CDATA:
                return "CBlock";

        default:
                return "Unknown";
        }
}

/** dump ast node to file at indent level */
static int genbind_ast_dump(FILE *dfile, struct genbind_node *node, int indent)
{
        const char *SPACES="                                                                               ";
        char *txt;
        int *val;

        while (node != NULL) {
                fprintf(dfile, "%.*s%s", indent, SPACES,
                        genbind_node_type_to_str(node->type));

                txt = genbind_node_gettext(node);
                if (txt == NULL) {
                        val = genbind_node_getint(node);
                        if (val == NULL) {
                                fprintf(dfile, "\n");
                                genbind_ast_dump(dfile,
                                                 genbind_node_getnode(node),
                                                 indent + 2);
                        } else {
                                fprintf(dfile, ": %d\n", *val);
                        }
                } else {
                        fprintf(dfile, ": \"%.*s\"\n", 75 - indent, txt);
                }
                node = node->l;
        }
        return 0;
}


/* exported interface documented in nsgenbind-ast.h */
int genbind_dump_ast(struct genbind_node *node)
{
        FILE *dumpf;

        /* only dump AST to file if required */
        if (!options->debug) {
                return 0;
        }

        dumpf = genb_fopen("binding-ast", "w");
        if (dumpf == NULL) {
                return 2;
        }

        genbind_ast_dump(dumpf, node, 0);

        fclose(dumpf);

        return 0;
}

FILE *genbindopen(const char *filename)
{
        FILE *genfile;
        char *fullname;
        int fulllen;
        static char *prevfilepath = NULL;

        /* try filename raw */
        genfile = fopen(filename, "r");
        if (genfile != NULL) {
                if (options->verbose) {
                        printf("Opened Genbind file %s\n", filename);
                }
                if (prevfilepath == NULL) {
                        fullname = strrchr(filename, '/');
                        if (fullname == NULL) {
                                fulllen = strlen(filename);
                        } else {
                                fulllen = fullname - filename;
                        }
                        prevfilepath = strndup(filename,fulllen);
                }
                return genfile;
        }

        /* try based on previous filename */
        if (prevfilepath != NULL) {
                fulllen = strlen(prevfilepath) + strlen(filename) + 2;
                fullname = malloc(fulllen);
                snprintf(fullname, fulllen, "%s/%s", prevfilepath, filename);
                if (options->verbose) {
                        printf("Attempting to open Genbind file %s\n", fullname);
                }
                genfile = fopen(fullname, "r");
                if (genfile != NULL) {
                        if (options->verbose) {
                                printf("Opened Genbind file %s\n", fullname);
                        }
                        free(fullname);
                        return genfile;
                }
                free(fullname);
        }

        /* try on idl path */
        if (options->idlpath != NULL) {
                fulllen = strlen(options->idlpath) + strlen(filename) + 2;
                fullname = malloc(fulllen);
                snprintf(fullname, fulllen, "%s/%s", options->idlpath, filename);
                genfile = fopen(fullname, "r");
                if ((genfile != NULL) && options->verbose) {
                        printf("Opend Genbind file %s\n", fullname);
#if 0
                        if (options->depfilehandle != NULL) {
                                fprintf(options->depfilehandle, " \\\n\t%s",
                                        fullname);
                        }
#endif
                }

                free(fullname);
        }

        return genfile;
}


int genbind_parsefile(char *infilename, struct genbind_node **ast)
{
        FILE *infile;
        int ret;

        /* open input file */
        infile = genbindopen(infilename);
        if (!infile) {
                fprintf(stderr, "Error opening %s: %s\n",
                        infilename,
                        strerror(errno));
                return 3;
        }

        /* if debugging enabled enable parser tracing and send to file */
        if (options->debug) {
                nsgenbind_debug = 1;
                nsgenbind__flex_debug = 1;
                genbind_parsetracef = genb_fopen("binding-trace", "w");
        } else {
                genbind_parsetracef = NULL;
        }

        /* set flex to read from file */
        nsgenbind_restart(infile);

        /* process binding */
        ret = nsgenbind_parse(infilename, ast);

        /* close tracefile if open */
        if (genbind_parsetracef != NULL) {
                fclose(genbind_parsetracef);
        }

        return ret;
}

int genbind_fprintf(FILE *stream, const char *format, ...)
{
        va_list ap;
        int ret;

        va_start(ap, format);

        if (genbind_parsetracef == NULL) {
                ret = vfprintf(stream, format, ap);
        } else {
                ret = vfprintf(genbind_parsetracef, format, ap);
        }
        va_end(ap);

        return ret;
}
