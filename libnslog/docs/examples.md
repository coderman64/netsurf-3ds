Example of libnslog in use
==========================

Since some people work better from an example, the following is an example of
both a library using libnslog, and a client application using that library.

The source is only inlined here, and not directly tested, but it should be
moderately complete and thus functional.  It was taken, in part, from the test
suite so that you can be reasonably confident it works.

The library
===========

First we need a public header:

```{.c}
    /* Example Library Header, include/libexample.h */

    #ifndef LIBEXAMPLE_H
    #define LIBEXAMPLE_H

    #include "nslog/nslog.h"

    /* All logging in libexample comes underneath this category */
    NSLOG_DECLARE_CATEGORY(libexample);

    /* This function does what the library does */
    extern void example_do_stuff(void);

    #endif
```

Next we need a private header, because libexample has some more categories...

```{.c}
    /* Example Library Header, src/hidden.h */

    #ifndef LIBEXAMPLE__HIDDEN_H
    #define LIBEXAMPLE__HIDDEN_H

    #include "libexample.h"

    /* When logging some stuff, we use this subcategory */
    NSLOG_DECLARE_CATEGORY(interesting);

    /* And when logging some other stuff, we use this subcategory */
    NSLOG_DECLARE_CATEGORY(boring);

    void libexample__hidden_func();

    #endif
```

Finally let's have some libexample code:

```{.c}
    /* Example Library source, src/libexample.c */

    #include "hidden.h"

    /* First up, lets realise the main category */
    NSLOG_DEFINE_CATEGORY(libexample, "The example library for nslog");

    void example_do_stuff(void)
    {
        /* Despite not having realised the categories here, we can use them */
        NSLOG(interesting, INFO, "Did you know?  Categories can be realised anywhere!");
        libexample__hidden_func();
        /* We can also log with the main category, despite it having subs */
        NSLOG(libexample, INFO, "All done, good bye %s", "Mr Bond");
    }
```

And because functionality may be spread among files:

```{.c}
    /* Example Library source, src/hidden.c */

    #include "hidden.h"

    /* Lets define the subcategories, even though the main is elsewhere */
    NSLOG_DEFINE_SUBCATEGORY(libexample, interesting, "Interesting things");
    NSLOG_DEFINE_SUBCATEGORY(libexample, boring, "Boring stuff");

    void libexample__hidden_func(void)
    {
        /* And here we can log with the main or sub categories at our leisure */
        NSLOG(libexample, INFO, "Yay, top level stuff");
        NSLOG(boring, DEBUG, "Boring debug number: %d", 18);
    }
```

The above, compiled together with libnslog's headers, will result in a library.

The client application
======================

Since it's easy enough to do, we'll show a client application in a single file.
It ought to be well enough commented to be of use...

```{.c}
    /* Example client application, main.c */

    /* We use nslog */
    #include "nslog/nslog.h"

    /* As the client, we only get to see the public API of our library */
    #include "libexample.h"

    /* And we're using printf and friends */
    #include <stdio.h>
    #include <stdarg.h>

    /* All client applications *MUST* have a render function.
     * Ours is deliberately obnoxious in order to make things clear.
     */
    static void
    exampleapp__render_function(void *_ctx, nslog_entry_context_t *ctx,
                                const char *fmt, va_list args)
    {
        UNUSED(_ctx);
        /* All the metadata about the log entry */
        fprintf(stderr,
                "EXAMPLE LOG MESSAGE:\n"
                "Category name: %.*s\n",
                "Category description: %s\n",
                "Logging level: %s\n",
                "Source location: %.*s (line %d function %.*s)\n",
                ctx->category->namelen, ctx->category->name, ctx->category->description,
                nslog_level_name(ctx->level),
                ctx->filenamelen, ctx->filename, ctx->lineno, ctx->funcnamelen, ctx->funcname);
        /* The log entry itself */
        vfprintf(stderr, fmt, args);
        /* Log entries aren't newline terminated, let's put a couple here for clarity */
        fprintf(stderr, "\n\n");
    }

    /* All that's left is to cause code to run... */
    int
    main(int argc, char **argv)
    {
        UNUSED(argc);
        UNUSED(argv);

        /* One beauty of libnslog is that it allows logging before the client
         * is nominally ready...
         */
        example_do_stuff();

        /* To make the client ready, we need to register our callback */
        if (nslog_set_render_callback(exampleapp__render_function, NULL) != NSLOG_NO_ERROR) {
            fprintf(stderr, "Unable to set render callback\n");
            return 1;
        }

        /* Next we can uncork the log.  This causes all previously logged
         * messages to make their way out of our render function
         */
        fprintf(stderr, "Before uncork...\n");
        if (nslog_uncork() != NSLOG_NO_ERROR) {
            fprintf(stderr, "Unable to uncork!\n");
            return 2;
        }
        fprintf(stderr, "After uncork.\n");

        /* And of course, we can log again... */
        example_do_stuff();

        /* We can set a filter, and since nslog knows all the categories, we
         * are permitted to set filters on categories we can't directly access
         * ourselves.
         */
        nslog_filter_t *filter;
        if (nslog_filter_from_text("cat:libexample/interesting", &filter) != NSLOG_NO_ERROR) {
            fprintf(stderr, "Giving up, unable to parse filter.\n");
            return 3;
        }

        /* We need to set that filter as the active filter to engage it */
        if (nslog_filter_set_active(filter, NULL) != NSLOG_NO_ERROR) {
            fprintf(stderr, "Unable to set active filter, stopping.\n");
            return 4;
        }

        /* We don't need to hold on to our filter handle any longer */
        filter = nslog_filter_unref(filter);

        /* This time, only "interesting" log messages come through */
        example_do_stuff();

        /* We can remove the filter... */
        if (nslog_filter_set_active(NULL, NULL) != NSLOG_NO_ERROR) {
            fprintf(stderr, "Unable to clear active filter, stopping.\n");
            return 4;
        }

        /* Finally, all log messages come through once more */
        example_do_stuff();

        /* and we're done */
        return 0;
    }
```
