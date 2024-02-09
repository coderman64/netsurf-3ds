#!/bin/make
#
# Makefile for libnspsl
#
# Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>

# Component settings
COMPONENT := nspsl
COMPONENT_VERSION := 0.1.7
# Default to a static library
COMPONENT_TYPE ?= lib-static

# Setup the tooling
PREFIX ?= /opt/netsurf
NSSHARED ?= $(PREFIX)/share/netsurf-buildsystem
include $(NSSHARED)/makefiles/Makefile.tools

# Reevaluate when used, as BUILDDIR won't be defined yet
TESTRUNNER = test/runtest.sh $(BUILDDIR) $(EXEEXT)

# Toolchain flags
WARNFLAGS := -Wall -W -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wnested-externs

CFLAGS := -D_GNU_SOURCE -D_DEFAULT_SOURCE \
	-I$(CURDIR)/include/ -I$(CURDIR)/src \
	-D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
	$(WARNFLAGS) $(CFLAGS)
ifneq ($(GCCVER),2)
  CFLAGS := $(CFLAGS) -std=c99
else
  # __inline__ is a GCCism
  CFLAGS := $(CFLAGS) -Dinline="__inline__"
endif
CFLAGS := $(CFLAGS) -D_POSIX_C_SOURCE=200809L

REQUIRED_LIBS := nspsl

TESTCFLAGS := -g -O2
TESTLDFLAGS := -l$(COMPONENT) $(TESTLDFLAGS)

include $(NSBUILD)/Makefile.top

# Extra installation rules
I := /$(INCLUDEDIR)
INSTALL_ITEMS := $(INSTALL_ITEMS) $(I):include/nspsl.h
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR)/pkgconfig:lib$(COMPONENT).pc.in
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR):$(OUTPUT)

# obtain public suffix list database from upstream
public_suffix_list.dat:
	wget -O $@ https://publicsuffix.org/list/public_suffix_list.dat
