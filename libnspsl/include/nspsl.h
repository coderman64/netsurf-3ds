/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnspsl
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * get the public suffix of a hostname
 *
 * \param hostname The name to check
 * \return pointer to the public suffix of the hostname if it was valid else
 * NULL.
 */
const char *nspsl_getpublicsuffix(const char *hostname);
