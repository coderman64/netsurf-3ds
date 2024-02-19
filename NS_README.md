NetSurf source -- Web Browser and all associated support libraries
==================================================================

Overview
--------

NetSurf is a compact graphical web browser which aims for HTML5, CSS
and JavaScript support.

This complete source package contains all the required NetSurf
support libraries to build the browser.

Binaries
--------

The NetSurf project provide binary builds for releases for several
platforms form the website and have packaged the browser in several
distributions. This option may be more suitable for some users than
compiling from source.

Compilation
-----------

A simple "make" is sufficient to compile the browser with the
default gtk toolkit selection.

It depends upon the host system having a suitable toolchain for
compiling c programs installed along with GNU make, zlib, flex,
bison and lipjpeg

In addition to the core library dependencies it is strongly
recommended libcurl, openssl and libpng libraries are present or the
browser will not have HTTP fetches, HTTPS fetches and PNG graphics
support respectively.

There are multiple GUI toolkits available which all have additional
dependencies i.e. gtk browser frontend requires the gtk development
libraries.
