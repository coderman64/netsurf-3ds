/* intermediate representation of WebIDL and binding data
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_ir_h
#define nsgenbind_ir_h

struct genbind_node;
struct webidl_node;

/**
 * map entry for each argument of an overload on an operation
 */
struct ir_operation_argument_entry {
        const char *name;

        int optionalc; /**< 1 if the argument is optional */
        int elipsisc; /**< 1 if the argument is an elipsis */

        struct webidl_node *node;
};

/**
 * map entry for each overload of an operation.
 */
struct ir_operation_overload_entry {
        struct webidl_node *type; /**< The return type of this overload */

        int optionalc; /**< Number of parameters that are optional */
        int elipsisc; /**< Number of elipsis parameters */

        int argumentc; /**< the number of parameters */
        struct ir_operation_argument_entry *argumentv;
};

/** map entry for operations on an interface */
struct ir_operation_entry {
        const char *name; /** operation name */
        struct webidl_node *node; /**< AST operation node */
        struct genbind_node *method; /**< method from binding */

        int overloadc; /**< Number of overloads of this operation */
        struct ir_operation_overload_entry *overloadv;
};

/**
 * ir entry for type of attributes or arguments.
 */
struct ir_type_entry {
        enum webidl_type base; /**< base of the type (long, short, user etc.) */
        enum webidl_type_modifier modifier; /**< modifier for the type */
        bool nullable; /**< the type is nullable */
        const char *name; /**< name of type for user types */
};

/**
 * ir entry for attributes on an interface
 */
struct ir_attribute_entry {
        const char *name; /**< attribute name */
        struct webidl_node *node; /**< AST attribute node */

        int typec; /**< number of types for attribute  */
        struct ir_type_entry *typev; /**< types on attribute */

        enum webidl_type_modifier modifier; /**< modifier for the attribute intself */
        const char *putforwards; /**< putforwards attribute */
        const char *treatnullas; /**< treatnullas attribute */

        struct genbind_node *getter; /**< getter from binding */
        struct genbind_node *setter; /**< getter from binding */

        char *property_name; /**< the attribute name converted to output
                              * appropriate value. e.g. generators targetting c
                              * might lowercase the name or add underscores
                              * instead of caps
                              */
};

/**
 * map entry for constants on an interface
 */
struct ir_constant_entry {
        const char *name; /** attribute name */
        struct webidl_node *node; /**< AST constant node */
};


/** map entry for an interface */
struct ir_interface_entry {
        bool noobject; /**< flag indicating if no interface object should eb
                        * generated. This allows for interfaces which do not
                        * generate code. For implements (mixin) interfaces
                        */
        bool primary_global; /**< flag indicating the interface is the primary
                             * global javascript object.
                             */

        int operationc; /**< number of operations on interface */
        struct ir_operation_entry *operationv;

        int attributec; /**< number of attributes on interface */
        struct ir_attribute_entry *attributev;

        int constantc; /**< number of constants on interface */
        struct ir_constant_entry *constantv;
};

/**
 * map entry for a dictionary
 */
struct ir_dictionary_entry {
        int memberc; /**< the number of members */
        struct ir_operation_argument_entry *memberv;
};

enum ir_entry_type {
        IR_ENTRY_TYPE_INTERFACE,
        IR_ENTRY_TYPE_DICTIONARY,
};

enum ir_init_argtype {
	IR_INIT_ARG_POINTER,
	IR_INIT_ARG_UNSIGNED,
	IR_INIT_ARG_INT,
	IR_INIT_ARG_BOOL,
};

/** top level entry info common to interfaces and dictionaries */
struct ir_entry {
        const char *name; /** IDL name */
        struct webidl_node *node; /**< AST node */
        const char *inherit_name; /**< Name of interface inhertited from */
        struct genbind_node *class; /**< class from binding (if any) */

        enum ir_entry_type type;
        union {
                struct ir_dictionary_entry dictionary;
                struct ir_interface_entry interface;
        } u;

        int inherit_idx; /**< index into map of inherited interface or -1 for
			  * not in map
			  */
	int refcount; /**< number of interfacess in map that refer to this
		       * interface
		       */

        /* The variables are created and used by the output generation but
         * rather than have another allocation and pointer the data they are
         * just inline here.
         */

        char *filename; /**< filename used for output */

        char *class_name; /**< the interface name converted to output
                           * appropriate value. e.g. generators targetting c
                           * might lowercase the name or add underscores
                           * instead of caps
                           */
        int class_init_argc; /**< The number of parameters on the class
                              * initializer.
                              */
	enum ir_init_argtype *class_init_argt; /**< The types of the initialiser parameters */
};

/** intermediate representation of WebIDL and binding data */
struct ir {
        int entryc; /**< count of entries */
        struct ir_entry *entries; /**< interface entries */

        /** The AST node of the binding information */
        struct genbind_node *binding_node;

        /** Root AST node of the webIDL */
        struct webidl_node *webidl;
};

/**
 * Create a new interface map
 */
int ir_new(struct genbind_node *genbind,
                      struct webidl_node *webidl,
                      struct ir **map_out);

int ir_dump(struct ir *map);

int ir_dumpdot(struct ir *map);

/**
 * interface map parent entry
 *
 * \return inherit entry or NULL if there is not one
 */
struct ir_entry *ir_inherit_entry(struct ir *map, struct ir_entry *entry);

#endif
