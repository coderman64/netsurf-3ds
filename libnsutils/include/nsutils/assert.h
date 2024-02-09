/*
 * This file is part of LibNSUtils.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2022 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * Static assertion macro.
 */

#ifndef NSUTILS_ASSERT_H__
#define NSUTILS_ASSERT_H__

/** Compile time assertion macro. */
#define ns_static_assert(e) \
{ \
	enum { \
		ns_static_assert_check = 1 / (!!(e)) \
	}; \
}

#endif
