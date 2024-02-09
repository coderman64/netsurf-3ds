User Interface
==============

[TOC]

Netsurf is divided into a series of frontends which provide a user
interface around common core functionality. Each frontend is a
distinct implementation for a specific GUI toolkit.

Because of this the user interface has different features in
each frontend allowing the browser to be a native application.

# Frontends

As GUI toolkits are often applicable to a single Operating
System (OS) some frontends are named for their OS instead of the
toolkit e.g. RISC OS WIMP frontend is named riscos and the Windows
win32 frontend is named windows.

## amiga

Frontend specific to the amiga

## atari

Frontend specific to the atari

## beos

Frontend specific to the Haiku OS

## framebuffer

There is a basic user guide for the [framebuffer](docs/using-framebuffer.md)

## gtk

Frontend that uses the GTK+2 or GTK+3 toolkit

## monkey

This is the internal unit test frontend.

There is a basic user guide [monkey](docs/using-monkey.md)

## riscos

Frontend for the RISC OS WIMP toolkit.

## windows

Frontend which uses the Microsodt win32 GDI toolkit.

# User configuration

The behaviour of the browser can be changed from the defaults with a
configuration file. The [core user options](docs/netsurf-options.md)
of the browser are common to all versions and are augmented by each
frontend in a specific manner.


