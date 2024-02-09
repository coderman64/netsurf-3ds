Implementing a new frontend
===========================

[TOC]

# Introduction

NetSurf is divided into a series of frontends which provide a user
interface around common core functionality.

Each frontend is a distinct implementation for a specific GUI toolkit.

The existing frontends are covered in the [user
interface](docs/user-interface.md) documentation.

Note implementing a new frontend implies using a toolkit distinct from
one of those already implemented and is distinct from porting NetSurf
to a new operating system platform.

It is recommend, in the strongest terms, that if the prospective
developer is porting to both a new platform and toolkit that they
*start* by getting the [monkey](docs/using-monkey.md) frontend
building and passing at least the basic integration tests on their
platform.

Experience has shown that attempting to port to a platform and
implement a toolkit at the same time generally results in failure to
achieve either goal.

NetSurf is built using GNU make and frontends are expected to
integrate with this buildsystem.

Implementation languages have historically been limited to C, C++ and
objective C. However any language that can call C functions and
*importantly* be called back from C code ought to be usable. For
example there have been experiments with JAVA using JNI but no current
frontend is implemented using it.

# Implementation complexity

An absolutely minimal "proof of concept" frontend implementation (like
the FLTK frontend that will be used as an example) is around 1,000
lines of C code. Basic functionality like the windows frontend is
around 7,000 lines. A complete fully functional frontend such as the
one for GTK is closer to 15,000 lines.

It should be noted the majority of the minimal implementation can
simply be copied and the names changed as appropriate from an existing
example. The actual amount of new code that needs to be provided is
very small.

NetSurf provides a great deal of generic functionality for things like
cookie, bookmark, history windows which require only minimal frontend
support with the [core window API](docs/core-window-interface.md).

A frontend developer is free to implement any and all of this generic
functionality thelselves in a manner more integrated into a toolkit.

# Implementation

A frontend is generally named for the toolkit it is implementing (i.e
gtk for the GTK+ toolkit). It is advisable to be as specific as
possible e.g. the frontend for the windows operating system should
have been named win32 allowing for an impementation using a differnt
toolkit (e.g mfc)

All the files needed for the frontend are contained in a single
sub-directory in the NetSurf source tree e.g. `frontends/fltk`

The only file outside this directory that much be changed is the
`frontends/Makefile.hts` where a new entry must be added to the valid
targets list.

## Build system

A frontend must provide three GNU Makefile fragments (these will be
included from the core Makefile):

 - `Makefile` - This is used to extend CFLAGS, CXXFLAGS and LDFLAGS variables as required. The executable target is set with EXETARGET and the browser source files are listed in the SOURCES variable
 - `Makefile.defaults` - allows setting frontend specific makefile variables and overriding of the default core build variables.
 - `Makefile.tools` - allows setting up frontend specific build tooling (as a minimum a tool for the package configuration in PKG_CONFIG)
 
Source code modules can be named as the devloper desires within the
frontend directory and should be added to the SOURCES variable as
desired.
 
## Program entry

Generally the entry point from the OS is the `main()` function and
several frontends have a `main.cpp` where some have used `gui.c`.
 
The usual shape for the `main()` function is a six step process:
 1. The frontends operation tables are registered with NetSurf
 2. The toolkit specific initialisation is performed (which may involve calling NetSurf provided utility functions for support operations like logging, message translations etc.)
 3. Initialise the NetSurf core. After this point all browser functionality is available and registered operations can be called.
 4. Perform toolkiit setup, usually opening the initial browsing window (perhaps according to user preferences)
 5. Run the toolkits main loop while ensuring the Netsurf scheduled operations are also run at teh apropriate time.
 6. Finalisation on completion.

## NetSurf operations tables

The frontend will generally call netsurf interfaces to get a desired
behaviour e.g. `browser_window_create()` to create a new browsing
context (the `browser_window_` prefix is historical and does not
necessarily create a window e.g. on gtk it is more likely to open a
tab in an existing window). To achive the desired operation some
operations need to be performed by the frontend under control of
NetSurf, these operations are listed in tables.

The operation tables should be registered with the NetSurf core as one
of the first operations of the frontend code. The functions in these
tables (and the tables themselves) must remain valid until
`netsurf_exit()` is called.

There are (currently) twelve sets of operation tables held in separate
structures. Only five of these are mandantory (misc, window, fetch,
bitmap and layout).

In this context mandantory means the tables must be non NULL and do
not have a suitable default. Each of the mandantory sets contain
function pointers to implement operations.

### misc operation table

The only mandantory operation in this table is schedule.

When schedule is called the frontend must arrange for the passed
callback to be called with the context parameter after a number of
miliseconds.

This callback is typicaly driven through the toolkits event loop and
it is important such callbacks are not attempted from an operation.

### window operation table

The window operations (poorly named as already mentioned) are where
the frontend is called to actually manipulate widgets in the
toolkit. This is mediated through a `gui_window` context pointer which
is typed as a structure.

This context pointer is passed to all window operations and is
generally assumed to contain at least a reference to the underlying
`browser_window` which is provided in the initial create operation to
allow subsequent use of various core functionality.

The mandantory window operations are:
 - create - create a new browsing context widget in the frontend toolkit
 - destroy - destoy a previously created `gui_window`
 - invalidate - mark an area of the browsing context viewport as requiring redraw (note no redraw should be attempted from here)
 - get_scroll - get the scroll offsets from the toolkit drawing widget
 - set_scroll - set the scroll offsets on the toolkit drawing widget
 - get_dimensions - get the dimensions of the toolkit drawing widget
 - event - deal with various window events from netsurf which have no additional parameters


### fetch operation table

The fetch operations allow the built in scheme fetchers (file, about, resource) to obtain additional information necessary to complete their operation.

The two mandantory operations are:
 - `filetype` - allows the file scheme to obtain a mime type from a file path e.g. `a.file.name.png` would result in `image/png`
 - `get_resource_url` - maps resource scheme paths to URL e.g. `resource:default.css` to `file:///usr/share/netsurf/default.css`

### bitmap operation table

The bitmap table and all of its operations are mandantory only because
the empty defaults have not been included as it is assumed a browser
will want to display images.

All operations may be provided by stubs that return the failure codes
until full implementations are made.

### layout operation table

The layout table is used to layout text. All operations are given
strings to manipulate encoded in UTF-8. There are three mandantory
operations:
 - `width` - Calculate the width of a string.
 - `position` - Find the position in a string where an x coordinate falls.
 - `split` - Find where to split a string to make it fit a width.

# Worked Example

Rather than attempt to describe every aspect of an implementation we
will rather work from an actual minimal example for the FLTK toolkit.

This is availble as a single commit (`git show 28ecbf82ed3024f51be4c87928fd91bacfc15cbc`) in the NetSurf source repository. Alternatively it can be [viewed in a web browser](https://git.netsurf-browser.org/netsurf.git/commit/?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc).

This represents the absolute minimum implementation to get a browser
window on screen (and be able to click visible links). It is
implemented in C++ as that is the FLTK implementation language but an
equivalent implementation in other languages should be obvious.

## Building

The [frontends/Makefile.hts](https://git.netsurf-browser.org/netsurf.git/diff/frontends/Makefile.hts?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc)
had the fltk target added to the VLDTARGET variable. This allows
NetSurf to be built for this frontend with `make TARGET=fltk`

As previously described the three GNU Make files are added:

[Makefile](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/Makefile?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc)
this shows how the flags are extended to add the fltk headers and
library. Additionaly the list of sources are built here, as teh
comment suggests it is important the SOURCES variable is not expanded
here so the S_FRONTEND variable is used to allow expansion at teh
correct time in the build process.

[Makefile.defaults](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/Makefile.defaults?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc) 
has the default setting to control the build parameters and file locations. These can be overriden by the `Makefile.config` at compile time.

[Makefile.tools](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/Makefile.tools?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc)
allows the configuration of additional tools necessary to build for the target as a minimum pkg-config is usually required to find libraries.
 
## Program entry

In our example program entry is the classical `main()` in the [main.cpp](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/main.cpp?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc) module.

This implements the six stage process outlined previously. 

### Operations table registration

The `netsurf_table` structure is initialised and passed to
`netsurf_register()`. It should be noted that the approach taken here
and in most frontends is to have a source module for each operation
table. The header for each module exposes just the pointer to the
indivial operation set, this allows for all the operation functions to
be static to their module and hence helps reduce global symbol usage.

### Frontend specific initialisation

Her it is implemented in `nsfltk_init()` this function performs all
the operations specific to the frontend which must be initialised
before netsurf itself. In some toolkits this would require calling the
toolkit initialisation (e.g. `gtk_init()`).

It is nessesary to initialise netsurf logging and user options at this
point. A more fully featured implementation would also initialise the
message translation system here.

### Netsurf initialisation

This is simply the call to `netsurf_init()` from this point the
browser is fully operational and operations can and will be called.

### Frontend specific startup

Although the browser is running it has not yet been told to open a
window or navigate to a page. Here `nsfltk_start()` examines the
command line and environment to determine the initial page to navigate
to and calls `browser_window_create()` with the url, this will cause
the browser to open a new browsing context and start the navigation.

A frontend may choose to implement more complex logic here but the
example here is a good start.

### Toolkit run loop

The function `nsfltk_run()` runs the toolkit event loop. In this case it is using the generic scheduleing in the [misc.cpp](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/misc.cpp?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc) module to ensure callbacks get made at the apropriate time.

There is a `nsfltk_done` boolean global checked here so when all the
browser windows are closed the program will exit.

A more fully featured port might use the toolkits scheduling rather
than open coding a solution with a linked list as is done
here.

A futher optimisation would be to obtain the set of file descriptors
being used (with `fetch_fdset()`) for active fetches allowing for
activity based fetch progress instead of the fallback polling method.

### finalisation

This simply finalises the browser stopping all activity and cleaning
up any resource usage. After the call to `netsurf_exit()` no more
operation calls will be made and all caches used by the core will be
flushed.

If user option chnages are to be made persistant `nsoption_finalise()`
should be called.

The finalisation of logging will ensure that any output buffers are
flushed.

## The window operation table

Amongst all the boilerplate of the default implementation the only novel code is in the window operation table in the [window.cpp](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/window.cpp?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc) module.

### `nsfltk_window_create`

The create operation instansiates a new `NS_Window` object and
references it in the gui_window structure which it returns to the
caller. Technically we could simply return the `NS_Window` object as
the gui_window pointer but this implementation is avoiding the cast.

Secondly `Fl_Double_Window` is subclassed as `NS_Widget`. The sublass
allows the close callback to be accessed so the global `nsfltk_done`
boolean can be set during the destructor method.

The NS_Window creates an instance of `NS_Widget` in its constructor, a
more extensive implementation would add other window furniture here
(scroll bars, url bar, navigation elements, etc.)

The implementation subclasses `Fl_Widget` implementing the draw
method to render the browsing context and the handle method to handle
mouse events to allow teh user to click.

The `NS_Widget::handle()` method simply translates the mouse press
event from widget coordinates to netsurf canvas cooridinates and maps
teh mouse button state. The core is informed of these events using
`browser_window_mouse_click()`

The `NS_Widget::draw` method similarly translates the fltk toolkits
clip rectangle, builds a plotting context and calls
`browser_window_redraw()` which will use the plotting operations in
the plotting context to render the browsing context within the area
specified. One thing to note here is the translation between the
coordinates of the render area and the internal page canvas given as
the second and third parameters to the draw call. When scrolling is
required this is achived by altering these offsets.


### `nsfltk_window_invalidate()`

This simply calls the damage method on the `Fl_Widget` class with the
appropriate coordinate translation.

### `nsfltk_window_get_dimensions()`

This obtains the fltk widget width and height and returns them.

## The plotting interface

When the `NS_Widget::draw` method was discussed it was noted that a
plotting context is built containing an operation table. That table is
implemented in [plotters.cpp](https://git.netsurf-browser.org/netsurf.git/diff/frontends/fltk/plotters.cpp?h=vince/fltk&id=28ecbf82ed3024f51be4c87928fd91bacfc15cbc)

The implementation here is as minimal as can be, only line, rectangle
and text have any implementation at all and even that simply sets a
colour and performs the appropriate fltk draw function (`fl_line`,
`fl_rect` and `fl_draw` respectively)

# Worked Example next steps

The previous section outlined the absolute minimum
implementation. Here we can exmaine some next steps taken to extend
the frontend.

## Improving the user interface

The example discussion is based on a commit (`git show bc546388ce428be5cfa37cecb174d549c7b30320`) in the NetSurf source repository. Alternatively it can be [viewed in a web browser](https://git.netsurf-browser.org/netsurf.git/commit/?h=vince/fltk&id=bc546388ce428be5cfa37cecb174d549c7b30320).

This changes a single module `window.cpp` where the `NS_Window`,
`NS_Widget` and `NS_URLBar` classes are used to create a basic
browsing interface.

The static window operation functions are moved inside the `NS_Window`
class and the `gui_window` structure is used to obtain an instance
allowing normal methods to be called to implement functionality. This
is purely to make the C++ code more idiomatic and obviously would be
handled differently in other languages.

The `NS_Window` constructor builds additional widgets to just the
browser drawing widget. It creates:
 - a URL bar widget containing some navigation buttons and a widget to show the current url
 - a vertical scrollbar
 - a horizontal scrollbar
 - a status text widget
 
The scrollbar widgets fltk callbacks (called when user interacts with
the scrollbar) call a method on the `NS_Widget` allowing it to track
the current scroll offsets which are subsequently used in the drawing
and user input handling methods.

## Improving rendering

Up to this point the rendering has been minimal and the text in a
single face and size with incorrect width measurement. There was no
proper handling of plotting styles and colours.

## Implementing bitmap rendering

There was no bitmap rendering so no pretty pictures.

## Implementing the user messages API

This immediately allows the browser to use the existing language
translations for many internal strings.

## Implementing a user settings dialog

Implementing a way for the user to change configuration options
without having to edit a configuration file greatly improves the
perceived functionality.

## Implementing corewindow

The [core window interface](docs/core-window-interface.md) allows a
frontend to use inbuilt rendering for several interfaces gaining a
great deal of functionality for very litte code. This one interface
set gives a cookie viewer,a local and global history viewer and a
hotlist(bookmarks) viewer.

# Conclusion

Hopefully this breif overview and worked example should give the
prospectinve frontend developer enough information to understand how
to get started implementing a new frontend toolkit for NetSurf.

As can be seen there is actualy very little novel code necessary to
get started though I should mention that the move from "minimal" to
"full" implementation is a large undertaking and it would be wise to
talk with the NetSurf developers if undertaking such work.
