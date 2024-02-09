/*
 * Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>
 *
 * This file is part of libnslog.
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * NetSurf Logging
 */

#ifndef NSLOG_INTERNAL_H_
#define NSLOG_INTERNAL_H_

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

#include "nslog/nslog.h"

bool nslog__filter_matches(nslog_entry_context_t *ctx);

#endif /* NSLOG_INTERNAL_H_ */
