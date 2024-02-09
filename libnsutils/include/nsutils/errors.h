/*
 * Copyright 2014 John-Mark Bell <jmb@netsurf-browser.org>
 *
 * This file is part of libnsutils.
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * Error codes.
 */

#ifndef NSUTILS_ERRORS_H_
#define NSUTILS_ERRORS_H_

/**
 * Enumeration of error codes
 */
typedef enum {
	NSUERROR_OK,			/**< No error */

	NSUERROR_UNKNOWN,		/**< Unknown error - DO *NOT* USE */

	NSUERROR_NOMEM,			/**< Memory exhaustion */

	NSUERROR_NO_FETCH_HANDLER,	/**< No fetch handler for URL scheme */

	NSUERROR_NOT_FOUND,		/**< Requested item not found */

	NSUERROR_NOT_DIRECTORY,           /**< Missing directory */

	NSUERROR_SAVE_FAILED,		/**< Failed to save data */

	NSUERROR_CLONE_FAILED,		/**< Failed to clone handle */

	NSUERROR_INIT_FAILED,		/**< Initialisation failed */

	NSUERROR_MNG_ERROR,		/**< An MNG error occurred */

	NSUERROR_BAD_ENCODING,		/**< The character set is unknown */

	NSUERROR_NEED_DATA,		/**< More data needed */

	NSUERROR_ENCODING_CHANGE,	/**< The character changed */

	NSUERROR_BAD_PARAMETER,		/**< Bad Parameter */

	NSUERROR_INVALID,		/**< Invalid data */

	NSUERROR_BOX_CONVERT,		/**< Box conversion failed */

	NSUERROR_STOPPED,		/**< Content conversion stopped */

	NSUERROR_DOM,	                /**< DOM call returned error */

	NSUERROR_CSS,	                /**< CSS call returned error */

	NSUERROR_CSS_BASE,               /**< CSS base sheet failed */

	NSUERROR_BAD_URL,		/**< Bad URL */

	NSUERROR_BAD_CONTENT,		/**< Bad Content */

	NSUERROR_FRAME_DEPTH,            /**< Exceeded frame depth */

	NSUERROR_PERMISSION,             /**< Permission error */

	NSUERROR_NOSPACE,		/**< Insufficient space */

	NSUERROR_BAD_SIZE,               /**< Bad size */

	NSUERROR_NOT_IMPLEMENTED,        /**< Functionality is not implemented */
} nsuerror;

#endif

