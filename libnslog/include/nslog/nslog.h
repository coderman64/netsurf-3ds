/*
 * Copyright 2017 Daniel Silverstone <dsilvers@netsurf-browser.org>
 *
 * This file is part of libnslog.
 *
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * NetSurf Logging
 */

#ifndef NSLOG_NSLOG_H_
#define NSLOG_NSLOG_H_

#include <stdarg.h>

/**
 * Log levels
 *
 * When logging, you can set the 'log level' which can be used as a basic way
 * to filter the logging which occurs.  In addition, the logging level can be
 * used to control which logging is compiled in.  As such, release builds can
 * be made where DEBUG and DEEPDEBUG are compiled out.
 *
 * If logging is set at INFO then everything above INFO will be logged,
 * including warnings, errors, etc.
 */
typedef enum {
	NSLOG_LEVEL_DEEPDEBUG = 0,
	NSLOG_LEVEL_DEBUG = 1,
	NSLOG_LEVEL_VERBOSE = 2,
	NSLOG_LEVEL_INFO = 3,
	NSLOG_LEVEL_WARNING = 4,
	NSLOG_LEVEL_ERROR = 5,
	NSLOG_LEVEL_CRITICAL = 6,
} nslog_level;

/**
 * Convert a logging level to a string.
 *
 * The returned string is owned by the nslog library (static) and should
 * not be freed.
 *
 * \param level The level for which you want the 'canonical' name.
 */
const char *nslog_level_name(nslog_level level);

/**
 * Convert a logging level to a short string.
 *
 * The returned string is owned by the nslog library (static) and should
 * not be freed.  It will be exactly four characters wide and suitable for
 * logging to a file neatly.
 *
 * \param level The level for which you want the 'short' name.
 */
const char *nslog_short_level_name(nslog_level level);

#define NSLOG_LEVEL_DD		NSLOG_LEVEL_DEEPDEBUG
#define NSLOG_LEVEL_DBG		NSLOG_LEVEL_DEBUG
#define NSLOG_LEVEL_CHAT	NSLOG_LEVEL_VERBOSE
#define NSLOG_LEVEL_WARN	NSLOG_LEVEL_WARNING
#define NSLOG_LEVEL_ERR		NSLOG_LEVEL_ERROR
#define NSLOG_LEVEL_CRIT	NSLOG_LEVEL_CRITICAL

#ifndef NSLOG_COMPILED_MIN_LEVEL
/**
 * The minimum log level to be compiled in.
 *
 * When compiling a library or application which uses nslog, you can set the
 * minimum level to be compiled in.  By setting this, you can reduce the
 * size of the binary and potentially improve the performance of hotspots
 * which contain logging instructions.
 */
#define NSLOG_COMPILED_MIN_LEVEL NSLOG_LEVEL_DEBUG
#endif

/**
 * Logging category
 *
 * This structure is used internally by nslog to manage categories for logging.
 * Categories are declared by the \ref NSLOG_DECLARE_CATEGORY and defined by
 * either \ref NSLOG_DEFINE_CATEGORY and \ref NSLOG_DEFINE_SUBCATEGORY.
 *
 * It is not recommended that clients of nslog look inside the category structs
 * excepting the name (and namelen) values.  In addition you should only trust
 * those values when handling a log callback.
 */
typedef struct nslog_category_s {
	const char *cat_name; /**< The category leaf name (static) */
	const char *description; /**< The category description (static) */
	struct nslog_category_s *parent; /**< The category's parent (static) */
	char *name; /**< The fully qualified category name (owned by nslog) */
	int namelen; /**< The length of the category name */
	struct nslog_category_s *next; /**< Link to next category (internal) */
} nslog_category_t;

/**
 * Log entry context
 *
 * When logging, nslog will create an entry context and pass it to the log
 * callback.  This context tells you everything about the log entry being
 * created (except the message) and is ephemeral (if you need the content
 * beyond the scope of the log callback, copy it).
 */
typedef struct nslog_entry_context_s {
	nslog_category_t *category; /**< The category under which the log entry is being made */
	nslog_level level; /**< The level at which the log entry is being made */
	const char *filename; /**< The source filename where the log entry is */
	int filenamelen; /**< The source filename name's length */
	const char *funcname; /**< The source function where the log entry is */
	int funcnamelen; /**< The source function name's length */
	int lineno; /**< The line number at which the log entry is */
} nslog_entry_context_t;

/**
 * Declare a category
 *
 * This macro is used to declare a category.  Use it in headers to allow more
 * than one source file to use the same category.
 *
 * \param catname The category leaf name (as a bareword)
 */
#define NSLOG_DECLARE_CATEGORY(catname)				\
	extern nslog_category_t __nslog_category_##catname

/**
 * Define a category
 *
 * This macro is used to define the storage for a category.  Use it in one of
 * your C source files to allow the category to have some storage space.
 *
 * \param catname The category name (as a bareword)
 * \param description The category description (as a static string)
 */
#define NSLOG_DEFINE_CATEGORY(catname, description)	\
	nslog_category_t __nslog_category_##catname = { \
		#catname,				\
		description,				\
		NULL,					\
		NULL,					\
		0,					\
		NULL,					\
	}

/**
 * Define a sub-category
 *
 * This macro is used to define the storage for a category which has a parent.
 * Use it in one of your C source files to allow the category to have some
 * storage space.  Sub-categories end up named by their parent name and their
 * name separated by slashes.  This is arbitrary in depth, allowing sub-sub-sub
 * categories etc.
 *
 * \param parentcatname The category name of the parent category (as a bareword)
 * \param catname The category name (as a bareword)
 * \param description The category description (as a static string)
 */
#define NSLOG_DEFINE_SUBCATEGORY(parentcatname, catname, description)	\
	nslog_category_t __nslog_category_##catname = {			\
		#catname,						\
		description,						\
		&__nslog_category_##parentcatname,			\
		NULL,							\
		0,							\
		NULL,							\
	}

/**
 * Log something
 *
 * This is the primary logging macro in nslog.  It needs access to the category
 * which is to be used to log, hence header files and the \ref
 * NSLOG_DECLARE_CATEGORY macro.
 *
 * \param catname The category name (as a bareword)
 * \param level The level at which this is logged (as a bareword such as WARNING)
 * \param logmsg The log message itself (printf format string)
 * \param args The arguments for the log message
 */
#define NSLOG(catname, level, logmsg, args...)				\
	do {								\
		if (NSLOG_LEVEL_##level >= NSLOG_COMPILED_MIN_LEVEL) {	\
			static nslog_entry_context_t _nslog_ctx = {	\
				&__nslog_category_##catname,		\
				NSLOG_LEVEL_##level,			\
				__FILE__,				\
				sizeof(__FILE__) - 1,			\
				__PRETTY_FUNCTION__,			\
				sizeof(__PRETTY_FUNCTION__) - 1,	\
				__LINE__,				\
			};						\
			nslog__log(&_nslog_ctx, logmsg, ##args);	\
		}							\
	} while(0)

/**
 * Internal logging function
 *
 * While clients of nslog will not call this function directly (preferring to
 * use the \ref NSLOG macro, this is the actual function which implements it.
 *
 * \param ctx The log entry context for the log
 * \param pattern The printf message pattern
 * \param ... The arguments for the log entry
 */
void nslog__log(nslog_entry_context_t *ctx,
		const char *pattern,
		...) __attribute__ ((format (printf, 2, 3)));

/**
 * Log error types
 *
 * When nslog is performing actions which can allocate memory or otherwise
 * rely on internal state machines.  Such functions always return an error
 * code and any extra values in pointer-based out-parameters.
 */
typedef enum {
	NSLOG_NO_ERROR = 0, /**< Nothing went wrong.  Have a party. */
	NSLOG_NO_MEMORY = 1, /**< nslog ran out of memory.  Worry, a great deal */
	NSLOG_UNCORKED = 2, /**< nslog is already uncorked, don't rush it */
	NSLOG_PARSE_ERROR = 3, /**< nslog failed to parse the given log filter */
} nslog_error;

/**
 * Callback type for logging
 *
 * In order for a logging client to actually receive the messages which are
 * logged, the client must register a logging callback.  It is recommended that
 * only the highest level client do so (for example NetSurf) but test suites
 * can also register for log messages if you want to use that as part of your
 * testing.
 *
 * \param context The context pointer registered for the callback
 * \param ctx The log entry context
 * \param fmt The log message (printf style format string)
 * \param args The printf arguments for the log entry
 */
typedef void (*nslog_callback)(void *context, nslog_entry_context_t *ctx,
			       const char *fmt, va_list args);

/**
 * Set the render callback for logging
 *
 * In order for the client to receive the log messages, it needs to register
 * a \ref nslog_callback function.
 *
 * \param cb The callback function pointer
 * \param context The context pointer to provide to the callback
 * \return Whether or not this succeeded
 */
nslog_error nslog_set_render_callback(nslog_callback cb, void *context);

/**
 * Uncork the log
 *
 * When nslog starts up, it is corked which means that any messages logged will
 * be saved up ready for when the client calls this function.  Corked messages
 * will be rendered when logged, so don't expect the format strings to be
 * identical between logging before and after uncorking.
 *
 * Any stored log messages will be drained before this function returns.
 *
 * \return Whether or not the uncorking succeeded.
 */
nslog_error nslog_uncork(void);

/**
 * Finalise log categories, release filter handles, etc.
 *
 * Since logging categories can have memory allocated to them at runtime,
 * and the logging filters can have references held inside the library,
 * clients which wish to be 'valgrind clean' may wish to call this to
 * ensure that any memory allocated inside the nslog library is released.
 *
 * This does not remove the active log callback, so logging calls after this
 * returns will still work (though will be unfiltered).  Of course, they will
 * cause memory to be allocated once more.  This function can be called as
 * many times as desired, it is idempotent.
 *
 * If the logging was corked when this was called, pending corked messages
 * will be delivered if necessary before any filters are removed.
 */
void nslog_cleanup(void);

/**
 * Log filter handle
 *
 * nslog allows clients to set a complex filter which can be used to restrict
 * the log messages which make their way through to the log callback.
 *
 * Clients can build log filters up "by hand" or can pass a string
 * representation of a log filter to the internal nslog parser.
 *
 * Log filters are reference counted and filter builders will return
 * a log filter with a reference, and will take automatically reference
 * any filters passed in, so remember to unref them if you're done.
 */
typedef struct nslog_filter_s nslog_filter_t;

/**
 * Create a category based filter
 *
 * The returned filter matches against the fully qualified category name
 * and succeeds if the given category name is either an exact match or
 * is a proper prefix of the category.
 *
 * For example, a filter of 'foo/bar' will match 'foo/bar' 'foo/bar/baz'
 * but not 'foo' or 'foo/barfle'.
 *
 * \param catname The category name to filter on
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_category_new(const char *catname,
				      nslog_filter_t **filter);
/**
 * Create a log level based filter
 *
 * The returned filter matches against the level of the log entry and succeeds
 * if the entry's level is at least the value of the filter.
 *
 * For example, a filter of \ref NSLOG_LEVEL_WARNING will match log entries
 * of the same level or higher (such as \ref NSLOG_LEVEL_ERROR)
 * but not lower levels (such as \ref NSLOG_LEVEL_DEBUG)
 *
 * \param level The log level to filter at
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_level_new(nslog_level level,
				   nslog_filter_t **filter);

/**
 * Create a filename based filter
 *
 * The returned filter matches against the filename and succeeds
 * if the entry's filename leafname matches the value of the filter.
 *
 * For example, a filter of "foo/bar.c" will match log entries
 * for "foo/bar.c" or "baz/foo/bar.c" but not "baz/bar.c".
 *
 * \param filename The filename to filter with
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_filename_new(const char *filename,
				      nslog_filter_t **filter);

/**
 * Create a dirname based filter
 *
 * The returned filter matches against the filename and succeeds
 * if the entry's filename dirname matches the value of the filter.
 *
 * For example, a filter of "foo" will match log entries
 * for "foo/bar.c" or "foo/bar/baz.c" but not "baz/foo.c".
 *
 * \param dirname The directory name to filter with
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_dirname_new(const char *dirname,
				     nslog_filter_t **filter);

/**
 * Create a function-name based filter
 *
 * The returned filter matches against the name of the function
 * where the \ref NSLOG statement exists, and succeeds if the
 * entry's function name exactly matches the value of the filter.
 *
 * For example, a filter of "foo" will match log entries
 * for `foo()` or `foo(int)` but not `foobar()`.
 *
 * \param funcname The function name to filter on
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_funcname_new(const char *funcname,
				      nslog_filter_t **filter);

/**
 * Create a logical 'and' of two filters.
 *
 * The returned filter succeeds if both the passed in filters
 * pass.  It will short-circuit and not evaluate the `right`
 * filter if the `left` filter fails.
 *
 * \param left The first filter to check
 * \param right The second filter to check
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_and_new(nslog_filter_t *left,
				 nslog_filter_t *right,
				 nslog_filter_t **filter);

/**
 * Create a logical 'or' of two filters.
 *
 * The returned filter succeeds if either of the passed in filters
 * pass.  It will short-circuit and not evaluate the `right`
 * filter if the `left` filter succeeds.
 *
 * \param left The first filter to check
 * \param right The second filter to check
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_or_new(nslog_filter_t *left,
				nslog_filter_t *right,
				nslog_filter_t **filter);

/**
 * Create a logical 'xor' of two filters.
 *
 * The returned filter succeeds if one, or the other,
 * but not both the passed in filters pass.
 *
 * \param left The first filter to check
 * \param right The second filter to check
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_xor_new(nslog_filter_t *left,
				 nslog_filter_t *right,
				 nslog_filter_t **filter);

/**
 * Create a logical 'not' of a filters.
 *
 * The returned filter succeeds if the passed in filter
 * fails.
 *
 * \param input The first filter to check
 * \param filter A pointer to a filter to be filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_not_new(nslog_filter_t *input,
				 nslog_filter_t **filter);

/**
 * Take a reference to the passed in filter.
 *
 * This increments the reference count of the given filter
 * and returns it directly to encourage a style of:
 *
 * `myfilter = nslog_filter_ref(incomingfilter)`
 *
 * \param filter A pointer to a filter to be referenced
 * \return Whether or not this succeeds
 */
nslog_filter_t *nslog_filter_ref(nslog_filter_t *filter);

/**
 * Release a reference to the passed in filter.
 *
 * This decrements the reference count of the given filter
 * and returns NULL to encourage the style of:
 *
 * `myfilter = nslog_filter_ref(myfilter)`
 *
 * \param filter A pointer to a filter to be dereferenced
 * \return Whether or not this succeeds
 */
nslog_filter_t *nslog_filter_unref(nslog_filter_t *filter);

/**
 * Set the passed in filter as the active filter
 *
 * This sets the active filter for managing the log.
 * If you don't pass `NULL` in prev, then the currently
 * active filter is returned for you to reuse later.
 *
 * \param filter A pointer to a filter to be set active
 * \param prev A pointer which will be optionally filled out
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_set_active(nslog_filter_t *filter,
				    nslog_filter_t **prev);


/**
 * Render a filter as its canonical textual form.
 *
 * Filters can be written in plain text and this function
 * turns filters into their canonical textual form.
 *
 * The caller owns the returned string and must `free()` it.
 *
 * \param filter A pointer to a filter to be rendered as text
 * \return A string representing the filter
 */
char *nslog_filter_sprintf(nslog_filter_t *filter);

/**
 * Parse a filter's textual form.
 *
 * Filters can be written in plain text and this function
 * turns textual filters into filters which can be used
 * by nslog.
 *
 * The caller owns the reference returned to it and must
 * unreference the filter when done with it.
 *
 * \param input A pointer to filter text to be parsed
 * \param output A pointer to fill out with the parsed filter
 * \return Whether or not this succeeds
 */
nslog_error nslog_filter_from_text(const char *input,
				   nslog_filter_t **output);

#endif /* NSLOG_NSLOG_H_ */
