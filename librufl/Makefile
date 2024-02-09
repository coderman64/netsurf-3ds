# Component settings
COMPONENT := rufl
COMPONENT_VERSION := 0.1.0
# Default to a static library
COMPONENT_TYPE ?= lib-static

# Setup the tooling
PREFIX ?= /opt/netsurf
NSSHARED ?= $(PREFIX)/share/netsurf-buildsystem
include $(NSSHARED)/makefiles/Makefile.tools

TESTRUNNER := $(PERL) $(NSTESTTOOLS)/testrunner.pl

# Toolchain flags
WARNFLAGS := -Wall -W -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wnested-externs
ifeq ($(findstring -riscos,$(HOST)),-riscos)
  WARNFLAGS := $(WARNFLAGS) -pedantic
endif
# BeOS/Haiku/AmigaOS4 standard library headers create warnings
ifneq ($(BUILD),i586-pc-haiku)
  ifneq ($(findstring amigaos,$(BUILD)),amigaos)
    WARNFLAGS := $(WARNFLAGS) -Werror
  endif
endif
CFLAGS := -I$(CURDIR)/include/ -I$(CURDIR)/src $(WARNFLAGS) \
  	-D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
    $(CFLAGS)
ifneq ($(GCCVER),2)
  CFLAGS := $(CFLAGS) -std=c99
else
  # __inline__ is a GCCism
  CFLAGS := $(CFLAGS) -Dinline="__inline__"
endif
CFLAGS := $(CFLAGS) -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=500

# OSLib
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
  ifeq ($(findstring -riscos,$(HOST)),-riscos)
    CFLAGS := $(CFLAGS) -I$(PREFIX)/include
    LDFLAGS := $(LDFLAGS) -lOSLib32
    TESTLDFLAGS := $(TESTLDFLAGS) -static
  else
    # Regardless of the host platform we're building for, we
    # still need the RISC OS build environment because we need the
    # OSLib headers.
    # XXX: is there a way to avoid this path being hard-coded?
    CFLAGS := $(CFLAGS) -I/opt/netsurf/arm-unknown-riscos/env/include
  endif
endif

include $(NSBUILD)/Makefile.top

# Extra installation rules
I := /include
INSTALL_ITEMS := $(INSTALL_ITEMS) $(I):include/rufl.h
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR)/pkgconfig:lib$(COMPONENT).pc.in
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR):$(OUTPUT)
