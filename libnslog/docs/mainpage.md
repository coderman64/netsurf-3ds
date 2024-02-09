Lib NSLOG
=========

The purpose of the NSLog library is to provide a flexible logging macro which
permits logging with categories, and at levels.  Then the library can queue
log entries and permit filtering for log readers.

Since logging may have to happen during static initialisation, or before the
logging library is fully initialised, categories are progressively added to
the logging system as they are encountered.  Since, at their core, categories
are essentially just strings, we can write filters which will be resolved to
categories later as and when they are encountered.

Libraries which use NSLog should *NOT* act as clients of the library excepting
in tools, and test drivers.  By implementing a client in the test drivers,
libraries can verify the log messages sent.  If a library might produce a lot
of logging then it must implement a client in its test drivers or it might run
out of RAM while running tests.

If you're not into prose, [here is a commented example](md_docs_examples.html).

How to log stuff
----------------

In order to log messages, you need to have a category to log under.  We
recommend that you have one category per library or application, and that you
create sub-categories beneath that to divide the library (or app) into logical
units.

Categories should be declared in headers, and defined in C files.  You declare
a category thusly:

    NSLOG_DECLARE_CATEGORY(catname);

In that `catname` is a valid suffix for a C identifier, and will be used as
the logical name of the category for the purpose of NSLOG statements.

You define a category thusly:

    NSLOG_DEFINE_CATEGORY(catname, description);

In that `catname` matches the above, and `description` is a C string (or
identifier to a C string) which describes the category.

You define a subcategory thusly:

    NSLOG_DEFINE_SUBCATEGORY(parentcatname, catname, description);

In that `parentcategory` is the `catname` of another category which will be
considered the parent of this subcategory.  `catname` and `description` as
above.  Subcategories are rendered with slashes between their names when shown
as text, so a category of `foo` with a subcategory of `bar` would result in
two categories, one `foo` and one `foo/bar`.

Once you have categories, you can log stuff.  Your categories don't have to be
entirely public, but you will need access to the symbol to be able to log
things with the given category.  To log something, you use:

    NSLOG(catname, level, logmsg, args...);

In that `catname` is a `catname` from the above, `level` is a bareword logging
level which will be prefixed with `NSLOG_LEVEL_` for use (e.g. you can use
`WARNING` directly).  `logmsg` and `args...` are a `printf()` style log message
which will be rendered by the time `NSLOG()` returns, so you don't have to
worry about lifetimes.

Lib NSLOG lets you define a minimum compiled-in logging level which can be used
to elide deep debugging in release builds.  The `NSLOG()` macro expands to a
constant comparison which will be resolved at compile time (assuming
optimisations are turned on) to result in logging which doesn't need to be
compiled in, being compiled out (and thus is is basically zero-cost to sprinkle
deep debugging in your code).

Being a libnslog client
-----------------------

In order for code which uses `NSLOG()` to be hosted effectively, the client must
do two things.  Firstly a callback must be provided.  The simplest callback
would be:

    static nslog__client_callback(void *context, nslog_entry_context_t *ctx,
                                  const char *fmt, va_list args)
    {
        (void)context;
        (void)ctx;
        (void)fmt;
        va_start(args, fmt);
        va_end(args);
    }

In that `context` is a `void*` which is passed through to the client code
directly, allowing a client to provide some struct or other through to the
logging client callback.  `ctx` is an NSLOG context which you can find out more
about by reading the docs.  It contains the information about the logging
*site* such as the filename, line number, function name, level, and category
used to make the log entry.  `fmt` is the `logmsg` from above, and `args`
is the variadic argument set used to format the arguments.

This callback is registered into Lib NSLOG with something like:

    nslog_set_render_callback(nslog__client_callback, NULL);

If a callback is not set, then bad things will happen when the next thing is
run.  The final thing which must happen before the client will receive log
statements is that the client must _uncork_ the logging library.  Since logging
can occur **before** the client is ready, the library will render and store the
log messages in a list, awaiting the call to uncork.  Once uncorked, log
messages will be passed through as flatly as possible to the client.  To uncork
the library, call:

    nslog_uncork();

All corked messages will be passed to the client callback, in the order they
were logged, before the uncork call returns.

Filtering logs
--------------

The primary way to filter logs is to simply compile out the logging levels you
do not wish to exist.  Sadly this is a very coarse filter and is best used to
exclude deep debug (and possibly debug) from release builds.  If you wish to
filter your logs more flexibly at runtime, then you need to set up a log
filter.  You *can* build the filters "by hand" but much easier is to use the
text filter parser as such:

    nslog_filter_t *filter = NULL;
    if (nslog_filter_from_text(my_filter_text, &output) == NSLOG_NO_ERROR) {
        nslog_filter_set_active(filter, NULL);
        nslog_filter_unref(filter);
    }

The filter syntax is fairly simple:

* A `filter` is one of a `NOT` an `AND`, `OR`, or `XOR`, or a `simple` filter.
* a `NOT` filter is of the form `!filter`
* an `AND` filter is of the form `(filter && filter)`
* an `OR` filter is of the form `(filter || filter)`
* an `XOR` filter is of the form `(filter ^ filter)`
* A `simple` filter is one of a `level`, `category`, `filename`, `dirname`,
  or `funcname` filter.
* a `level` filter is of the form `level: FOOLEVEL` where `FOOLEVEL` is one of
  `DEEPDEBUG`, `DEBUG`, `VERBOSE`....
* a `category` filter is of the form `cat: catname` where `catname` is a
  category name.  If it is of the form `foo` then it will match all categories
  `foo` or `foo/*` but not `foobar`
* a `filename` filter is of the form `file: filename` and is essentially a
  suffix match on the filename.  `foo.c` will match `foo.c` and `bar/foo.c`
  but not `barfoo.c`
* a `dirname` filter is of the form `dir: dirname` and is essentially a prefix
  match on the filename.  `foo` will match `foo/bar.c` and `foo/bar/baz.c` but
  not `foobar.c`

Between tokens, any amount of whitespace is valid, but canonically only a small
amount will be used.  You can take any filter you have and re-render it in its
canonical text form by using `nslog_filter_sprintf()` which is documented.

Filters are evaluated in standard order, with parentheses doing the obvious
thing.  In addition, the `AND` and `OR` filters will short-circuit as you'd
expect, so you can use that to your advantage to make your filters efficient.
Internally, Lib NSLOG will cache filtering intermediates to make things as fast
as possible, but a badly designed filter will end up slowing things down
dramatically.
