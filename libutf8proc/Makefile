#!/bin/make
#
# Makefile for libutf8proc
#
# Copyright 2009-2015 John-Mark Bell <jmb@netsurf-browser.org>

# Component settings
COMPONENT := utf8proc
COMPONENT_VERSION := 2.4.0-1
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

CFLAGS := -I$(CURDIR)/include/libutf8proc/ -I$(CURDIR)/src \
	-DUTF8PROC_EXPORTS \
  -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
	$(WARNFLAGS) $(CFLAGS)
ifneq ($(GCCVER),2)
  CFLAGS := $(CFLAGS) -std=c99
else
  # __inline__ is a GCCism
  CFLAGS := $(CFLAGS) -Dinline="__inline__"
endif

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
I := /$(INCLUDEDIR)/libutf8proc
INSTALL_ITEMS := $(INSTALL_ITEMS) $(I):include/libutf8proc/utf8proc.h
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR)/pkgconfig:lib$(COMPONENT).pc.in
INSTALL_ITEMS := $(INSTALL_ITEMS) /$(LIBDIR):$(OUTPUT)

