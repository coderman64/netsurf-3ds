Quick Build Steps for NetSurf
=============================

Last Updated: 21st January 2020

This document provides steps for building NetSurf.

These instructions use a shell script to perform several operations.
  This script has only been tested with the bash and zsh bourne style
  shell interpreters. The latest version of this script should be
  retrieved from the official NetSurf source repository.

This shell script is used by the NetSurf Developers but you should
  satisfy yourself that the script is not malicious. It should be noted
  that building the browser will also be executing shell code and
  requires a similar level of trust.


Native build
============

Grab a temporary env.sh
-----------------------

     $ wget https://git.netsurf-browser.org/netsurf.git/plain/docs/env.sh
     $ unset HOST
     $ source env.sh


Install any packages you need
-----------------------------

Installs all packages required to build NetSurf and the NetSurf project
libraries.

     $ ns-package-install

If your package manager is not supported, you will have to install third
  party packages manually.


Get the NetSurf project source code from Git
--------------------------------------------

All the sources for the browser and support libraries is available
  from the public git server.

Local copies may be easily obtained with the ns-clone command.

     $ ns-clone


Build and install our project libraries
---------------------------------------

Updates NetSurf project library sources to latest, builds and installs them.

      $ ns-pull-install


Switch to new NetSurf workspace
-------------------------------

Remove the bootstrap script and use the newly installed one

      $ rm env.sh
      $ cd ~/dev-netsurf/workspace
      $ source env.sh


Build and run NetSurf
---------------------

      $ cd netsurf

To build the native front end (the GTK front end on Linux, BSDs, etc)
  you could do:

      $ make
      $ ./nsgtk3

To build the framebuffer front end, you could do:

      $ make TARGET=framebuffer
      $ ./nsfb

More detailed documentation on using the [framebuffer](docs/using-framebuffer.md)
  frontend are available.

Cross Compiling
===============

If you are cross compiling, you can follow the above steps, but when
  sourcing env.sh, you should set HOST environment variable to the
  appropriate triplet for your cross compiler. For example, to cross
  compile for RISC OS:

      $ HOST=arm-unknown-riscos source env.sh

After that, the commands such as `ns-package-install` and
  `ns-pull-install` will do what is appropriate for the platform you are
  building for.

To do the final build of NetSurf, pass the appropriate TARGET to
  make. For example, to cross compile for RISC OS:

      $ make TARGET=riscos

Finally, you can package up your build to transfer to the system you
  are developing for.  For example, to produce a package for RISC OS:

      $ make TARGET=riscos package

Getting a cross compiler set up
-------------------------------

We maintain cross compilation environments and an SDK for a number of
  platforms.  These may be found in our toolchains repository.

      $ git clone git://git.netsurf-browser.org/toolchains

Pre-built versions of the toolchains for 64bit x86 Debian systems are
  available via our [automated build and test
  infrastructure](https://ci.netsurf-browser.org/builds/toolchains/)


Not working?
============

If the above steps are inapplicable, or don't work, you can build
  manually. Follow the instructions in the BUILDING-* documents in the
  docs/ directory the NetSurf browser source tree.

