/* duktape and libdom binding generation implementation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

/**
 * \file
 * functions that automatically generate binding contents based on heuristics
 * with explicit knowledge about libdom and how IDL data types map to it.
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

static int
output_generated_attribute_user_getter(struct opctx *outc,
                                       struct ir_entry *interfacee,
                                       struct ir_attribute_entry *atributee)
{
        UNUSED(interfacee);

        if ((atributee->typev[0].name != NULL) &&
            strlen(atributee->typev[0].name) >= 12 &&
            strcmp(atributee->typev[0].name + strlen(atributee->typev[0].name) - 12,
                   "EventHandler") == 0) {

                /* this can generate for onxxx event handlers */
                if ((atributee->name[0] != 'o') ||
                    (atributee->name[1] != 'n')) {
                        return -1; /* not onxxx */
                }

                if (interfacee->u.interface.primary_global) {
                        outputf(outc,
                                "\tdom_event_target *et = NULL;\n");
                } else {
                        outputf(outc,
                                "\tdom_event_target *et = (dom_event_target *)(((node_private_t *)priv)->node);\n");
                }
                outputf(outc,
                        "\tdom_string *name;\n"
                        "\tdom_exception exc;\n\n"
                        "\texc = dom_string_create((const uint8_t *)\"%s\", %ld, &name);\n"
                        "\tif (exc != DOM_NO_ERR) return 0;\n\n"
                        "\tduk_push_this(ctx);\n"
                        "\t/* ... node */\n"
                        "\tif (dukky_get_current_value_of_event_handler(ctx, name, et) == false) {\n"
                        "\t\tdom_string_unref(name);\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\tdom_string_unref(name);\n"
                        "\t/* ... handler node */\n"
                        "\tduk_pop(ctx);\n"
                        "\t/* ... handler */\n"
                        "\treturn 1;\n",
                        atributee->name + 2,
                        strlen(atributee->name + 2));
                return 0;
        }
        return -1;
}

/* exported function documented in duk-libdom.h */
int
output_generated_attribute_getter(struct opctx *outc,
                                  struct ir_entry *interfacee,
                                  struct ir_attribute_entry *atributee)
{
        int res = 0;

        /* generation can only cope with a single type on the attribute */
        if (atributee->typec != 1) {
                return -1;
        }

        switch (atributee->typev[0].base) {
        case WEBIDL_TYPE_STRING:
                outputf(outc,
                        "\tdom_exception exc;\n"
                        "\tdom_string *str;\n"
                        "\n");
                outputf(outc,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &str);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tif (str != NULL) {\n"
                        "\t\tduk_push_lstring(ctx,\n"
                        "\t\t\tdom_string_data(str),\n"
                        "\t\t\tdom_string_length(str));\n"
                        "\t\tdom_string_unref(str);\n"
                        "\t} else {\n");
                if (atributee->typev[0].nullable) {
                        outputf(outc,
                                "\t\tduk_push_null(ctx);\n");
                } else {
                        outputf(outc,
                                "\t\tduk_push_lstring(ctx, NULL, 0);\n");
                }
                outputf(outc,
                        "\t}\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        case WEBIDL_TYPE_LONG:
                if (atributee->typev[0].modifier == WEBIDL_TYPE_MODIFIER_UNSIGNED) {
                        outputf(outc,
                                "\tdom_ulong l;\n");
                } else {
                        outputf(outc,
                                "\tdom_long l;\n");
                }
                outputf(outc,
                        "\tdom_exception exc;\n"
                        "\n");
                outputf(outc,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &l);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tduk_push_number(ctx, (duk_double_t)l);\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        case WEBIDL_TYPE_SHORT:
                if (atributee->typev[0].modifier == WEBIDL_TYPE_MODIFIER_UNSIGNED) {
                        outputf(outc,
                                "\tdom_ushort s;\n");
                } else {
                        outputf(outc,
                                "\tdom_short s;\n");
                }
                outputf(outc,
                        "\tdom_exception exc;\n"
                        "\n");
                outputf(outc,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &s);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tduk_push_number(ctx, (duk_double_t)s);\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        case WEBIDL_TYPE_BOOL:
                outputf(outc,
                        "\tdom_exception exc;\n"
                        "\tbool b;\n"
                        "\n");
                outputf(outc,
                        "\texc = dom_%s_get_%s((struct dom_%s *)((node_private_t*)priv)->node, &b);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\tduk_push_boolean(ctx, b);\n"
                        "\n"
                        "\treturn 1;\n");
                break;

        case WEBIDL_TYPE_USER:
                res = output_generated_attribute_user_getter(outc,
                                                             interfacee,
                                                             atributee);
                break;

        default:
                res = -1;
                break;

        }

        if (res >= 0) {
                WARN(WARNING_GENERATED,
                     "Generated: getter %s::%s();",
                     interfacee->name, atributee->name);
        }

        return res;
}

static int
output_generated_attribute_user_setter(struct opctx *outc,
                                       struct ir_entry *interfacee,
                                       struct ir_attribute_entry *atributee)
{
        UNUSED(interfacee);

        if ((atributee->typev[0].name != NULL) &&
            strlen(atributee->typev[0].name) >= 12 &&
            strcmp(atributee->typev[0].name + strlen(atributee->typev[0].name) - 12,
                   "EventHandler") == 0) {

                /* this can generate for onxxx event handlers */
                if ((atributee->name[0] != 'o') ||
                    (atributee->name[1] != 'n')) {
                        return -1; /* not onxxx */
                }

                if (interfacee->u.interface.primary_global) {
                        outputf(outc,
                                "\tdom_element *et = NULL;\n");
                } else {
                        outputf(outc,
                                "\tdom_element *et = (dom_element *)(((node_private_t *)priv)->node);\n");
                }

                outputf(outc,
                        "\t/* handlerfn */\n"
                        "\tduk_push_this(ctx);\n"
                        "\t/* handlerfn this */\n"
                        "\tduk_get_prop_string(ctx, -1, HANDLER_MAGIC);\n"
                        "\t/* handlerfn this handlers */\n"
                        "\tduk_push_lstring(ctx, \"%s\", %ld);\n"
                        "\t/* handlerfn this handlers %s */\n"
                        "\tduk_dup(ctx, -4);\n"
                        "\t/* handlerfn this handlers %s handlerfn */\n"
                        "\tduk_put_prop(ctx, -3);\n"
                        "\t/* handlerfn this handlers */\n"
                        "\tdukky_register_event_listener_for(ctx, et,\n"
                        "\t\tcorestring_dom_%s, false);\n"
                        "\treturn 0;\n",
                        atributee->name + 2,
                        strlen(atributee->name + 2),
                        atributee->name + 2,
                        atributee->name + 2,
                        atributee->name + 2);
                return 0;
        }
        return -1;
}


/* exported function documented in duk-libdom.h */
int
output_generated_attribute_setter(struct opctx *outc,
                                  struct ir_entry *interfacee,
                                  struct ir_attribute_entry *atributee)
{
        int res = 0;

        /* generation can only cope with a single type on the attribute */
        if (atributee->typec != 1) {
                return -1;
        }

        switch (atributee->typev[0].base) {
        case WEBIDL_TYPE_STRING:
                outputf(outc,
                        "\tdom_exception exc;\n"
                        "\tdom_string *str;\n"
                        "\tduk_size_t slen;\n"
                        "\tconst char *s;\n");
                if ((atributee->treatnullas != NULL) &&
                    (strcmp(atributee->treatnullas, "EmptyString") == 0)) {
                        outputf(outc,
                                "\tif (duk_is_null(ctx, 0)) {\n"
                                "\t\ts = \"\";\n"
                                "\t\tslen = 0;\n"
                                "\t} else {\n"
                                "\t\ts = duk_safe_to_lstring(ctx, 0, &slen);\n"
                                "\t}\n");
                } else {
                        outputf(outc,
                                "\ts = duk_safe_to_lstring(ctx, 0, &slen);\n");
                }
                outputf(outc,
                        "\n"
                        "\texc = dom_string_create((const uint8_t *)s, slen, &str);\n"
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n");
                outputf(outc,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, str);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tdom_string_unref(str);\n"
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        case WEBIDL_TYPE_LONG:
                if (atributee->typev[0].modifier == WEBIDL_TYPE_MODIFIER_UNSIGNED) {
                        outputf(outc,
                                "\tdom_exception exc;\n"
                                "\tdom_ulong l;\n"
                                "\n"
                                "\tl = duk_get_uint(ctx, 0);\n"
                                "\n");
                } else {
                        outputf(outc,
                                "\tdom_exception exc;\n"
                                "\tdom_long l;\n"
                                "\n"
                                "\tl = duk_get_int(ctx, 0);\n"
                                "\n");
                }
                outputf(outc,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, l);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        case WEBIDL_TYPE_SHORT:
                if (atributee->typev[0].modifier == WEBIDL_TYPE_MODIFIER_UNSIGNED) {
                        outputf(outc,
                                "\tdom_exception exc;\n"
                                "\tdom_ushort s;\n"
                                "\n"
                                "\ts = duk_get_uint(ctx, 0);\n"
                                "\n");
                } else {
                        outputf(outc,
                                "\tdom_exception exc;\n"
                                "\tdom_short s;\n"
                                "\n"
                                "\ts = duk_get_int(ctx, 0);\n"
                                "\n");
                }
                outputf(outc,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, s);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        case WEBIDL_TYPE_BOOL:
                outputf(outc,
                        "\tdom_exception exc;\n"
                        "\tbool b;\n"
                        "\n"
                        "\tb = duk_get_boolean(ctx, 0);\n"
                        "\n");
                outputf(outc,
                        "\texc = dom_%s_set_%s((struct dom_%s *)((node_private_t*)priv)->node, b);\n",
                        interfacee->class_name,
                        atributee->property_name,
                        interfacee->class_name);
                outputf(outc,
                        "\tif (exc != DOM_NO_ERR) {\n"
                        "\t\treturn 0;\n"
                        "\t}\n"
                        "\n"
                        "\treturn 0;\n");
                break;

        case WEBIDL_TYPE_USER:
                res = output_generated_attribute_user_setter(outc,
                                                             interfacee,
                                                             atributee);
                break;

        default:
                res = -1;
                break;

        }

        if (res >= 0) {
                WARN(WARNING_GENERATED,
                     "Generated: getter %s::%s();",
                     interfacee->name, atributee->name);
        }

        return res;
}
