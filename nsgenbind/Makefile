#!/bin/make
#
# Makefile for nsgenbind
#
# Copyright 2013-2020 Vincent Sanders <vince@netsurf-browser.org>

# Define the component name
COMPONENT := nsgenbind
# And the component type
COMPONENT_TYPE := binary
# Component version
COMPONENT_VERSION := 0.9

# Tooling
PREFIX ?= /opt/netsurf
NSSHARED ?= $(PREFIX)/share/netsurf-buildsystem
include $(NSSHARED)/makefiles/Makefile.tools

TESTRUNNER := test/testrunner.sh

# Toolchain flags
WARNFLAGS := -Wall -W -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wnested-externs
# Non release variants should make compile warnings errors
ifneq ($(VARIANT),release)
  # BeOS/Haiku/AmigaOS have standard library errors that issue warnings.
  ifneq ($(BUILD),i586-pc-haiku)
    ifneq ($(findstring amigaos,$(BUILD)),amigaos)
      WARNFLAGS:= $(WARNFLAGS) -Werror
    endif
  endif
endif

CFLAGS := -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L \
	-I$(CURDIR)/include/ -I$(CURDIR)/src \
  -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
	$(WARNFLAGS) $(CFLAGS)
ifneq ($(GCCVER),2)
  CFLAGS := $(CFLAGS) -std=c99
else
  # __inline__ is a GCCism
  CFLAGS := $(CFLAGS) -Dinline="__inline__"
endif

# Grab the core makefile
include $(NSBUILD)/Makefile.top

# Add extra install rules for binary
INSTALL_ITEMS := $(INSTALL_ITEMS) /bin:$(OUTPUT)
