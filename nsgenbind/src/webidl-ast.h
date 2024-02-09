/* Web IDL AST interface 
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_webidl_ast_h
#define nsgenbind_webidl_ast_h

enum webidl_node_type {
	/* generic node types which define structure or attributes */
	WEBIDL_NODE_TYPE_ROOT = 0,
	WEBIDL_NODE_TYPE_IDENT,
	/** access modifier e.g. for attributes or types */
	WEBIDL_NODE_TYPE_MODIFIER,
	/** a list of nodes (interface members, arguments)  */
	WEBIDL_NODE_TYPE_LIST, 

        /* non structural node types */
	WEBIDL_NODE_TYPE_INTERFACE, /**< node is an interface*/
	WEBIDL_NODE_TYPE_INTERFACE_IMPLEMENTS,

	WEBIDL_NODE_TYPE_ATTRIBUTE,
	WEBIDL_NODE_TYPE_OPERATION,
	WEBIDL_NODE_TYPE_CONST,

	WEBIDL_NODE_TYPE_DICTIONARY, /**< node is a dictionary */

	WEBIDL_NODE_TYPE_INHERITANCE, /**< node has inheritance */
	WEBIDL_NODE_TYPE_SPECIAL,
	WEBIDL_NODE_TYPE_ARGUMENT,
        WEBIDL_NODE_TYPE_OPTIONAL,
	WEBIDL_NODE_TYPE_ELLIPSIS,
	WEBIDL_NODE_TYPE_TYPE,
	WEBIDL_NODE_TYPE_TYPE_BASE,
	WEBIDL_NODE_TYPE_TYPE_NULLABLE,
	WEBIDL_NODE_TYPE_TYPE_ARRAY,

	WEBIDL_NODE_TYPE_LITERAL_NULL,
	WEBIDL_NODE_TYPE_LITERAL_INT,
	WEBIDL_NODE_TYPE_LITERAL_BOOL,
	WEBIDL_NODE_TYPE_LITERAL_FLOAT,
	WEBIDL_NODE_TYPE_LITERAL_STRING,

	WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE,

};

enum webidl_type {
        WEBIDL_TYPE_ANY, /**< 0 - The type is unconstrained */
	WEBIDL_TYPE_USER, /**< 1 - The type is a dictionary or interface */
	WEBIDL_TYPE_BOOL, /**< 2 - The type is boolean */
	WEBIDL_TYPE_BYTE, /**< 3 - The type is a byte */
	WEBIDL_TYPE_OCTET, /**< 4 - The type is a octet */
	WEBIDL_TYPE_FLOAT, /**< 5 - The type is a float point number */
	WEBIDL_TYPE_DOUBLE, /**< 6 - The type is a double */
	WEBIDL_TYPE_SHORT, /**< 7 - The type is a signed 16bit */
	WEBIDL_TYPE_LONG, /**< 8 - The type is a signed 32bit */
	WEBIDL_TYPE_LONGLONG, /**< 9 - The type is a signed 64bit */
	WEBIDL_TYPE_STRING, /**< 10 - The type is a string */
	WEBIDL_TYPE_SEQUENCE, /**< 11 - The type is a sequence */
	WEBIDL_TYPE_OBJECT, /**< 12 - The type is a object */
	WEBIDL_TYPE_DATE, /**< 13 - The type is a date */
	WEBIDL_TYPE_VOID, /**< 14 - The type is void */
};

/** modifiers for operations, attributes and arguments */
enum webidl_type_modifier {
        WEBIDL_TYPE_MODIFIER_NONE,
	WEBIDL_TYPE_MODIFIER_UNSIGNED,
	WEBIDL_TYPE_MODIFIER_UNRESTRICTED,
	WEBIDL_TYPE_MODIFIER_READONLY,
	WEBIDL_TYPE_MODIFIER_STATIC, /**< operation or attribute is static */
	WEBIDL_TYPE_MODIFIER_INHERIT, /**< attribute inherits */
};

/* the type of special node */
enum webidl_type_special {
        WEBIDL_TYPE_SPECIAL_GETTER,
        WEBIDL_TYPE_SPECIAL_SETTER,
        WEBIDL_TYPE_SPECIAL_CREATOR,
        WEBIDL_TYPE_SPECIAL_DELETER,
        WEBIDL_TYPE_SPECIAL_LEGACYCALLER,
};

struct webidl_node;

/** callback for search and iteration routines */
typedef int (webidl_callback_t)(struct webidl_node *node, void *ctx);

int webidl_cmp_node_type(struct webidl_node *node, void *ctx);

/**
 * create a new node with a pointer value
 */
struct webidl_node *webidl_node_new(enum webidl_node_type, struct webidl_node *l, void *r);

/**
 * create a new node with an integer value
 */
struct webidl_node *webidl_new_number_node(enum webidl_node_type type, struct webidl_node *l, int number);


struct webidl_node *webidl_node_prepend(struct webidl_node *list, struct webidl_node *node);
struct webidl_node *webidl_node_append(struct webidl_node *list, struct webidl_node *node);

struct webidl_node *webidl_node_add(struct webidl_node *node, struct webidl_node *list);

/* node contents acessors */
char *webidl_node_gettext(struct webidl_node *node);
struct webidl_node *webidl_node_getnode(struct webidl_node *node);
int *webidl_node_getint(struct webidl_node *node);
float *webidl_node_getfloat(struct webidl_node *node);

enum webidl_node_type webidl_node_gettype(struct webidl_node *node);

/* node searches */

/**
 * Iterate nodes children matching their type.
 *
 * For each child node where the type is matched the callback function is
 * called with a context value.
 */
int webidl_node_for_each_type(struct webidl_node *node,
                              enum webidl_node_type type,
                              webidl_callback_t *cb,
			      void *ctx);

int webidl_node_enumerate_type(struct webidl_node *node,
                               enum webidl_node_type type);

struct webidl_node *
webidl_node_find(struct webidl_node *node,
                 struct webidl_node *prev,
                 webidl_callback_t *cb,
		 void *ctx);

struct webidl_node *
webidl_node_find_type(struct webidl_node *node,
                      struct webidl_node *prev,
		      enum webidl_node_type type);

struct webidl_node *
webidl_node_find_type_ident(struct webidl_node *root_node, 
			    enum webidl_node_type type, 
			    const char *ident);



/**
 * parse web idl file into Abstract Syntax Tree
 */
int webidl_parsefile(char *filename, struct webidl_node **webidl_ast);

/**
 * dump AST to file
 */
int webidl_dump_ast(struct webidl_node *node);

/**
 * perform replacement of implements elements with copies of ast data
 */
int webidl_intercalate_implements(struct webidl_node *node);

/**
 * formatted printf to allow webidl trace data to be written to file.
 */
int webidl_fprintf(FILE *stream, const char *format, ...);

/**
 * get string of argument type
 */
const char *webidl_type_to_str(enum webidl_type_modifier m, enum webidl_type t);

#endif
