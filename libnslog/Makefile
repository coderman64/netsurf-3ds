#!/bin/make
#
# Makefile for libnslog
#
# Copyright 2014-1015 Vincent Sanders <vince@netsurf-browser.org>
# Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>

# Component settings
COMPONENT := nslog
COMPONENT_VERSION := 0.1.3
# Default to a static library
COMPONENT_TYPE ?= lib-static

# Setup the tooling
PREFIX ?= /opt/netsurf
NSSHARED ?= $(PREFIX)/share/netsurf-buildsystem
include $(NSSHARED)/makefiles/Makefile.tools

# Reevaluate when used, as BUILDDIR won't be defined yet
TESTRUNNER = $(BUILDDIR)/test_testrunner$(EXEEXT)

# Toolchain flags
WARNFLAGS := -Wall -W -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wnested-externs

CFLAGS := -D_GNU_SOURCE -D_DEFAULT_SOURCE \
  -I$(DEVKITPRO)/libctru/include \
  -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft\
	-I$(CURDIR)/include/ -I$(CURDIR)/src $(WARNFLAGS) $(CFLAGS) -lctru
ifneq ($(GCCVER),2)
  CFLAGS := $(CFLAGS) -std=c99
else
  # __inline__ is a GCCism
  CFLAGS := $(CFLAGS) -Dinline="__inline__"
endif
CFLAGS := $(CFLAGS) -D_POSIX_C_SOURCE=200809L -g

REQUIRED_LIBS := nslog

# Strictly the requirement for rt is dependant on both the clib and if
# the build is using rt features like clock_gettime() but this check
# will suffice
ifeq ($(HOST),x86_64-linux-gnu)
  REQUIRED_LIBS := $(REQUIRED_LIBS) rt
endif

TESTCFLAGS := -g -O2
TESTLDFLAGS := -lm -l$(COMPONENT) $(TESTLDFLAGS)

include $(NSBUILD)/Makefile.top

ifeq ($(WANT_TEST),yes)
  ifneq ($(PKGCONFIG),)
    TESTCFLAGS := $(TESTCFLAGS) $(shell $(PKGCONFIG) --cflags check)
    TESTLDFLAGS := $(TESTLDFLAGS) $(shell $(PKGCONFIG) --libs check)
  else
    TESTLDFLAGS := $(TESTLDFLAGS) -lcheck
  endif
endif

# Extra installation rules
I := /$(INCLUDEDIR)/nslog
INSTALL_ITEMS := $(INSTALL_ITEMS) $(I):include/nslog/nslog.h
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR)/pkgconfig:lib$(COMPONENT).pc.in
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR):$(OUTPUT)
