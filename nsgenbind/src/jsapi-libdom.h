/* binding generator output
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_jsapi_libdom_h
#define nsgenbind_jsapi_libdom_h

struct options;

struct binding_interface {
	const char *name; /**< name of interface */
	struct genbind_node *node; /**< node of interface in binding */
	struct webidl_node *widl_node; /**< node of interface in webidl */
	const char *inherit_name; /**< name of interface this inherits from */
	int own_properties; /**< the number of properties the interface has */
	int own_functions; /**< the number of functions the interface has */

	bool has_type_properties; /**< some of the properties on the
				   * interface have a type handler
				   */

	int inherit_idx; /**< index into binding map of inherited
			  * interface or -1 for not in map
			  */
	int refcount; /**< number of entries in map that refer to this
		       * interface
		       */
	int output_idx; /**< for interfaces that will be output (node
			 * is valid) this is the output array index
			 */
};

struct binding {
	struct genbind_node *gb_ast; /* root node of binding AST */
	struct webidl_node *wi_ast; /* root node of webidl AST */

	const char *name; /* Name of binding (first interface name by default) */
	int interfacec; /* numer of interfaces in the interface map */
	struct binding_interface *interfaces; /* binding interface map */

	const char *interface; /* webidl interface binding is for */

	bool has_private; /* true if the binding requires a private structure */
	struct genbind_node *binding_list; /* node list of the binding */

	struct genbind_node *addproperty; /* binding api add property node or NULL */
	struct genbind_node *delproperty; /* binding api delete property node or NULL */
	struct genbind_node *getproperty; /* binding api get property node or NULL */
	struct genbind_node *setproperty; /* binding api set property node or NULL */
	struct genbind_node *enumerate; /* binding api enumerate node or NULL */
	struct genbind_node *resolve; /* binding api resolve node or NULL */
	struct genbind_node *finalise; /* binding api finalise node or NULL */
	struct genbind_node *mark; /* binding api mark node or NULL */

	const char *hdrguard; /* header file guard name */

	FILE *outfile ; /* file handle output should be written to,
			 * allows reuse of callback routines to output
			 * to headers and source files
			 */
	FILE *srcfile ; /* output source file */
	FILE *hdrfile ; /* output header file */
};

/** Generate binding between jsapi and netsurf libdom */
int jsapi_libdom_output(struct options *options, struct genbind_node *genbind_ast, struct genbind_node *binding_node);

/** Build interface map.
 *
 * Generate a map of all interfaces referenced from a binding and
 * their relationships to each other.
 *
 * The map will contain all the interfaces both directly referenced by
 * the binding and all those inherited through the WebIDL.
 *
 * The map is topoligicaly sorted to ensure no forward inheritance
 * references.
 *
 * The map contains an monotinicaly incrementing index for all
 * interfaces referenced in the binding (i.e. those to be exported).
 */
int build_interface_map(struct genbind_node *binding_node,
			struct webidl_node *webidl_ast,
			int *interfacec_out,
			struct binding_interface **interfaces_out);


/** output code block from a node */
void output_code_block(struct binding *binding, struct genbind_node *codelist);

/** generate classes */
int output_jsclasses(struct binding *binding);

/* Generate jsapi native function specifiers */
int output_function_spec(struct binding *binding);

/**
 * Generate jsapi native function bodies.
 *
 * web IDL describes methods as operators
 * http://www.w3.org/TR/WebIDL/#idl-operations
 *
 * This walks the web IDL AST to find all operator interface members
 * and construct appropriate jsapi native function body to implement
 * them.
 *
 * Function body contents can be overriden with an operator code
 * block in the binding definition.
 *
 * @param binding The binding information
 */
int output_function_bodies(struct binding *binding);

/** generate property tinyid enum */
int output_property_tinyid(struct binding *binding);

/** generate property specifier structure */
int output_property_spec(struct binding *binding);

/** generate property function bodies */
int output_property_body(struct binding *binding);

/** generate binding initialisation */
int output_class_init(struct binding *binding);

/** generate binding class constructors */
int output_class_new(struct binding *binding);


#endif
