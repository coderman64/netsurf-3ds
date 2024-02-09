--------------------------------------------------------------------------------
  Build Instructions for BeOS and Haiku NetSurf               13 February 2010
--------------------------------------------------------------------------------

  This document provides instructions for building the BeOS and Haiku version 
  of NetSurf and provides guidance on obtaining NetSurf's build dependencies.

  BeOS NetSurf has been tested on Zeta and Haiku only for now. There are still some
  issues to sort out for other BeOS versions.

  Quick Start
=============

  See the QUICK-START document, which provides a simple environment with
  which you can fetch, build and install NetSurf and its dependencies.

  The QUICK-START is the recommended way to build NetSurf for Haiku. BeOS needs too much manual
  hacking to be built this way.


  Manual building
================================

  To build NetSurf on a BeOS, provided you have the relevant
  build dependencies installed, simply run:

      $ make

  If that produces errors, you probably don't have some of NetSurf's build
  dependencies installed. See "Obtaining NetSurf's dependencies" below. You
  may need to "make clean" before attempting to build after installing the 
  dependencies. Also note BeOS has an old make command that won't work, see 
  below.


  Obtaining NetSurf's dependencies
==================================

  Many of NetSurf's dependencies are either installed or available for BeOS and 
  Haiku. The remainder must be installed manually.

  The NetSurf project's libraries
---------------------------------

  The NetSurf project has developed several libraries which are required by
  the browser. These are:

  BuildSystem     --  Shared build system, needed to build the other libraries
  LibParserUtils  --  Parser building utility functions
  LibWapcaplet    --  String internment
  Hubbub          --  HTML5 compliant HTML parser
  LibCSS          --  CSS parser and selection engine
  LibNSGIF        --  GIF format image decoder
  LibNSBMP        --  BMP and ICO format image decoder
  LibROSprite     --  RISC OS Sprite format image decoder

  To fetch each of these libraries, run the appropriate commands from the
  Docs/LIBRARIES file, from within your workspace directory.

  To build and install these libraries, simply enter each of their directories
  and run:
  
      $ make install

  | Note: We advise enabling iconv() support in libparserutils, which vastly
  |       increases the number of supported character sets.  To do this,
  |       create a file called Makefile.config.override in the libparserutils
  |       directory, containing the following line:
  |
  |           CFLAGS += -DWITH_ICONV_FILTER
  |
  |       For more information, consult the libparserutils README file.

  TODO: add some more here.

  Additional requirements for BeOS
==================================

  On Haiku, other libraries and tools are either shipped with the system or available through the
  package repositories. For BeOS based systems, you will need to install and update all the
  required tools, as described below.

  rc
----

  Building NetSurf needs the Haiku resource compiler (rc), that allows 
  importing files from resource definitions (.rdef).

      $ cd <haiku-trunk-directory>
      $ TARGET_PLATFORM=r5 jam -q rc
      $ cp generated/objects/dano/x86/release/tools/rc/rc  /boot/home/config/bin/


  GNU make 3.81
---------------

  BeOS has an old make tool, which won't work when building NetSurf.
  Haiku has 3.81 which is the one that works. For BeOS, one has to replace 
  the original make with one built from the Haiku tree, or install it as gmake:

      $ cd <haiku-trunk-directory>
      $ TARGET_PLATFORM=r5 jam -q make
      $ cp generated/objects/r5/x86/release/bin/make/make /boot/home/config/bin/gmake


  cURL
------

  NetSurf uses cURL to fetch files from the network. 
  There is a patch against the official version on HaikuPorts.

  TODO


  libpng
--------

  NetSurf uses libPNG to display PNG files.
  It should build just fine on BeOS.


  libjpeg
---------

  NetSurf uses libjpeg to display JPEG files.
  It should already be available in your dev kit.


  OpenSSL
----------

  NetSurf uses OpenSSL for encrypted transfers.


  General requirements
----------------------

  There is currently an issue on stdbool.h (unsigned char bool vs enum bool) 
  which needs to be fixed, for now one can use the Haiku version of the header 
  and copy it over the gcc-provided one.
      $ cd <haiku-trunk-directory>
      $ cp headers/build/gcc-2.95.3/stdbool.h /boot/develop/tools/gnupro/lib/gcc-lib/i586-pc-beos/2.95.3-beos-060710/include/stdbool.h


  NetSurf might build on BeOS R5 but probably won't work on anything else than 
  BONE. 
