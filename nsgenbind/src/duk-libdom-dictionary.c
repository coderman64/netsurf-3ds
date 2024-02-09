/* duktape binding generation implementation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>

#include "options.h"
#include "utils.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "ir.h"
#include "output.h"
#include "duk-libdom.h"

/** prefix for all generated functions */
#define DLPFX "dukky"

#define MAGICPFX "\\xFF\\xFFNETSURF_DUKTAPE_"


/**
 * get default value as a string
 */
static int
get_member_default_str(struct ir_entry *dictionarye,
                       struct ir_operation_argument_entry *membere,
                       enum webidl_type member_type,
                       char **defl_out)
{
        struct webidl_node *lit_node;
        enum webidl_node_type lit_type;
        int *lit_int;
        float *lit_flt;

        lit_node = webidl_node_getnode(
                webidl_node_find_type(
                        webidl_node_getnode(membere->node),
                        NULL,
                        WEBIDL_NODE_TYPE_OPTIONAL));
        if (lit_node == NULL) {
                *defl_out = NULL;
                return 0;
        }

        lit_type = webidl_node_gettype(lit_node);

        switch (lit_type) {

        case WEBIDL_NODE_TYPE_LITERAL_BOOL:
                if (member_type != WEBIDL_TYPE_BOOL) {
                        fprintf(stderr,
                                "Dictionary %s:%s literal boolean type mismatch\n",
                                dictionarye->name,
                                membere->name);
                        return -1;
                }
                lit_int = webidl_node_getint(lit_node);
                if (*lit_int == 0) {
                        *defl_out = strdup("false");
                } else {
                        *defl_out = strdup("true");
                }
                break;

        case WEBIDL_NODE_TYPE_LITERAL_NULL:
                *defl_out = strdup("NULL");
                break;

        case WEBIDL_NODE_TYPE_LITERAL_STRING:
                *defl_out = strdup(webidl_node_gettext(lit_node));
                break;

        case WEBIDL_NODE_TYPE_LITERAL_INT:
                lit_int = webidl_node_getint(lit_node);
                *defl_out = malloc(128);
                snprintf(*defl_out, 128, "%d", *lit_int);
                break;

        case WEBIDL_NODE_TYPE_LITERAL_FLOAT:
                lit_flt = webidl_node_getfloat(lit_node);
                *defl_out = malloc(128);
                snprintf(*defl_out, 128, "%f", *lit_flt);
                break;

        default:
                *defl_out = NULL;
                break;
        }

        return 0;
}


/**
 * generate a single class method for an interface operation
 */
static int
output_member_acessor(struct opctx *outc,
                      struct ir_entry *dictionarye,
                      struct ir_operation_argument_entry *membere)
{
        struct webidl_node *type_node;
        enum webidl_type *argument_type;
        char *defl; /* default for member */
        int res;

        type_node = webidl_node_find_type(
                webidl_node_getnode(membere->node),
                NULL,
                WEBIDL_NODE_TYPE_TYPE);

        if (type_node == NULL) {
                fprintf(stderr, "%s:%s has no type\n",
                        dictionarye->name,
                        membere->name);
                return -1;
        }

        argument_type = (enum webidl_type *)webidl_node_getint(
                webidl_node_find_type(
                        webidl_node_getnode(type_node),
                        NULL,
                        WEBIDL_NODE_TYPE_TYPE_BASE));

        if (argument_type == NULL) {
                fprintf(stderr,
                        "%s:%s has no type base\n",
                        dictionarye->name,
                        membere->name);
                return -1;
        }

        /* get default text */
        res = get_member_default_str(dictionarye, membere, *argument_type, &defl);
        if (res != 0) {
                return res;
        }

        switch (*argument_type) {

        case WEBIDL_TYPE_STRING:
                outputf(outc,
                        "const char *\n"
                        "%s_%s_get_%s(duk_context *ctx, duk_idx_t idx)\n"
                        "{\n",
                        DLPFX, dictionarye->class_name, membere->name);

                if (defl == NULL) {
                        outputf(outc,
                                "\tconst char *ret = NULL; /* No default */\n");
                } else {
                        outputf(outc,
                                "\tconst char *ret = \"%s\"; /* Default value of %s */\n",
                                defl, membere->name);
                }

                outputf(outc,
                        "\t/* ... obj@idx ... */\n"
                        "\tduk_get_prop_string(ctx, idx, \"%s\");\n"
                        "\t/* ... obj@idx ... value/undefined */\n"
                        "\tif (!duk_is_undefined(ctx, -1)) {\n"
                        "\t\t/* Note, this throws a duk_error if it's not a string */\n"
                        "\t\tret = duk_require_string(ctx, -1);\n"
                        "\t}\n"
                        "\tduk_pop(ctx);\n"
                        "\treturn ret;\n"
                        "}\n\n",
                        membere->name);

                break;

        case WEBIDL_TYPE_BOOL:
                outputf(outc,
                        "duk_bool_t\n"
                        "%s_%s_get_%s(duk_context *ctx, duk_idx_t idx)\n"
                        "{\n",
                        DLPFX, dictionarye->class_name, membere->name);

                if (defl == NULL) {
                        outputf(outc,
                                "\tduk_bool_t ret = false; /* No default */\n");
                } else {
                outputf(outc,
                        "\tduk_bool_t ret = %s; /* Default value of %s */\n",
                        defl, membere->name);
                }

                outputf(outc,
                        "\t/* ... obj@idx ... */\n"
                        "\tduk_get_prop_string(ctx, idx, \"%s\");\n"
                        "\t/* ... obj@idx ... value/undefined */\n"
                        "\tif (!duk_is_undefined(ctx, -1)) {\n"
                        "\t\t/* Note, this throws a duk_error if it's not a boolean */\n"
                        "\t\tret = duk_require_boolean(ctx, -1);\n"
                        "\t}\n"
                        "\tduk_pop(ctx);\n"
                        "\treturn ret;\n"
                        "}\n\n",
                        membere->name);

                break;

        case WEBIDL_TYPE_SHORT:
        case WEBIDL_TYPE_LONG:
        case WEBIDL_TYPE_LONGLONG:
                outputf(outc,
                        "duk_int_t\n"
                        "%s_%s_get_%s(duk_context *ctx, duk_idx_t idx)\n"
                        "{\n",
                        DLPFX, dictionarye->class_name, membere->name);

                if (defl == NULL) {
                        outputf(outc,
                                "\tduk_int_t ret = 0; /* No default */\n");
                } else {
                        outputf(outc,
                                "\tduk_int_t ret = %s; /* Default value of %s */\n",
                                defl, membere->name);
                }

                outputf(outc,
                        "\t/* ... obj@idx ... */\n"
                        "\tduk_get_prop_string(ctx, idx, \"%s\");\n"
                        "\t/* ... obj@idx ... value/undefined */\n"
                        "\tif (!duk_is_undefined(ctx, -1)) {\n"
                        "\t\t/* Note, this throws a duk_error if it's not a int */\n"
                        "\t\tret = duk_require_int(ctx, -1);\n"
                        "\t}\n"
                        "\tduk_pop(ctx);\n"
                        "\treturn ret;\n"
                        "}\n\n",
                        membere->name);
                break;

        case WEBIDL_TYPE_FLOAT:
        case WEBIDL_TYPE_DOUBLE:
                outputf(outc,
                        "duk_double_t\n"
                        "%s_%s_get_%s(duk_context *ctx, duk_idx_t idx)\n",
                        DLPFX, dictionarye->class_name, membere->name);

                outputf(outc,
                        "{\n"
                        "\tduk_double_t ret = %s; /* Default value of %s */\n"
                        "\t/* ... obj@idx ... */\n"
                        "\tduk_get_prop_string(ctx, idx, \"%s\");\n",
                        defl, membere->name, membere->name);

                outputf(outc,
                        "\t/* ... obj@idx ... value/undefined */\n"
                        "\tif (!duk_is_undefined(ctx, -1)) {\n"
                        "\t\t/* Note, this throws a duk_error if it's not a number */\n"
                        "\t\tret = duk_require_number(ctx, -1);\n"
                        "\t}\n"
                        "\tduk_pop(ctx);\n"
                        "\treturn ret;\n"
                        "}\n\n");
                break;

        default:
                WARN(WARNING_UNIMPLEMENTED,
                        "Dictionary %s:%s unhandled type (%d)",
                        dictionarye->name,
                        membere->name,
                        *argument_type);
                outputf(outc,
                        "/* Dictionary %s:%s unhandled type (%d) */\n\n",
                        dictionarye->name,
                        membere->name,
                        *argument_type);
        }

        if (defl != NULL) {
                free(defl);
        }

        return 0;
}

static int
output_member_acessors(struct opctx *outc, struct ir_entry *dictionarye)
{
        int memberc;
        int res = 0;

        for (memberc = 0;
             memberc < dictionarye->u.dictionary.memberc;
             memberc++) {
                res = output_member_acessor(
                        outc,
                        dictionarye,
                        dictionarye->u.dictionary.memberv + memberc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}


/* exported function documented in duk-libdom.h */
int output_dictionary(struct ir *ir, struct ir_entry *dictionarye)
{
        struct opctx *dyop;
        int res = 0;

        /* open the output */
        res = output_open(dictionarye->filename, &dyop);
        if (res != 0) {
                return res;
        }

        /* tool preface */
        output_tool_preface(dyop);

        /* binding preface */
        output_method_cdata(dyop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* class preface */
        output_method_cdata(dyop,
                            dictionarye->class,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* tool prologue */
        output_tool_prologue(dyop);

        /* binding prologue */
        output_method_cdata(dyop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        /* class prologue */
        output_method_cdata(dyop,
                            dictionarye->class,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        outputf(dyop, "\n");

        res = output_member_acessors(dyop, dictionarye);
        if (res != 0) {
                goto op_error;
        }

        outputf(dyop, "\n");

        /* class epilogue */
        output_method_cdata(dyop,
                            dictionarye->class,
                            GENBIND_METHOD_TYPE_EPILOGUE);

        /* binding epilogue */
        output_method_cdata(dyop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_EPILOGUE);

        /* class postface */
        output_method_cdata(dyop,
                            dictionarye->class,
                            GENBIND_METHOD_TYPE_POSTFACE);

        /* binding postface */
        output_method_cdata(dyop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_POSTFACE);

op_error:
        output_close(dyop);

        return res;
}

/**
 * generate a single class method declaration for an interface operation
 */
static int
output_member_declaration(struct opctx *outc,
                          struct ir_entry *dictionarye,
                          struct ir_operation_argument_entry *membere)
{
        struct webidl_node *type_node;
        enum webidl_type *argument_type;

        type_node = webidl_node_find_type(
                webidl_node_getnode(membere->node),
                NULL,
                WEBIDL_NODE_TYPE_TYPE);

        if (type_node == NULL) {
                fprintf(stderr, "%s:%s has no type\n",
                        dictionarye->name,
                        membere->name);
                return -1;
        }

        argument_type = (enum webidl_type *)webidl_node_getint(
                webidl_node_find_type(
                        webidl_node_getnode(type_node),
                        NULL,
                        WEBIDL_NODE_TYPE_TYPE_BASE));

        if (argument_type == NULL) {
                fprintf(stderr,
                        "%s:%s has no type base\n",
                        dictionarye->name,
                        membere->name);
                return -1;
        }


        switch (*argument_type) {

        case WEBIDL_TYPE_STRING:
                outputf(outc,
                        "const char *%s_%s_get_%s(duk_context *ctx, duk_idx_t idx);\n",
                        DLPFX, dictionarye->class_name, membere->name);
                break;

        case WEBIDL_TYPE_BOOL:
                outputf(outc,
                        "duk_bool_t %s_%s_get_%s(duk_context *ctx, duk_idx_t idx);\n",
                        DLPFX, dictionarye->class_name, membere->name);
                break;

        case WEBIDL_TYPE_SHORT:
        case WEBIDL_TYPE_LONG:
        case WEBIDL_TYPE_LONGLONG:
                outputf(outc,
                        "duk_int_t %s_%s_get_%s(duk_context *ctx, duk_idx_t idx);\n",
                        DLPFX, dictionarye->class_name, membere->name);
                break;

        case WEBIDL_TYPE_FLOAT:
        case WEBIDL_TYPE_DOUBLE:
                outputf(outc,
                        "duk_double_t %s_%s_get_%s(duk_context *ctx, duk_idx_t idx);\n",
                        DLPFX, dictionarye->class_name, membere->name);
                break;

        default:
                outputf(outc,
                        "/* Dictionary %s:%s unhandled type (%d) */\n",
                        dictionarye->name,
                        membere->name,
                        *argument_type);
        }

        return 0;
}

/* exported function documented in duk-libdom.h */
int
output_dictionary_declaration(struct opctx *outc, struct ir_entry *dictionarye)
{
        int memberc;
        int res = 0;

        for (memberc = 0;
             memberc < dictionarye->u.dictionary.memberc;
             memberc++) {
                res = output_member_declaration(
                        outc,
                        dictionarye,
                        dictionarye->u.dictionary.memberv + memberc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}
