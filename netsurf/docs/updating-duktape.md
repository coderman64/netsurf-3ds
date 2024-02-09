Updating Duktape
================

1.  Fetch the [latest release](http://duktape.org/download.html) archive.

2.  Extract it somewhere.

3.  That extracts to a `duktape-[VERSION]` directory.

4.  We need to tell duktape about our `duk_custom.h` header:

    1.  Change into the `duktape-[VERSION]` directory.

    2.  Run the following command:

            python2 tools/configure.py \
              --output-directory /tmp/output \
              --source-directory src-input \
              --config-metadata config \
              --fixup-line '#include "duk_custom.h"'

    3.  This generates a suitable set of duktape
        sources in `/tmp/output`

5.  Replace the `duktape.c`, `duktape.h` and
    `duk_config.h` files in the netsurf source
    tree (in `content/handlers/javascript/duktape`)
    with those generated in `/tmp/output`.

