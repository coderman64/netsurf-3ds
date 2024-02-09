nsgenbind
=========

This is a tool to generate JavaScript to DOM bindings from w3c webidl
files and a binding configuration file.

Building
--------

The tool requires bison and flex as pre-requisites

Commandline
-----------

nsgenbind [-v] [-n] [-g] [-D] [-W] [-I idlpath] inputfile outputdir

-v
  The verbose switch makes the tool verbose about what operations it
   is performing instead of the default of only reporting errors.

-n
  The tool will not generate any output but will perform all other
   operations as if it was. Useful for debugging bindings

-g
  The generated code will be augmented with runtime debug logging so
   it can be traced

-D
  The tool will generate output to allow debugging of output
   conversion.  This includes dumps of the binding and IDL files AST

-W
  This switch will make the tool generate warnings about various
   issues with the binding or IDL files being processed.

-I
  An additional search path may be given so idl files can be located.

The tool requires a binding file as input and an output directory in
 which to place its output.


Debug output
------------

In addition to the generated source the tool will output several
 debugging files with the -D switch in use.

interface.dot

  The interfaces IDL dot file contains all the interfaces and their
   relationship. graphviz can be used to convert this into a visual
   representation which is sometimes useful to help in debugging
   missing or incorrect interface inheritance.

  Processing the dot file with graphviz can produce very large files
   so care must be taken with options. Some examples that produce
   adequate output:

  # classical tree
  dot -O -Tsvg interface.dot

  # radial output
  twopi -Granksep=10.0 -Gnodesep=1.0 -Groot=0009 -O -Tsvg interface.dot


Web IDL
-------

The IDL is specified in a w3c document[1] but the second edition is in
 draft[2] and covers many of the features actually used in the whatwg
 dom and HTML spec.

The principal usage of the IDL is to define the interface between
 scripts and a browsers internal state. For example the DOM[3] and
 HTML[4] specs contain all the IDL for accessing the DOM and interacting
 with a web browser (this not strictly true as there are several
 interfaces simply not in the standards such as console).

The IDL uses some slightly strange names than other object orientated
 systems.

   IDL     |  JS              |  OOP           |  Notes
-----------+------------------+----------------+----------------------------
 interface | prototype        | class          | The data definition of
           |                  |                |  the object
 constants | read-only value  | class variable | Belong to class, one copy
           |  property on the |                |
           |  prototype       |                |
 operation | method           | method         | functions that can be called
 attribute | property         | property       | Variables set per instance
-----------+------------------+----------------+----------------------------


Binding file
------------

The binding file controls how the code generator constructs its
output. It is deliberately similar to c++ in syntax and uses OOP
nomenclature to describe the annotations (class, method, etc. instead
of interface, operation, etc.)

The binding file consists of three types of element:

  binding

      The binding element has an identifier controlling which type of
       output is produced (currently duk_libdom and jsapi_libdom).

      The binding block may contain one or more directives which
       control overall generation behaviour:

      webidl

          This takes a quoted string which identifies a WebIDL file to
           process. There may be many of these directives as required
           but without at least one the binding is not very useful as
           it will generate no output.

      preface

          This takes a cdata block. There may only be one of these per
           binding, subsequent directives will be ignored.

          The preface is emitted in every generated source file before
           any other output and generally is used for copyright
           comments and similar. It is immediately followed by the
           binding tools preamble comments.

      prologue

          This takes a cdata block. There may only be one of these per
           binding, subsequent directives will be ignored.

          The prologue is emitted in every generated source file after
           the class preface has been generated. It is often used for
           include directives required across all modules.

      epilogue

          This takes a cdata block. There may only be one of these per
           binding, subsequent directives will be ignored.

          The epilogue is emitted after the generated code and the
           class epilogue

      postface

          This takes a cdata block. There may only be one of these per
           binding, subsequent directives will be ignored.

          The postface is emitted as the very last element of the
           generated source files.


  class

      The class controls the generation of source for an IDL interface
       private member variables are declared here and header and
       footer elements specific to this class.

      Although not generally useful, because methods that reference a
       class cause it to be implicitly created, empty definitions are
       permitted.

      private

          variables added to the private structure for the class.

      preface

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The preface is emitted in every generated source file after
           the binding preface and tool preamble.

      prologue

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The prologue is emitted in every generated source file after
           the binding prologue has been generated.

      epilogue

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The epilogue is emitted after the generated code and before
           the binding epilogue

      postface

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The postface is emitted after the binding epilogue.


  methods

      The methods allow a binding to provide code to be inserted in
       the output and to control the class initializer and finalizer
       (note not the constructor/destructor)

      All these are in the syntax of:

          methodtype declarator ( parameters )

      They may optionally be followed by a cdata block which will be
       added to the appropriate method in the output. A semicolon may
       be used instead of the cdata block but this is not obviously
       useful except in the case of the init type.

      methods and getters/setters for properties must specify both
       class and name using the c++ style double colon separated
       identifiers i.e. class::identifier

      Note: the class names must match the IDL interface names in the
       binding but they will almost certainly have to be translated
       into more suitable class names for generated output.


      preface

          The declarator for this method type need only identify the
           class (an identifier may be provided but will be ignored).

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The preface is emitted in every generated source file after
           the binding preface and tool preamble.


      prologue

          The declarator for this method type need only identify the
           class (an identifier may be provided but will be ignored).

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The prologue is emitted in every generated source file after
           the binding prologue has been generated.


      epilogue

          The declarator for this method type need only identify the
           class (an identifier may be provided but will be ignored).

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The epilogue is emitted after the generated code and before
           the binding epilogue


      postface

          The declarator for this method type need only identify the
           class (an identifier may be provided but will be ignored).

          This takes a cdata block. There may only be one of these per
           class, subsequent directives will be ignored.

          The postface is emitted after the binding epilogue.


      init

          The declarator for this method type need only identify the
           class (an identifier may be provided but will be ignored).

          TODO: should it become necessary to defeat the automated
           generation of an initializer altogether the identifier can
           be checked and if set to the class name (like a
           constructor) output body simply becomes a verbatim copy of
           the cdata block.

          The parameter list may be empty or contain type/identifier
           tuples. If there is a parent interface it will be called
           with the parameters necessary for its initializer, hence the
           entire ancestry will be initialised.

          The parameters passed to the parent are identified by
           matching the identifier with the parents initializer
           parameter identifier, if the type does not match a type
           cast is inserted.

          It is sometimes desirable for the parent initializer
           identifier to be different from the childs identifier. In
           this case the identifier may have an alias added by having
           a double colon followed by a second identifier.

          For example consider the case below where HTMLElement
           inherits from Element which inherits from Node.

            init Node("struct dom_node *" node);
            init Element("struct dom_element *" element::node);
            init HTMLElement("struct dom_html_element *" html_element::element);

          The three initializers have parameters with different
           identifiers but specify the identifier as it appears in
           their parents parameter list. This allows for differing
           parameter ordering and identifier naming while allowing the
           automated enforcement of correct initializer calling
           chains.


      fini

          The declarator for this method type need only identify the
           class (an identifier may be provided but will be ignored).

         The cdata block is output.

	 The parent finalizer is called (finalizers have no parameters
	  so do not need the complexity of initializers.

      method

          The declarator for this method type must contain both the
           class and the identifier.

          The cdata block is output.

      getter

          The declarator for this method type must contain both the
           class and the identifier.

          The cdata block is output.

      setter

          The declarator for this method type must contain both the
           class and the identifier.

          The cdata block is output.


Generated source
----------------

duk_libdom

  This binding type generates several files as output:
    - binding.c
          support function source

    - binding.h
          header to declare the support functions and magic constant

    - prototype.h
          header which declares all the prototype builder, initialiser
            and finalizer functions.

    - private.h
          Which defines all the private structures for all classes

    - C source file per class
          These are the main output of the tool and are structured to
            give the binding author control of the output while
            automatically generating as much code as possible.

          The files are generated in sections:
              tool preface
              binding preface
              class preface
              tool prologue
              binding prologue
              class prologue
              initialiser
              finalizer
              constructor
              destructor
              methods
              getters and setters
              prototype
              class epilogue
              binding epilogue
	      tool epilogue
	      class postface
	      binding postface
	      tool postface

          The preface, prologue, epilogue and postface are all
            verbaitum output from the binding and class elements in the
            binding. All the other sections are generated code augmented
            and controlled by the binding.

	  Method generation produces a method with appropriate
	    signature added to the prototype. Any verbatum cdata from
	    the binding is added after the code that automatically
	    populates a private pointer of the appropriate type
	    (named priv).

References
----------

[1] http://www.w3.org/TR/WebIDL/
[2] https://heycam.github.io/webidl/
[3] https://dom.spec.whatwg.org/
[4] https://html.spec.whatwg.org/
