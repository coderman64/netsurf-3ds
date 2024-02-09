Change Log
==========

NetSurf 3.11
------------

### Core / All platforms

* HTML: Fixed failure to reflow SVGs if fetched and ready before layout.
* HTML: Support for `display: flex` layout.
* HTML: Improved minimum/maximum sizes in box layout.
* HTML: Improved handling of percentages.
* HTML: Minor cleaning up of layout code.
* HTML: Use new LibCSS API for unit conversion.
* HTML: Improved ordered list handling.
* CSS: Updated selection callbacks to latest LibDOM API.
* Options: Added core option to disable CSS.
* Options: Added core option to prefer dark mode.
* Options: Improved user choices file processing.
* Bitmap: Opaque testing now implemented in core.
* Bitmap: Added core support for premultiplied-alpha.
* Bitmap: Format logged on startup.
* Bitmap: Added support for pixel colour component order configuration.
* Bitmap: Added colour component order conversion functions.
* Bitmap: Generally more optimal on all platforms for all image formats.
* Image: JPEGXL image handler.
* Image: Compatibility with latest rSVG version.
* Image: Improved handling for broken GIF images.
* Image: Updated to new LibNSGIF API.
* Image: Updated all image format handlers to use new core bitmap capabilities.
* Fetch: Updated to new libcurl API.
* Fetch: Disabled TLS1.0 and TLS1.1.
* Fetch: Improved handling of bad SSL connections.
* Fetch: Change to libcurl to optimise HTTPS connections (upstreamed).
* Local history: More robust rendering.
* Resources: Updated certificate bundle.
* JavaScript: Minor updates to DOM bindings.
* JavaScript: Updated to Duktape 2.7.0 release.
* JavaScript: Console: Don't log through closed window.
* Utility: Cleaned up UTF8 handling.
* Utility: Improved recursive directory removal.
* Utility: Add support for xx_YY format language codes.
* CI: Various improvements to build automation and testing.
* General: Various warning fixes.
* General: Aligned UserAgent with compatibility spec.
* Documentation: Updated URLs to https.
* Documentation: Added front-end development guide.
* Text areas: Clear selection on word left/right.
* Buildsystem: Fixed handling of removed header files.
* Disc cache: Minor fixes.
* Debug: Added generated charts to image cache stats page.
* Debug: Added descendant bounding boxes to HTML box tree dumps.
* Built in: Cleaned up generated `about:` pages.

* LibParserUtils library ?.?.? (parser building utility functions):
  - Optimised consuming from buffer.
  - Optimised endian detection.
  - Added new API to append vector to buffer.

* Hubbub library ?.?.? (HTML parser):
  - Massively optimised element type detection using perfect hash.
  - Optimised and updated performance tester.
  - Fixed bitrot in tests.
  - Improved example client code.
  - Buildsystem improvements.

* LibCSS library ?.?.? (CSS parser and selection engine):
  - Added support for SVG `fill-opacity` property.
  - Added support for SVG `stroke-opacity` property.
  - Added support for CSS property wide `revert` value.
  - Added support for CSS property wide `unset` value.
  - Added support for CSS property wide `initial` value.
  - Added support for CSS `position` property `sticky` value.
  - Added support for CSS `display` property "grid" values.
  - Added support for `prefers-color-scheme` media query.
  - Added new public API for CSS unit conversion.
  - Added support for predefined counter styles.
  - Optimised media query handling.
  - Made selection code generator deterministic.
  - Various selection code generator improvements.
  - Squashed leak of system font names.
  - Improved internal handling of property units.
  - Improved internal string map.
  - Minor buildsystem improvements.
  - Improved example code.
  - Added new tests.

* LibDOM library ?.?.? (Document Object Model):
  - Fixed XML parser error handling.
  - Fixed XML parser empty document handling.
  - Added DOMTokenList implementation.
  - Added DOM tree walking function.
  - Improved example code.
  - Fixed HTML Element int32 attribute getter to handle signed values.
  - Various stability improvements.
  - Buildsystem improvements.

* LibNSGIF library 1.0.0 (GIF support):
  - Complete rewrite.
  - New API that doesn't expose internal state.
  - Much better handling of bad or broken GIFs.
  - Support for decoding to client's choice of pixel colour component order.
  - Many fixes.
  - Faster decoding.
  - Updated documentation.

* LibSVGTiny library ?.?.? (SVG support):
  - Fixed X11 example utility build.
  - Implemented path arc correctly.
  - Updated documentation.

### RISC OS-Specific

* Removed last vestiges of plugin support.
* Added Choices option to disable CSS.
* Updated licence information.
* Updated links to use https in documentation.
* Fixed broken links in documentation.
* Support for building with `arm-riscos-gnueabi` toolchain.
* Updated bundled resources.
* Improved bitmap rendering.
* Fixed EX0 EY0 "high DPI" rendering.
* Text selection support in URL bar (RO5.28 onwards).
* Dragging favicon saves whole URL.
* Updated to new RUfl API.
* Fixed font scanning on startup behaviour.
* Unified redraw code for browser windows and other core-rendered windows.
* Fixed auto-scroll crash when pointer leaves core window.
* Allow drag and drop loading of WEBP image format.

* RUfl library ?.?.? (RISC OS Unicode support):
  - Added astral character support.
  - RUfl_cache version now in filename.
  - Support for multiple versions of RUfl_cache coexisting.
  - Substitution table reworked for astral characters and heavily optimised.
  - Render 6-digit replacements for codepoints outside Basic Multilingual Plane.
  - Refactoring and many code improvements.
  - Support for UCS-aware Encoding files.
  - Various API changes.
  - Detect overlong and invalid UTF-8 sequences.
  - Improved compatibility with different Font Manager versions.
  - Fixed menu building to cope with system with no fonts.
  - Ignore UCS fonts if using a non-UCS Font Manager.
  - Remove assumption that pointers are 32-bit.
  - Added test infrastructure and many tests.
  - Buildsystem improvements.

### GTK-Specific

* Cleaned up initialisation.
* Various build warning fixes.
* Fixed crash when destroying scaffolding.
* Don't create zero-sized bitmaps.
* Configure core to use Cairo's bitmap format.
* Added support for cursor word left/right key bindings.
* Added support for delete word left/right key bindings.
* Added back/forward mouse button processing.
* Fixed path plotter.
* Made UI resources more consistent.

### Amiga-Specific

* Added page theme option.
* Improved bitmap handling.
* Improved and optimised Unicode handling.
* Stability improvements.

### Windows-Specific

* Buildsystem: Use pkg-config.
* Support Ctrl+A in address bar.

### Framebuffer-Specific

* Minor internal font fixes.
* Improved documentation.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.10
------------

### Core / All platforms

* Improved handling of NetSurf generated queries and errors, including:
  - Timeouts.
  - Authentication requests.
  - Certificate problems.
* Created consistent visual style for NetSurf Core generated content.
* HTML layout fix for {min|max}-width on replaced elements.
* Improved keyboard navigation.
* Improved local history with keyboard navigation and improved display.
* Fixed mouse interaction issue when entire document had visibility:hidden.
* Refactoring of the browser window handling.
* Refactoring of the HTML content handler.
* Refactoring of SSL certificate handling.
* Refactoring of HTML, text search and selection code.
* Many improvements to JavaScript robustness.
* Many new and improved Duktape JavaScript bindings.
* Improved core/front end interface for content scaling.
* Much improved content caching, both in memory and disc caching.
* Added a generic hashmap, used by the disc cache code.
* Fetcher improvements for robustness and performance.
* Updated JavaScript engine to Duktape 2.4.0.
* Improved handling of aborted fetches.
* Improved integration testing with framework built around monkey front end.
* Improvements to monkey front end to make testing more extensive and reliable.
* Minor HTML form improvements.
* Minor HTML handler optimisations.
* Fixes to HTML handler for robustness.
* Refactoring generation of about: pages.
* Fixed save complete not to save multiple copies of the same files.
* Change how animated GIFs are rate-limited to be in line with other browsers.
* Added a page info core window.
* Clamping screen DPI to a sensible range.
* Initial work towards supporting HTML5 canvas element.
* Various treeview fixes.

* Hubbub library 0.3.7 (HTML parser):
  - Minor code quality improvements.
  - Fixes for some HTML5 element handling.

* LibCSS library 0.9.1 (CSS parser and selection engine):
  - Fixed a problem in the parsing of invalid media blocks.
  - Minor code quality improvements.

* LibDOM library 0.4.1 (Document Object Model):
  - Improved HTML TextArea element handling.
  - Fix SubtreeModified event to be fired after updating attribute values.
  - Support finalising the namespace strings.
  - Basic support for keypress events.
  - Improvements to event dispatch event lifetime handling.
  - Support for InnerHTML.
  - Added support for HTMLCanvasElement.
  - Minor code quality improvements.
  - Fixed HTMLElement property setters.
  - Fixed HTMLTitleElement text getter/setter.

* LibNSBMP library 0.1.6 (BMP support):
  - Minor code quality improvements.

* LibNSLog library 0.1.3 (Filterable logging):
  - Added support for short log level names.

* LibWapcaplet library 0.4.3 (String internment):
  - Free global context if no strings leak.
  - Enabled build with toolchains that don't support statement expressions.

### RISC OS-specific

* Added Page Info url bar icon and window.
* Updated to improved core content scaling interface.
* Add support for scrolling core windows in response to core requests.
* Removed login window because it's now handled by the core.
* Fixed use after free in dialog handling.
* Fixed cookie window re-opening bug.

### GTK-specific

* Huge reworking of the browser interface to be more modern.
  - Particularly toolbar and menu code.
* Default buildsystem target changed from gtk2 to gtk3.
* Added Page Info icon.
* Use Curl's default path for the ca-bundle.
* Updated to improved core content scaling interface.
* Run scheduler before fetching file descriptor set.
* Add support for scrolling core windows in response to core requests.
* Removed login window because it's now handled by the core.
* Moved favicon from URL entry widget to tab.
* Improved cursor focus handling.

### AmigaOS-specific

* Added Page Info icon and window.
* Updated to improved core content scaling interface.
* Add support for scrolling core windows in response to core requests.
* Allow running JavaScript from ARexx.
* Removed login window because it's now handled by the core.

### Windows-specific

* Added Page Info icon.
* Improved Unicode handling.
* Add support for scrolling core windows in response to core requests.
* Removed login window because it's now handled by the core.
* Improved command line handling.
* Improved error reporting.

### Haiku-specific

* Run scheduler before fetching file descriptor set.

### Atari-specific

* Removed login window because it's now handled by the core.

### Framebuffer-specific

* Add support for scrolling core windows in response to core requests.
* Added keyboard controls for scaling.
* Added some glyphs to the internal font.
* Can now enumerate available backend surfaces.

* LibNSFB library 0.2.2 (NetSurf Framebuffer):
  - Optimisation to pixel/colour conversion.
  - Added backend surface enumerator.
  - Minor code quality improvements.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.9
-----------

### Core / All platforms

* Buildsystem improvements.
* Improved URL query string handling.
* Improved form submission code.
* Improved error handling in HTML box construction.
* Much improved testing, with automated test frontend.
* Improved treeview rendering.
* Improved HTML content handling of viewport dimensions.
* Improved debug logging of JavaScript handling.
* Added support for Webp image format.
* Fixed SVG stroke width plotting.
* Lots of new JavaScript bindings.
* Support for script insertion after beginning conversion.
* Added support for CSS media queries.
* Various JavaScript related fixes.
* Window is now supported as a JavaScript event target.
* Support for nodeList indexing in JavaScript handler.
* Allowed front ends to execute JavaScript scripts.
* Optimised data URL handling.
* Improved support for logging to JavaScript console.
* Squashed several leaks.
* CSS pixel support: Better rendering on high DPI screens.
* Made curl fetcher use the standard NetSurf logging API.
* Fixed incremental redraw, while fetching HTML images.
* Improved HTTP header Cache-Control parsing.
* Cleaned up handling of aborted fetches and loading content.
* Improvements to generated JavaScript bindings.

* Hubbub library 0.3.6 (HTML parser):
  - Fix for script insertion during parse.

* LibCSS library 0.9.0 (CSS parser and selection engine):
  - Added support for parsing CSS Media Queries Level 4.
  - API for adding sheets to a selection context now takes
    a Media string.
  - The selection API now allows the client to describe the
    media that is being selected for.
  - Selection now honours width/height feature queries.
  - Removed the uncommon property extension blocks in
    computed style data.  This simplfies the code and
    fixes a subtle bug in the cascade/composition handling.
  - Fixed leak where rules had over 256 selectors.

* LibDOM library 0.4.0 (Document Object Model):
  - Removed dom_string implementation details from
    public header.
* LibDOM library 0.3.4 (Document Object Model):
  - Fixed a few places where public API didn't ref the
    objects it returned.
  - Fixes to the test generator.
  - Initial support for SCRIPT element flags.
  - Added dom_node_contains() call.
  - Fixed leaks in DOM node set text context.

* LibNSPSL library 0.1.3 (Public suffix list):
  - Updated public suffix list.

### GTK-specific

* Fixed HTTP authentication dialogue NULL dereference.

### AmigaOS-specific

* Improved logging.
* Better handling of local charset.
* Improved the sheduler.
* Removed RISC OS ,type extensions from resource files.
* Support building against AmiSSL instead of OpenSSL.
* Squashed many build warnings.
* Tidied up gui_window handling.
* Fixed SimpleRefresh mode and made it default.

### Windows-specific

* Added clipboard support.
* Fixed keyboard input.

### Haiku-specific

* Fixed package version.

### Atari-specific

* Buildsystem: Fixed binary stripping.

### Framebuffer-specific

* Improved buildsystem.
* Support for internationalized UI resources.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.8
-----------

### Core / All platforms

* Updated to be compatible with latest LibCSS.
* Updated Duktape to version 2.3.0.
* Added support for new CSS units.
* Fixed resource cleanup in the backing store.
* Added support for CMYK/YCCK JPEGs.
* Replaced tables are now treated as blocks.
* Improved percentage max-width handling.
* Improved browser history handling to be more robust.
* Buildsystem dependency improvements.
* Improved handling of messages and bundled resources.
* Added support for HTTP Strict Transport Security.
* Enabled TLS session tickets.
* Restricted the HTTPS ciphersuites allowed.
* Reorganised source code for HTML and text handling.
* Improved plotter API consistency and completeness.
* Enabled front ends to get current URL with fragment IDs.
* Fixed rendering glitch in treeviews.
* Fixed dragging to current selection in treeviews.
* Allowed building with address sanitiser.
* Suppressed fetch poll logging.
* Suppressed logging of data: URLs.
* Refactored HTTP header processing in llcache.
* Revised front end API for HTTP authentication.
* Display application/json as text, instead of download.
* Fixed JavaScript related CSS crash.

* Hubbub library 0.3.5 (HTML parser):
  - Squashed leaks in tests.

* LibCSS library 0.8.0 (CSS parser and selection engine):
  - Added support for CSS flexbox properties.
  - Added support for new CSS units.
  - Auto-generated source code for computed style handling.
  - Squashed some undefined behaviour issues.
  - Unified some duplicated media handling code.

* LibDOM library 0.3.3 (Document Object Model):
  - Fixed some resource leaks.
  - Fixed some flawed error handling.
  - Fixed some tests.

* LibNSGIF library 0.2.1 (GIF support):
  - Tiny optimisation to LZW decoding.
  - Cleaned up test suite.

* LibNSBMP library 0.1.5 (BMP support):
  - Fixed a few issues caught by address sanitiser and
    undefined behaviour sanitiser.

* LibSVGTiny library 0.1.7 (SVG support):
  - Fixed a few issues caught by address sanitiser and
    undefined behaviour sanitiser.

* LibNSPSL library 0.1.3 (Public suffix list):
  - Updated public suffix list to current edition.

* LibNSLog library 0.1.2 (Filterable logging):
  - Tweaks to suppress warnings on openbsd.

### RISC OS-specific

* Improved iconsprite selection based on alpha support.
* Translate resource paths.
* Drag saving the URL bar now includes fragment IDs.

### GTK-specific

* Fixed bugs in toolbar configuration.
* Improved accelerator (keyboard shortcut) handling.

### AmigaOS-specific

* Removed Iconify gadget for OS3.
* Fixed radio button labels on OS3.
* Fixed switch from public screen to own screen.
* Minor optimisations.
* Fixed HTTP authentication login window resource ownership.
* Improved Unicode handling in HTTP authentication login window.
* Close HTTP authentication login window on quit.

### Windows-specific

* Fixed image scaling crash.
* Added support for HTTP authentication.

### Framebuffer-specific

* Added support for HOME and END keys.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.7
-----------

### Core / All platforms

* CSS computed styles shared globaly between elements.
* Fixed OpenSSL 1.1.0 X509 certificate handling.
* Updated for compatibility with upstream libutf8proc.
* Updated JS engine to Duktape 2.1.0.
* Improved treeview robustness.
* Local history uses system colours.
* Made local history use corewindow interface.
* Simplified local history code.
* Added EventListener support to Duktape binding.
* Added EventTarget JavaScript binding.
* Improved plotter API.
* Simplified HTML border plotting.
* Make disabled form elements read-only.
* Improved URLdb.
* Improved tests.
* Fixed bug in content state machine for HTML contents.
* Fixed issue in parsing of file urls.
* Split out ASCII locale-safe handling used in parsers.
* Made libcurl build-time optional.
* Hotlist saved on change, rather than at exit.
* Improved handling of display:table applied to :after.
* Added support for CSS3 box-sizing to HTML layout.
* Improved handling of margins where constrained by {min|max}-width.
* Cleaned up content message broadcasts.
* Logging now run-time filterable using LibNSLog.
* Added search feature to treeviews.
* Browser history now remembers page scroll offsets.

* Hubbub library 0.3.4 (HTML parser):
  - Support falling back to space-separated charset.

* LibCSS library 0.7.0 (CSS parser and selection engine):
  - Computed styles can now be shared between elements.
    - Computed styles interned, so only unique computed styles
      reside in memory.  This means multiple pages/tabs/windows
      can share computed style data.
    - Where possible, we now bypass CSS selection, and simply
      take a reference to another element in the DOM's style,
      if we're sure it will be the same.
  - Added support for the CSS3 box-sizing property.
  - Fixed invalid read when removing sheet from selection context.
  - Fixed example / demo app build.

* LibDOM library 0.3.2 (Document Object Model):
  - Add support for checking if an event is in dispatch.
  - Add is_initialised flag to events.
  - Add getter/setter for event_target is_trusted.
  - Simplified API for removing all instances of a listener.

* LibNSGIF library 0.2.0 (GIF support):
  - Complete rewrite of LZW decompression is more optimal and robust.
  - Stability fixes.

* LibSVGTiny library 0.1.6 (SVG support):
  - Buildsystem improvements.
  - Fixed bug in parsing of multiple lineto commands.

* LibNSPSL library 0.1.2 (Public suffix list):
  - Updated public suffix list.

* LibNSLog library 0.1.0 (Filterable logging):
  - First release.

### RISC OS-specific

* Improved quit/exit behaviour.
* Moved treeviews to use the corewindow interface.
* Moved local history to use the core window interface.
* Updated to new plotter API.

### GTK-specific

* Moved treeviews to use the corewindow interface.
* Moved local history to use the core window interface.
* Fixed Hotlist finalisation on exit.
* Updated to new plotter API.
* Improved text display and DPI handling.
* Optimised tiled bitmap rendering.

### AmigaOS-specific

* Improved memory handling.
* Reduced frequency of diskfont open/close.
* Improved splash screen robustness.
* Improved quit/exit behaviour.
* Moved treeviews to use the corewindow interface.
* Moved local history to use the core window interface.
* Improved redrawing.
* Improved menus.
* Updated to new plotter API.
* Reduced use of globals.
* Improved use of notifications.
* Improved AmigaOS3 compatibility.

### Haiku-specific

* View source in user's preferred editor.
* Reworked menu accelerator handling.
* Fixed popup menu.
* Updated to new plotter API.

### Atari-specific

* Updated to new plotter API.
* Don't package the ca-bundle twice.

### Framebuffer-specific

* Fixed font handling return types.
* Moved local history to use the core window interface.
* Updated to new plotter API.

### Windows-specific

* Fixed font handling return types.
* Fixed Windows installer generation.
* Updated to new plotter API.
* Moved local history to use the core window interface.
* Fixed text handling.
* Fixed redraw bug on resize.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.6
-----------

### Core / All platforms

* Build system improvements.
* Internal code reorganisation.
* Improved time handling.
* Removed use of warn_user.
* Updated Duktape JavaScript engine to version 1.5.1.
* Removed old spidermonkey build options.
* Fixed certificate chain handling.
* Improved URL (un)escape code.
* Added unit tests.
* Allowed build without libcurl and openssl (lacks http(s) support).
* Added parser for HTTP date time strings.
* Simplified treeview initialisation and finalisation.
* Added treeview font size user option.
* Improved locale handling, by using with ascii-only parsing routines.
* Fixed blocking of supercookies, and updated public suffix list.
* Improved User Agent default CSS.

* LibWapcaplet library 0.4.0 (String internment):
  - Added function to get caseless hash value.

* LibSVGTiny library 0.1.5 (SVG support):
  - Added tests.
  - Improved handling of bad SVGs.
  - Improved linear gradient handling.

* LibNSGIF library 0.1.4 (GIF support):
  - Improved tests.

* LibNSBMP library 0.1.4 (BMP support):
  - Improved tests.
  - Improved documentation.
  - Fixed RLE decoding.
  - Improved handling of bad BMPs.
  - Improved BMP alpha channel handling.
  - Fixed row padding handling.

* LibDOM library 0.3.1 (Document Object Model):
  - Fixed DOM tree walking routine.
  - Fixed Expat parser binding.

* LibCSS library 0.6.1 (CSS parser and selection engine):
  - Minor fixes.
  - Updated to use libwapcaplet caseless hash value API.

* LibNSPSL library 0.1.0 (Public suffix list):
  - First release.

### GTK-specific

* Fixed TLS certificate viewer, with multiple queries.
* Switched to corewindow API for treeviews for improved robustness.
* Fixed About dialogue licence and credits buttons.
* Fixed MIME types of local BMP files.

### AmigaOS-specific

* Make use of guigfx library for improved image display.
* Fixed websearch crash.
* Code cleanup.
* Improve character encoding conversion.
* Improved search engine list.
* Improvements to aid compiler optimisation.
* Improved scheduler.
* Improved tab handling, when launching URLs with existing NeSurf instance.
* Improved AmigaOS3 support.

### Framebuffer-specific

* LibNSFB library 0.1.5 (NetSurf Framebuffer):
  - Minor buildsystem changes.

### Haiku-specific

* Support http(s) schemes URI schemes in the .desktop.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.5
-----------

### Core / All platforms

* Fixed URL parsing to avoid locale-specific character handling.
* Improved documentation of font APIs.

### RISC OS-specific

* Improved !Boot and !System distributions.
* Updated distributed SharedUnixLibrary.
* Improved relocatable module checks.

### GTK-specific

* Fixed resource handling.
* Removed theme handling code.
* Cleaned up 401 login window.

### AmigaOS-specific

* Fixed bug that caused NetSurf to fail to exit.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.4
-----------

### Core / All platforms

* Simplified web page thumbnailing interface between core and front ends.
* Buildsystem improvements.
* Added prefix to key codes to avoid namespace conflict with ncurses.
* Fixed URL hash not to contain the URL fragment.
* Removed outdated RISC OS cross-compilation instructions.
* Improved disc cache indexing and element layout.
* Improved disc cache performance for small files.
* Moved bitmap operations to use new operation table pattern.
* Improved bitmap rendering.
* Removed support for webp image format.
* Fixed freeing of form controls.
* Minor code cleanup in handling of relative positioning.
* Added scaled content redraw interface.
* Updated default user agent CSS to support HTML5 block level elements.
* Fixed PNG handling in low memory situations.
* Automatically disable the disc cache if the disc is too slow.
* Improved LOG macro.
* Improved build-time generation of internationalised messages.
* Allowed verbose logging to be redirected to a file.
* Reduced overhead of logging.
* Moved message loading from core out to front ends.
* Improved error reporting.
* Fixed a URL parsing bug.
* Improved URL parsing tests.
* Added code coverage reporting for tests.
* Improved test coverage.
* Updated IDL specs.
* Optional UTF-8 rendering of URLs.
* Migrated to Duktape JavaScript engine.  Using Duktape 1.4.0.
* Fixed ALIGN attribute on TABLE elements.
* Fixed percentage calculation to avoid arithmetic overflow.
* Fixed JavaScript instanceof.
* Fixed asynchronous JavaScript scripts.
* Much improved JavaScript engine bindings.
* Fixed leak of frame scrollbar data.
* Fixed leak of frameset DOM documents.
* Added initial support for JavaScript events.
* Fixed overflow-{x|y} property handling.
* Improved option handling.
* Fixed mouse handling to consider the root element's box.
* Added support for auto-retrying timed-out cURL fetches.
* Fixed mime sniffing when fetcher completes with no data.
* Don't try to look up mailto URLs in database, as they aren't stored.
* Optimised HTML layout of floats.
* Fixed HTML layout bug relating to position of floats.
* Fixed bug when user toggled javascript_enabled setting during page load.
* Slight optimisation of DOM node inserted handling.
* Introduced 10s script execution timeout.
* Used new LibCSS API to optimise handling of HTML presentational hints.
* Removed logging during browser window resize.
* Improved cache logging.
* Fixed to avoid generating pseudo elements for HTML replaced elements.
* Simplified status bar updating while fetching HTML contents.
* Fixed bug in accounting of objects being used by HTML contents.
* Stopped GIF animations for when GIF has no users.

* Hubbub library 0.3.3 (HTML parser):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Updated name of JSON library used for tests.
  - Added doxygen code documentation generation support.
  - Fixed doxygen warnings.

* LibCSS library 0.6.0 (CSS parser and selection engine):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Added doxygen code documentation generation support.
  - Minor improvements to tests.
  - Added "96" to useful fixed point values in fpmath header.
  - Slight simplification of initial computed style setup.
  - Changed how presentational hints are handled to be more optimal.

* LibDOM library 0.3.0 (Document Object Model):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Added doxygen code documentation generation support.
  - Improved tests.
  - Fixed event listener removal.
  - Removed unused document argument from event layer API.
  - Improved HTML Element specialisation APIs to better match IDL.
  - Added a _FINISHED event action callback.
  - Handle cleanup during event target gathering.
  - Ensured HTML element attributes are lower cased.
  - Added asynchronous property to script element.
  - Fix event handling not to capture and bubble on event target.
  - Added support for Event.eventPhase.
  - Fixed event target listener destruction.
  - Fixed HTMLDocument's vtable, so .textContent will work.
  - Fixed handling of document base URI.
  - Improved and optimised handling of HTMLElement specialisations.
  - Fixed copy constructors for HTMLElement and its specialisations.
  - Optimised DOM string upper/lower-case conversion.
  - Optimised event dispatch.
  - Provided public API for efficient identification of HTML elements.

* LibNSBMP library 0.1.3 (NetSurf BMP decoder):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Added BMP decode test suite.
  - Added ICO decode test suite.
  - Fixed calculation in RLE decoding. (CVE-2015-7508)
  - Improved input data range check for RLE4 absolute mode.
  - Added range check to colour table accesses. (CVE-2015-7507)

* LibNSGIF library 0.1.3 (NetSurf GIF decoder):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Added basic decode test.
  - Improved checks for buffer exhaustion.
  - Ensured LZW decode stack does not overflow. (CVE-2015-7505)
  - Ensured LZW code is valid before use. (CVE-2015-7506)

* LibParserUtils library 0.2.3 (parser building utility functions):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.

* LibSVGTiny library 0.1.4 (SVG support):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Fixed path buffer length allocation issue.
  - Added test files.

* LibWapcaplet library 0.3.0 (String internment):
  - Fixed paths for Haiku.
  - Minor buildsystem changes.
  - Added assertions to interface macros.
  - Updated tests.
  - Added doxygen code documentation generation support.
  - Fixed doxygen warnings.
  - Added function to return lower-cased strings.

### RISC OS-specific

* Improved full-save output to retain their creation date on copy.
* Fixed closure of menu when asked for clipboard data.
* Updated bundled Shared Unix Library support module to 1.12.
* Fixed object info crash that occurred with NULL URLs.
* Doxygen generated code documentation fixes and improvements.
* Many minor cleanups.
* Converted to bitmap operations table.
* Set the default disc cache size to zero (off).
* Squashed a memory leak in path handling.
* Improved menu keyboard-shortcuts.
* Prevent sprite export when image can't be converted.
* Fixed rendering in 4k and 64k colour modes.
* Increased size of download window.
* Fixed page-search ignoring case sensitivity.
* Fixed lingering pointer shape on window close bug.

### GTK-specific

* Extended GTK 2/3 compatibility layer for changes in recent GTK 3 releases.
* Fixed warnings in about dialogue construction.
* Many minor UI changes and improvements.
* Cleaned up header usage.
* Converted to bitmap operations table.
* Improved view source behaviour.
* Memory leak fixes.
* Use of GResource to improve building of image resources.
* Made path to resources more robust.
* Removed GTK-specific default CSS file.
* Allow resources to be built into the binary, rather than loaded from disc.
* Improve URL bar completion to navigate on selection.
* Show default favicon on window creation.
* Add option to show IDN decoded URLs.
* Improved the man page.
* Tidied up themes code.
* Fixed memory leaking of Pango layouts when rendering text.

### AmigaOS-specific

* Fixed screen depth check for friend bitmaps.
* Made use of friend bitmaps optional.
* Removed asynchronous backing store code.
* Allowed URL completion to pick up non-visited URLs.
* Converted to bitmap operations table.
* Improved rendering.
* Improved plotted robustness.
* Improved menus.
* Improved file extension handling when saving.
* Cache pre-scaled versions of favicons.
* Added support for UTF-16 surrogates.  Enables emoji rendering.
* Improved AmigaOS 3 support.
* Improved font handling.
* Show IDN decoded URLs, subject to local charset restrictions.
* Added new keyboard shortcuts.
* Improved hotlist toolbar look.
* Optimised font lookups.
* Allowed specifying of user directory locations on command line.
* Further improvements to AmigaOS 3 support.
* Improved font engine selection.
* Fixed build warnings.

### Mac OS X-specific

* Updated for building with SDK 10.10.
* Various clean-ups.

### Haiku-specific

* Converted to bitmap operations table.
* Fixed 64-bit builds.
* Improved build instructions.
* Use core menu widget for SELECT form entry.
* Disabled replicant dragger.
* Improved about dialogue box.
* Improved menus.
* Embedded language resources in executable.
* Added cookie management support.
* Added web search bar.

### Atari-specific

* Converted to bitmap operations table.
* Cleaned up inconsistency of source code line endings.
* Enabled JavaScript-capable builds.

### Framebuffer-specific

* Reduced amount of verbose logging.
* Converted to bitmap operations table.
* Added glyphs to built-in bitmap font.

* LibNSFB library 0.1.4 (NetSurf Framebuffer):
  - Minor buildsystem changes.
  - Added checks to ensure non-zero size framebuffer.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.3
-----------

### Core / All platforms

* Rewrote html mouse pointer handling to reduce stack usage.
* Improved portability of the curl binding.
* Fixed relative URLs in inline CSS to resolve with respect to base URL.
* Improved debug box tree dump.
* Fixed textarea rendering glitches.
* Fixed textarea scrollbar behaviour.
* Removed support for all SSL versions due to vulnerabilities.
* Protect against inappropriate encryption protocol downgrades.
* Many changes to sanitise interface between core and front ends.
* Front ends can use their own main loop instead of polling core.
* Fixed bad background image URLs causing page load to abort.
* Improved buildsystem handling of warning flags.
* Improved HTML form handling.
* Improved the disc cache.
* Improved the hotlist/bookmarks loading and saving.
* Improved error handling.
* Added copy-to-clipboard of selected hotlist & history entries.
* All die calls removed; errors now passed to front ends.
* Rationalised URL handling functionality.
* Added API for front ends to set DPI.
* Added simple map access a about:maps.
* Improved documentation for cross-compiling.
* Many minor improvements and fixes.

* Hubbub library 0.3.1 (HTML parser):
  - Updated buildsystem.
  - Minor code tidying.

* LibCSS library 0.5.0 (CSS parser and selection engine):
  - Added selection support for CSS3 column-count property.
  - Added selection support for CSS3 column-fill property.
  - Added selection support for CSS3 column-gap property.
  - Added selection support for CSS3 column-rule-color property.
  - Added selection support for CSS3 column-rule-style property.
  - Added selection support for CSS3 column-rule-width property.
  - Added selection support for CSS3 column-span property.
  - Added selection support for CSS3 column-width property.
  - Added selection support for CSS3 break-after property.
  - Added selection support for CSS3 break-before property.
  - Added selection support for CSS3 break-inside property.
  - Improved and fixed absolute value conversion of computed styles.
  - Minor optimisations.
  - Updated buildsystem.
  - Improved selection tests.
  - Code tidying.
  - Many minor improvements and fixes.

* LibDOM library 0.1.2 (Document Object Model):
  - Updated buildsystem.
  - Minor fixes and improvements.

* LibNSBMP library 0.1.2 (NetSurf BMP decoder):
  - Minor code tidying.
  - Updated buildsystem.

* LibNSGIF library 0.1.2 (NetSurf GIF decoder):
  - Updated buildsystem.

* LibParserUtils library 0.2.1 (parser building utility functions):
  - Updated buildsystem.

* LibSVGTiny library 0.1.3 (SVG support):
  - Fix for relative move commands after path close.
  - Fix parsing of whitespace in transform operators.
  - Fix for path generation overrunning allocated storage.
  - Updated buildsystem.

* LibWapcaplet library 0.2.2 (String internment):
  - Updated buildsystem.

### RISC OS-specific

* Updated to use new core interface.
* Cleaned up the bundled !Cache distribution.
* Divorced from core-internal browser window data structure.
* Fixed user-launched fetches to be verifiable.
* Minor code tidying.

### GTK-specific

* Updated to use new core interface.
* Updated to use own main loop.
* Fixed thumbnailing issue.
* Added man page and XDG file..
* Divorced from core-internal browser window data structure.
* Various minor fixes and improvements.

### AmigaOS-specific

* Updated to use new core interface.
* Improved filenames in save requesters.
* Improved kiosk window handling.
* Improved browser window scrollbar behaviour.
* Improved library handling.
* Divorced from core-internal browser window data structure.
* Many minor fixes and improvements.
* Simplified handing of core browser windows and tab management.
* Improved initialisation, and main loop.
* Rewritten scheduler.
* Improved Style Guide compliance.
* Added YouTube video helper script.
* Removed obsolete GetVideo support.
* Write to the disc cache asynchronously.
* Added preliminary support for AmigaOS 3.
* Code tidying.

### BeOS/Haiku-specific

* Updated to use new core interface.
* Divorced from core-internal browser window data structure.
* Fixed resource fetcher.

### Atari-specific

* Updated to use new core interface.
* Divorced from core-internal browser window data structure.
* Fixed handling of local paths.
* Added support for disc cache.
* Code tidying.
* Many minor fixes and improvements.

### Framebuffer-specific

* Updated to use new core interface.
* Divorced from core-internal browser window data structure.
* Renamed internal font to "ns-sans".
* Added man page.
* Updated to use own main loop.
* Improved command line option handling.

* LibNSFB 0.1.2 library (NetSurf Framebuffer):
  - Fixed build of wayland surface.
  - Updated buildsystem.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.2
-----------

### Core / All platforms

* Improved building on OpenBSD.
* Improved interface between front ends and core.
* Improved internal URL handling.
* Cleaned up many internal interfaces.
* Added support for disc caching.
* Fixed treeview selection issue.
* Fixed line breaking of plain text with multi-byte sequences.
* Improved out-of-memory handling.
* Added support for overflow-x and overflow-y CSS3 properties.
* Improved web search feature.
* Improved internal task scheduling.
* Improved support for Unicode URLs.
* Fixed issue relating to CSS imports.
* Improved debug dumping of box trees.
* Fixed SSL certificate viewer lifetimes bug.
* Fixed HTML ordered list numbering issue.
* Fixed DOM tree debug dump.
* Fixed handling of malformed URLs.
* Fixed text selection.

* LibCSS library 0.4.0 (CSS parser and selection engine):
  - Added support for CSS3 overflow-x and overflow-y properties.
  - Fix to !important handling with play-during property.
  - Improved test coverage.

* LibDOM library 0.1.1 (Document Object Model):
  - Implemented many missing specialisations of HTMLElement.

### RISC OS-specific

* Fix for toolbar width calculation.
* Code clean-ups.
* Added option to use disc cache.

### GTK-specific

* Improved configuration handling.
* Improved command line usage.
* Enabled disc caching.
* Improved context menu.
* Fixed gdkpixbuf usage issue.
* Improved view source, and debug features.
* Improved main menu.
* Improved developer options.
* Added options to open view source, etc, in new window, tab, or editor.
* Many code clean-ups.
* Internationalisation fixes.

### AmigaOS-specific

* Removed Cairo.
* Improved installer script.
* Enabled disc caching.
* Improved line splitting behaviour for text wrap.
* Improved web search.
* Improved hotlist menu.
* Improved task scheduler.
* Wait for network activity, rather than polling for it.
* Improved throbber operation.
* Fixed tab bar update failure issue.
* Improved favicon handling.

### BeOS/Haiku-specific

* Various fixes.

### Atari-specific

* Made several little bug fixes.

### Framebuffer-specific

* Added Unicode support to internal font.
* Added codepoint rendering to internal font, for missing glyphs.
* Added scaled glyph rendering to internal font.
* Added support for window resizing, were underlying surface supports it.

* LibNSFB library 0.1.2 (NetSurf Framebuffer):
  - Made SDL surface resizeable.
  - Fixed rendering for 1bpp glyphs with greater than 1 byte width.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.1
-----------

### Core / All platforms

* Optimised UTF-8 conversion.
* Optimised URL database lookups.
* Added support for visited link rendering.
* Made better use of DOM events to optimise load times.
* Improved handling of user options.
* Improved about:config page.
* Faster application startup time.
* Fixed rendering of animated background images.
* Rewritten treeview implementation with improved look and behaviour.
* Rewritten hotlist (bookmarks, favourites) with new treeview.
* Rewritten global history with new treeview.
* Rewritten cookie manager with new treeview.
* Added multi-level undo and redo support to textareas.
* Hotlist has special directory for new 'unsorted' entries.
* Improved file:// directory listings.
* Faster CSS selection achieved by updating to new LibCSS version.
* Improved support for min-height and max-height CSS properties.
* Removed support for MNG and JNG images.
* Launch image fetches during HTML parsing.
* Improved support for file upload form element.
* Enabled TLS 1.2 support again.
* Fixed various issues with HTML forms by using the DOM properly.
* Make better use of string interning.
* Simplified favicon handling.
* Fixed fetch handle caching with modern libcurl versions.
* Simplified local history programmer interface.
* Improved handling of invalid HTML.
* Improved handling of browser tabs.
* Various fixes and improvements to URL handling.
* Fixed textarea selection redraw.
* Improved separation of core and platform-specific code.
* Improved content caching.
* Improved build documentation.
* Improved display of single-line form text inputs.
* Various fixes to table layout and rendering.

* Hubbub library 0.3.0 (HTML parser):
  - Updated to use latest LibParserUtils.
  - Removed client allocation function.
  - Added support for multilib.

* LibCSS library 0.3.0 (CSS parser and selection engine):
  - Added support for the writing-mode property.
  - Significantly optimised selection performance.
  - Made many fixes and internal clean-ups.
  - Added support for multilib.

* LibDOM library 0.1.0 (Document Object Model):
  - Fixed node comparison.
  - Made many fixes and internal clean-ups.
  - Updated Hubbub binding for latest Hubbub.
  - Added support for multilib.
  - Added HR specialisation of HTMLElement.

* LibNSBMP library 0.1.1 (NetSurf BMP decoder):
  - Added support for multilib.

* LibNSGIF library 0.1.1 (NetSurf GIF decoder):
  - Added support for multilib.

* LibParserUtils library 0.2.0 (parser building utility functions):
  - Removed client allocation function.
  - Added support for multilib.
  - Minor fixes and clean-ups.

* LibROSprite library 0.1.1 (RISC OS Sprite support for non-RO platforms):
  - Added support for multilib.

* LibSVGTiny library 0.1.1 (SVG support):
  - Several minor fixes.
  - Added support for multilib.

* LibWapcaplet library 0.2.1 (String internment):
  - Added support for multilib.

### RISC OS-specific

* Improved mouse handling.
* Added F8 and F9 key shortcuts for textarea undo and redo.
* Optimised rendering of treeview windows.
* Added hotlist indicator to URL bar.
* Improved handling of long URLs in URL bar.
* Improved menu handling.
* Reduced dependency on core's implementation details.

### GTK-specific

* Fixed scroll wheel step size.
* Improved GTK-3 compatibility.
* Fixed red/blue swapping with animated gifs.
* Added new tab button to tab bar.
* Improved window/tab closing.
* Added key shortcuts for textarea undo and redo.
* Improved save source.
* Added support for file upload form element.
* Reduced dependency on core's implementation details.
* Fixed alpha channel handling
* Fixed endian issues in bitmap handling.
* Improved support for keyboard input of non-latin letters.
* Improvement of text caret rendering.

### AmigaOS-specific

* Added key shortcuts for textarea undo and redo.
* Use OS mouse pointers.
* Improved hotlist menu.
* Added favourites/hotlist indicator to URL bar.
* Reduced dependency on core's implementation details.
* Added support for AmigaGuide help system.
* Improved behaviour on quit.
* Improved download handling.
* Improved URL handling.

### BeOS/Haiku-specific

* Improved embedding of resource files.
* Improved About dialogue box.
* Fixed CSS system colour setting.
* Updated to account for change in atomic_add() in Haiku.
* Reduced dependency on core's implementation details.

### Windows-specific

* Reduced dependency on core's implementation details.

### Atari-specific

* Added SSL certificate inspection window.
* Added About dialogue box.
* Added support for scaling web pages.
* Improved rendering.
* Reduced dependency on core's implementation details.

### Framebuffer-specific

* Added key shortcuts for textarea undo and redo.
* Use libnsfb for tiled bitmap rendering.
* Reduced dependency on core's implementation details.

* LibNSFB library 0.1.1 (NetSurf Framebuffer):
  - Fixed various memory leaks.
  - Added tiled bitmap rendering support.
  - Improved dithering.
  - Added support for multilib.
  - Allowed building for Win32.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 3.0
-----------

### Core / All platforms

* Internal restructuring to separate different components.
* Improved drag handling.
* Added support for User CSS file.
* Fixed compressed Messages loading.
* Updated translations.
* Improved internal options handling.
* Improved build documentation.
* Moved to using LibDOM for our document object model.
* Removed LibXML dependency.
* Merged messages files into single resource.
* Improved about:imagecache debug feature.
* Use system colours for treeview.
* Use system colours for scrollbar widget.
* Improved content cache.
* Added support for DoNotTrack header.
* Added early and primitive JavaScript support. (Disabled by default.)
* Textarea widget now supports double/triple click selections.
* Moved welcome page to local about: URL scheme.
* Improved text selection handling.
* Simplified interface for dealing with clipboard cut/copy/paste.
* Optimised file: fetcher where mmap is available.
* Allowed contents to request scrolls.
* Improved frames resizing.
* Internal restructuring of content communication with window.
* New system for obtaining debug dump from core browser window.
* Build system uses ccache where available.
* Improved handling of CSS white-space property.
* Avoid conversion of spaces to hard spaces.
* Reduced usage of talloc to box tree.
* Improved URL utility functions.
* Brought internal URL database into line with other URl handling.
* Improved title bar display for standalone image viewing.
* Try to downgrade TLS version support if server can't cope.
* Fix for layout issue involving floats with negative margins.
* Reduced overhead of debug logging.
* Don't abort HTML redraw when bad image fails to convert.
* Added support for CSS clip property.
* Improved error reporting.
* Added support for httponly cookies.
* Overhauled core textarea widget.
* Use textarea widget for HTML form text, password and textarea input.
* Improved caret and focus handling.
* Removed CPU architecture from user agent string.
* Changed text selection highlight colouring.
* Remove limitation of HTML line breaking handling that required space.
* Allowed CSS to be fetched/parsed in parallel with HTML document.
* Added support for readonly attribute on textareas and text inputs.
* Fixed text clipping issue.

* Hubbub library 0.2.0 (HTML parser):
  - Added script support.
  - Fixed handling of encoding change.
  - Updated to use new core buildsystem.

* LibCSS library 0.2.0 (CSS parser and selection engine):
  - Updated example code.
  - Added support for CSS widows and orphans properties.
  - Minor selection optimisations.
  - Several minor fixes.
  - Updated to use new core buildsystem.

* LibDOM library 0.0.1 (Document Object Model):
  - First release.

* LibNSBMP library 0.1.0 (NetSurf BMP decoder):
  - Removed suspend/invalidate stuff from API.
  - Updated to use new core buildsystem.

* LibNSGIF library 0.1.0 (NetSurf GIF decoder):
  - Updated to use new core buildsystem.

* LibParserUtils library 0.1.2 (parser building utility functions):
  - Fixed input insertion.
  - Updated to use new core buildsystem.

* LibROSprite library 0.1.0 (RISC OS Sprite support for non-RO platforms):
  - Minor improvement.
  - Updated to use new core buildsystem.

* LibSVGTiny library 0.1.0 (SVG support):
  - Ported from LibXML to LibDOM.
  - Minor fixes.
  - Updated to use new core buildsystem.

* LibWapcaplet library 0.2.0 (String internment):
  - Optimised to reduce function call overheads.
  - Updated to use new core buildsystem.

### RISC OS-specific

* Documentation menu options now launch online documentation.
* Added option to enable/disable JavaScript.
* Added distribution Zip file build target.
* Updated buildsystem to squeeze !RunImage.
* Now require v0.13 or higher of the CryptRand module.
* Added logging switch to !NetSurf.!Run.
* Consult internal mimemap table before consulting MimeMap module.
* Improve line wrap handling.
* Made clipboard use UTF-8 internally.
* Now require v0.12 or higher of the Iconv module.
* Pass double and triple clicks to the core.
* Fixed ctrl+{left|right|up|down} keys.

### GTK-specific

* Added URL launcher for NetSurf.
* Added support for building with GTK3.
* Fixed rendering of tiled background images.
* Fixed scrollwheel support.
* Fixed OpenBSD compatibility.
* Rewritten preferences handling.
* Added clipboard keyboard shortcuts for cut/copy/paste.
* Enabled support for line breaking on non-space characters.
* Improved localisation.

### AmigaOS-specific

* Disabled print support.
* Tiled redraw improvement.
* Fixed fast scrolling.
* Updated default favicon.
* Improved status bar.
* Made DataTypes picture handler use the core image cache.
* Improved font width measuring.
* Improved position in rendered string handling.
* Improved line splitting handling.
* Allow another font to be used if current font lacks required glyph.
* Fixed download window's handling of aborted/stalled downloads.
* Added option to enable/disable JavaScript.
* Support for less than 16bpp screen modes.
* Optimisation for opaque bitmap plotting.
* Removed p96 dependency.
* Install target now produces an archive for distribution.
* Added hotlist toolbar.
* Improved tabbed browsing.
* Added basic context sensitive help.
* Made various redraw optimisations.

### BeOS/Haiku-specific

* Made multiple toolbars share single icon instance.
* Optimised text rendering slightly.
* Added download window.
* Fixed off-by-one in caret placement and text selection.

### Windows-specific

* Install target creates an installer.

### Atari-specific

* Improved download window behaviour.
* Improved plotters.
* Optimised redraw areas.
* Simplified keyboard shortcut handling.
* Added native bitmap buffer for faster rendering.
* Improved toolbar.
* Improved URL bar.
* Enabled paste into URL bar.
* Faster rendering of opaque bitmaps.
* Faster freetype text rendering.
* Improved context menu.
* Added global history window.
* Improved text selection and caret placement.
* Improved font handling.
* Removed windom dependency.

### Framebuffer-specific

* Removed unnecessary redraw when not scrolling.
* Optimised scrolling.
* Reduced lag in handling mouse actions and movements.
* Improved scrollbar behaviour.
* Added text caret for URL bar and HTML forms.
* Improved URL bar editing.
* Optimised text widget redraw.
* Improved text selection and caret placement.
* Enable mouse pointers with offset 'hot-spots'.
* Improved key handling.
* Added support for drags.
* Added simple internal clipboard (cut/copy/paste) support.
* Added on-screen-keyboard (OSK) icon.
* Turned on Quit button by default.
* Improved system colours.
* Improved window width/height option handling.

* LibNSFB library 0.1.0 (NetSurf Framebuffer):
  - Fixed X surface build.
  - Fixed 1bpp bitmap rendering.  (Fixes internal font clipping.)
  - Improved 8bpp palette.
  - Optimised paletted rendering.
  - Added error diffusion support to bitmap plotter.
  - Added support for surfaces with BGR component ordering.
  - Added Wayland surface handler.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.9
-----------

### Core / All platforms

* Improved internal management of simultaneously occurring operations.
* Rewritten and streamlined URL handling.
* Improved handling of frames and iframes.
* Improved handling of min/max-width on replaced elements.
* Simplified layout object dimension calculations.
* Reduced memory usage in the fetch layer.
* Disabled extraneous fetch debug code when built without debugging.
* Optimised fetchers.
* Updated MIME sniff handling in accordance with evolution of spec.
* Optimised cache layer.
* Enabled yield and resumption of box tree construction.
* Improved resource handling.
* Added new support for favicons.
* Increased default memory cache size.
* Added about:imagecache status page.
* Optimised URL fragment handling.
* Made meta refresh handling more robust.
* Various minor table layout fixes.
* Cleaned up fetch callback API.
* New hotlist entries can be inserted into defined or selected folder.
* Enabled resizing of textarea widget.
* Fixed cookie expiration.
* Improved handling of CSS overflow scrollbars.
* Fixed caret position after deleting selection in textarea widget.
* Enabled mouse wheel scrolling of frames and other scrollable content.
* Improved handling of file drops, e.g. for HTML form file submission.
* Added the beginnings of a gstreamer binding.
* Fixed some content cache layer issues.
* Improved handling of deletion of nodes from treeviews.
* Added options to disable fetching of images.
* Simplified redraw API rendering options.
* Improved support for drag operations.

* Hubbub library 0.1.2 (HTML parser):
  - Fixed xmlns attribute handling.

* LibCSS library 0.1.2 (CSS parser and selection engine):
  - Avoid interning standard strings for every stylesheet, style
     tag and style attribute.
  - Made significant optimisations to style selection.
  - Fixed case where font-family is unspecified in input CSS.
  - Added some support for @font-face.
  - Fixed !important on opacity property.
  - Added support for parsing CSS3 Multi-column layout properties.

### RISC OS-specific

* Removed unused and broken plugin handler.
* Removed normalisation of user input URLs.
* Added support for external hotlist utilities.
* Improved signal handling.
* Improved keyboard input handling.
* Various fixes.
* Fixed Drawfile export not to show interactive features.
* Improved scroll event handling.

### GTK-specific

* Improved resource handling.
* Cleaned up handling of tabs.
* Made tab bar position configurable.
* Updated plotters to use Cairo surfaces throughout.
* Enabled search-as-you-type of page content.
* Fixed ~/.netsurf directory permissions.

### AmigaOS-specific

* Improved mouse handling.
* Improved multi-tasking behaviour.
* Increased keyboard scroll speed.
* Better MIME type handling.
* Improved contextual menu handling.
* Enabled context menu for frames.
* Enabled context menu for plain text contents.
* Improved aspect ratio handling.
* Consolidated user file operations.
* Fixed scrolling issues.
* Improved resource handling.
* Improved tab bar handling.
* Improved drag handling.
* Improved font handling.
* Improved clipboard support.

### Mac OS X-specific

* Fixed build.

### Atari-specific

* Added context menu.
* View source support.
* Use favicon when iconified.
* Removed unneeded frames handling code.
* Fixed rectangle plotter.
* Simplified status bar.
* Improved download window.
* Improved URL bar.
* Fixed conversion from local encoding to UTF-8.
* Improved scheduler.
* Added save page support.
* Added settings dialogue.
* Enabled view source feature.
* Added support for 8-bit displays.

### Framebuffer-specific

* Reduced excessive logging.
* Implemented RAM surfaces, instead of direct blitting.
* Fixed VNC surface.
* Enabled thumbnailing in local history view.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.8
-----------

### Core / All platforms

* Added rudimentary support for CSS :before pseudo element.
* Improved vertical layout of boxes and margin handling.
* Optimised layout engine not to calculate word widths it doesn't need.
* Optimised layout and redraw of pages as their images fetch.
* Simplified internal character in string location.
* Improved internal content management.
* Fixes to build system.
* Cleaned up image content handlers.
* Simplified internal content redraw interface.
* Simplified status bar text.
* Improved content handler initialisation.
* Improved scrollbar widget.
* Fixed direct dragging of scrollable boxes in HTML.
* Improved fetch option handling.
* Optimised treeview layout.
* Improved default cookie manager view.
* Added support for iframes.
* Added support for frames.
* Improved drag handling.
* Fixed text selection, search, and widget rendering in HTML redraw.
* Made page text search support more robust.
* Improved HTTP authentication header parsing.
* Removed plotter table global.
* Removed current browser global for redraw.
* Updated PNG handler for recent libPNGs.
* Refactored HTTP handling utilities.
* Improved handling of mouse input for text selection.
* Fixed stopping of active HTML content fetches.
* Fixed GCC 4.6 build warnings.
* Disallowed self-importing of CSS contents.
* Made download handling more robust.
* Simplified bitmap management.
* Improved handling of HTML font tag's size attribute.
* Added support for content type sniffing.
* Added an image cache, for managing decoded image reuse.
* Fixed handling of zero-length documents.
* Improved handling of memory cache sizes.

* Hubbub library 0.1.1 (HTML parser):
  - Fixed build with GCC 4.6.

* LibCSS library 0.1.1 (CSS parser and selection engine):
  - Improved build tree cleaning.
  - Fixed build with GCC 4.6.
  - Added support for selection of page-break properties.
  - Optimised style selection.

* LibParserUtils library 0.1.1 (parser building utility functions):
  - Changed build to require iconv by default.
  - Support Byte Order Marks in UTF16 and UTF32 streams.

### RISC OS-specific

* Improved Adjust double-click handling in treeviews.
* Consolidated content handlers for ArtWorks, Draw, and Sprite.
* Removed redundant code specific to old RISC OS frame implementation.
* Removed redundant code specific to old RISC OS iframe implementation.
* Updated for new frame and iframe support in core.
* Updated to handle new interface to core redraw functionality.

### GTK-specific

* Made new window open showing homepage, rather than current page.
* Fixed compatibility with latest GTK API.
* Updated to handle new interface to core redraw functionality.
* Updated for new frame and iframe support in core.

### AmigaOS-specific

* Improved font handling.
* Improved text charset handling.
* Added support for user defined DPI setting.
* Improved image handlers.
* Improved MIME type handling.
* Primitive plugin content support.
* Added support for additional mouse buttons.
* Updated to handle new interface to core redraw functionality.
* Improved handling of tabs.
* Improved toolbar button availability.
* Improved download window.
* Improved context menus.
* Updated for new frame and iframe support in core.
* Added a splash screen.
* Improved support for entering exotic characters.

### Mac OS X-specific

* Removed dependency on Xcode installation for MIME mappings.
* Accept URLs as arguments to binary.
* Fixed build.
* Fixed local directory listing display.
* Improved Apple image handler.
* Added view source functionality.
* Fixed crash due to broken certificate bundle path.
* Added quarantine attributes to downloaded files.
* Added NetSurf to 'Open with' menu for HTML files.
* Updated to handle new interface to core redraw functionality.
* Updated for new frame and iframe support in core.

### BeOS/Haiku-specific

* Reduced debug output.
* Updated resources.
* Updated to handle new interface to core redraw functionality.
* Updated for new frame and iframe support in core.

### Atari-specific

* Improved rendering.
* Implemented treeview support.
* Added Hotlist (bookmarks) feature.
* Simplified scheduler.
* Improved support for local files.
* Updated to handle new interface to core redraw functionality.
* Improved window resize accuracy.
* Ensured window title is converted to local encoding.
* Updated for new frame and iframe support in core.

### Framebuffer-specific

* Improved portability.
* Updated to handle new interface to core redraw functionality.
* Fixed to handle no configured Cookies file location.
* Updated for new frame and iframe support in core.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.7
-----------

### Core / All platforms

* Added WebP image support as build-time option.
* Made logging include timing information.
* Added treeview support.
* Added global history manager.
* Added hotlist manager.
* Added cookie manager.
* Added SSL certificate chain inspection display.
* Improved stability.
* Optimised plain text handling.
* Cleaned up build infrastructure.
* Fixed HTTP authentication issues.
* Improved cache cleanup.
* Improved detection of IP addresses in URLs.
* Fixed handling of IPv6 addresses.
* Updated rendering of local history.
* Made the cache more robust.
* Fixed building on OpenBSD.
* Optimised count of current fetches for given host.
* Added options for treeview rendering colours.
* Added partial support for CSS :after pseudo element.
* Fixed 'auto' top/bottom margins for tables.
* Improved font API documentation.
* Fixed float clearing bug.
* Fixed browser_window destruction issue.
* Added support for CSS system colours.
* Fixed colour treatment in rsvg binding.
* Improved portability.
* Fixed copying from plain text to clipboard.
* Improved core/front end interface for rendering into browser windows.
* Improved core/front end interface for rendering thumbnails.
* Optimised thumbnail rendering.
* Made rendering calls pass clipping rectangle around as pointer.
* Reduced floating point maths in the layout engine.
* Added support for about: URL scheme.
* Made cache more robust to strange server responses.
* Added about:config and about:Choices displays.
* Added about:licence and about:credits pages.
* Made knockout rendering optimisation independent of content types.
* Fixed clipping issue for HTML contents.
* Fixed overflow:auto and overflow:scroll behaviour.
* Set download filename according to Content-Disposition header.
* Added resource: URL scheme.
* Fixed poll loops for file: and data: URL scheme fetchers.
* Fixed cache control invalidation.
* Fixed text-indent layout issue.
* Fixed layout issue where clear wrongly interacted with margins.
* Improved cache performance.
* Fixed handling of objects which fail to load.
* Fixed various form submission issues.
* Parallelised fetch and conversion of imported stylesheets.
* Made content states more robust.
* Optimised layout code to reduce calls to measure strings.
* Improved layout code not to duplicate strings for text wrapping.
* Improved box structure for HTML contents.
* Optimised content message redraw requests.
* Made various cache enhancements.
* Text plot scaling handled in core.
* Handle API diversity of iconv() implementations.
* Optimise handling of child objects of an HTML content.
* Avoided stalling during early stages of fetch caused by cURL.
* Improved example of build configuration.
* Added generation of build testament for about:testament.
* Sanitised task scheduling.
* Improved debugging infrastructure.
* Fixed text/plain renderer to cope with scroll offsets.
* Added generated list of about: content at about:about.
* Allowed config. options to be set from the command line.

* Hubbub library 0.1.0 (HTML parser):
  - Added scoping for use from C++ programs.
  - Fixed example program.
  - Removed need for library initialisation and finalisation.
  - Generate entities tree at build time, rather than run time.
  - Added clang build support.

* LibCSS library 0.1.0 (CSS parser and selection engine):
  - Fixed destruction of bytecode for clip property.
  - Added scoping for use from C++ programs.
  - Removed need for library initialisation and finalisation.
  - Added support for CSS2 system colours.
  - Added support for CSS2 system fonts.
  - Altered external representation of colours to aarrggbb.
  - Added support for CSS3 rgba() colour specifier.
  - Added support for CSS3 'transparent' colour keyword.
  - Added support for CSS3 hsl() and hsla() colour specifiers.
  - Added support for CSS3 'currentColor' colour keyword.
  - Added support for CSS3 'opacity' property.
  - Added support for CSS3 selectors.
  - Added support for CSS3 namespaces.
  - Enabled clients to fetch imported stylesheets in parallel.
  - Made internal bytecode 64-bit safe.
  - Fixed leaking of strings.
  - Rewritten property parsers.
  - Certain property parsers auto-generated at build time.
  - Added clang build support.
  - Various portability enhancements.
  - Fixed selection for pseudo elements.
  - Added simultaneous selection for base and pseudo elements.
  - Namespaced all global symbols.
  - Updated test suite.
  - Future-proofed ABI.
  - Ensured fixed point maths saturates instead of overflowing.
  - Fixed clip property handling.
  - Fixed selection and cascade of "uncommon" CSS properties.
  - Added structure versioning for client input.

* LibNSBMP library 0.0.3 (NetSurf BMP decoder):
  - Added missing include.
  - Made more robust when handling broken ICO files.
  - Added clang build support.

* LibNSGIF library 0.0.3 (NetSurf GIF decoder):
  - Added missing include.
  - Added clang build support.

* LibParserUtils library 0.1.0 (parser building utility functions):
  - Fixed input stream encoding issue.
  - Added scoping for use from C++ programs.
  - Removed need for library initialisation and finalisation.
  - Removed need for run time provision of external Aliases file.
  - Added clang build support.
  - Namespaced all global symbols.
  - Handle API diversity of iconv() implementations.

* LibROSprite library (RISC OS Sprite support for non-RO platforms):
  - C89 compatibility.

* LibSVGTiny library (SVG support):
  - Improved parsing of stroke-width.
  - Added clang build support.
  - Various portability enhancements.

* LibWapcaplet library 0.1.1 (String internment):
  - Added scoping for use from C++ programs.
  - Removed need for library initialisation and finalisation.
  - Added clang build support.

### RISC OS-specific

* Replaced hotlist with core hotlist.
* Replaced global history with core global history.
* Replaced cookie manager with core cookie manager.
* Replaced SSL cert. inspection with core SSL cert. inspection.
* Apply weighted averaging to download rate display.
* Examine extension when fetching local file of type 'Data'.
* Iconv module version 0.11 required.
* Rewritten toolbar code.
* Created simplified, self-contained gui widgets.
* Obtain download filename from the core.
* Set CSS system colours from desktop palette.
* Added menu entries to load about:licence and about:credits pages.

### GTK-specific

* Replaced global history with core global history.
* Added bookmarks support, using core hotlist.
* Added cookie manager.
* Added SSL certificate inspection window.
* Support GTK >= 2.21.6.
* Improved full save implementation.
* Made drags less jerky.
* Made new tabs open to show homepage.
* Improved text wrap handling.
* Improved menu bar.
* Improved context sensitive popup menu.
* Made various thumbnailing fixes.
* Obtain download filename from the core.
* Updated to use resource: scheme for resources.
* Fixed makefie's installation target.
* Enabled tabbing between form inputs.
* Updated About NetSurf dialogue.
* Reduced overhead due to Pango when measuring text.

### AmigaOS-specific

* Improved bitmap caching.
* Fixed menus in kiosk mode.
* Improved filetype handling.
* Fixed menu shortcuts.
* Replaced hotlist with core hotlist.
* Replaced global history with core global history.
* Replaced cookie manager with core cookie manager.
* Replaced SSL cert. inspection with core SSL cert. inspection.
* Improved Cairo and non-Cairo plotters.
* Added auto-scroll on selection drags beyond window boundaries.
* Improved clipboard handling.
* Improved icon usage.
* Improved stability.
* Some incomplete work towards AmigaOS 3 support.
* Disabled iframes by default.
* Set CSS system colours from the pens in the screen DrawInfo table.
* Fixed kiosk mode to always fill screen.
* Improved scheduler.
* Made new tabs open to show homepage.
* Obtain download filename from the core.
* Added history content menus to back and forward buttons.
* Bitmap rendering optimisations.
* Improved download handling.
* Runtime selection of graphics plot implementations.
* Updated About requester.
* Enabled fast scrolling for all content types.

### Mac OS X-specific

* New front end.

### BeOS/Haiku-specific

* Fixed Replicant instantiation.
* Set CSS system colours according to current desktop settings.

### Windows-specific

* Improved sub-window creation.
* Fixed redraw bugs.
* Fixed bitmap plotting.
* Fixed thumbnailing.
* Fixed local history.
* Fixed URL bar.
* Cleaned up toolbar creation.
* Improved native build.
* Fixed CPU thrashing when idle.
* Use NetSurf icon on window decoration.
* Improved options dialogue.
* Made various 'look and feel' enhancements.

### Atari-specific

* New front end.

### Framebuffer-specific

* Improved toolbar.
* Improved font selection.
* Added glyph cache size configuration option.
* Made click action happen on mouse button release.
* Give browser widget input focus on startup.
* Fixed cursor leaving root widget.
* Dynamic detection of surface libraries.
* Updated to use resource: scheme for resources.
* Improved DPI handling.
* Fixed font size in text widgets.
* Added support for scaled rendering.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.6
-----------

### Core / All platforms

* Improvements to float positioning.
* Fix absolute positioned inlines.
* Improve handling of percentage margins and paddings on floats.
* Fix several memory leaks.
* Rationalised memory allocation functions.
* Updated SSL root certificates.
* Fix handling of PNGs with zero data.
* Moved input handling into content handlers.
* Simplified browser window module.
* Fix table borders specified in em/ex units.
* Improved CSS content handling.
* Don't try to draw zero size images.
* Simplified and fixed save complete feature.
* Reduce frequency of cache clean attempts.
* Improve and optimise rendering of borders in HTML.
* Fix URL comparison issue.
* Improve object handling in HTML.
* Fix absolute positioned root element.
* Rewritten file: fetch handler for local URLs.
* Much improved directory listings for local file: paths.
* Improve handling of binary data in plain text renderer.
* Fixed cache expiry problems.

* LibCSS library 0.0.2 (CSS parser and selection engine):
  - Allow stylesheets to be used in multiple contexts
     simultaneously.
  - Fix possible stylesheet reuse crash.

### RISC OS-specific

* Increased maximum allowed value for memory cache size option.
* Update 'about' page to include licences for linked components.
* Fix layout to paper width when printing.
* Fix screen redraw after printing.
* Enable sideways print option.

### GTK-specific

* Avoid potential use of NULL pointer.
* Fix for old versions of GTK.
* Improve internationalisation support.
* Improve handling of bitmap opacity.
* Fix redraw issues with non-HTML content.
* Cleaned up menu generation and handling.

### BeOS/Haiku-specific

* Fix build.
* Fix mimetype acquisition for local files.
* Improve handling of bitmap opacity.
* Enable knockout rendering optimisation.
* Improved polygon plotter accuracy.
* Fix colours on copy to clipboard.

### AmigaOS-specific

* Fixed menus when switching tabs.
* Improved font handling.
* Text kerning.
* Update for OS4.1 Update 2.
* Simplify building of Cairo and non-Cairo versions.
* Fixed scrollbar handling.
* Better NetSurf icon.
* Improve installer.
* Better support for ARexx commands.
* Context sensitive cut/copy/paste.
* Fix clipping for non-HTML contents.
* Option to show icon under drag saves.
* Bitmap plot optimisations.
* Now uses core fetcher for file: URLs.

### Framebuffer-specific

* Framebuffer toolkit rationalised.
* Improved scrollbar support.
* Added X surface handler.
* Improved scheduling.
* Fixed redraw issues.
* Improved keypress handling.
* On screen keyboard.
* History window.
* Configurable window furniture size.
* Support for scaled bitmap plotting.
* Factored out common plot code for rendering at different bpp.
* Load and save cookies file on start and quit.
* Improve handling of bitmap opacity.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.5
-----------

### Core / All platforms

* Fetch improvements.
* Fix fetching from https sites with http authentication.
* Handle http authentication with no realm.
* Cookie handling improvements.
* Improved support for {min|max}-{width|height} CSS properties.
* HTML redraw optimisations.
* Improved mouse tracking over browser windows.
* Browser window status bar update rationalisation.
* Better handling of memory exhaustion.
* Made text export more robust.
* Fix relative positioning of floats.
* Align baselines of text on a line.
* HTML layout optimisations.
* Fixed HTML based box alignment behaviour.
* Sorted out default table borders.
* Updated Italian translation.
* Updated German translation.
* Allow any name for frame targets.
* Allow sending of referer when moving from http hosts to https.
* Improved stability.
* Simplified plotter API.
* Fixed memory leak in imagemap handler.
* Optimised debug rendering of box model outlines.
* Activity is indicated by 'progress' pointer. 
* Ensure debug rendering outlines are plotted on top of other content.
* Now using LibCSS for CSS parsing and selection.
* Many CSS related bugs fixed.
* Rewritten cache and internal content handling.
* Fixes for file: URL handling.
* Don't try to access file: URLs through the configured proxy.
* Core widget for handling SELECT element menus.
* Core scrollbar widget used for CSS overflow and SELECT menu widget.
* Very basic quirks mode support.
* Cleaned up how the initial rendering of a content occurs.
* Command line argument handling and initialisation cleanup.
* Clearer build configuration messages.
* Favicon retrieval.
* Full page saves, including CSS and image files.
* PDF export disabled.
* Search-as-you-type page search support.

* Hubbub library 0.0.2 (HTML parser):
  - Fixed locale problem.
  - Various improvements.

* LibCSS library 0.0.1 (CSS parser and selection engine):
  - First release.

* LibNSBMP library 0.0.2 (NetSurf BMP decoder):
  - Fix ICO decoding on big-endian platforms.
  - Improve inverse height calculation.

* LibNSGIF library 0.0.2 (NetSurf GIF decoder):
  - Improve Palette entry handling.

* LibParserUtils library 0.0.2 (parser building utility functions):
  - Fixed charset problem.
  - Various improvements.

* LibSVGTiny library (SVG support):
  - Colour component ordering fix.
  - Support arcs in paths.
  - Improve circle element handling.
  - Add ellipse element support.
  - Improve stroke width rounding.

* LibWapcaplet library 0.0.2 (String internment):
  - First release.

### RISC OS-specific

* Theme code cleanup.
* Fix possible HTTP autentication crash.
* Fix potential problem with URL completion.
* Better handling of WIMP errors.
* Fixed memory leak in path plotting. (Used for SVG display.)
* Improve launching of URIs NetSurf doesn't handle.
* New guide to building NetSurf on RISC OS.
* Simplified native build process for NetSurf and libraries.
* Multitasking behaviour changed to be kinder to other apps.
* Fixed problem loading content served locally from WebJames.
* Improved stability.
* Save complete now implemented in the core.
* Find text moved to core and crash bug fixed.

* RUfl library (RISC OS Unicode support):
  - Fixed font scan crash.
  - Faster font scanning with FontManager 3.64 or later.

### GTK-specific

* Give drawing area input focus on click.
* Added local history toolbar button.
* Improve dot and dash line patterns.
* Made throbber loading more robust.
* Improved tab handling.
* Improved status bar control.
* Faster plotting of clipped and scaled images for smooth scrolling.
* Favicon display.
* Added view source code feature.
* Added search box on toolbar.
* Theme handling.
* Toolbar customisation.
* Context sensitive menu updated.
* Improved menu structure.
* Updated for compatibility with latest GTK versions.

### BeOS/Haiku-specific

* Complete save support.

### AmigaOS-specific

* Faster and more robust page plotting.
* Better clipping of plot actions.
* Smoother scrolling.
* Improved handling of tabs.
* Added UI for SSL certificate inspection.
* Improved stability.
* Better font handling.
* Added a preferences GUI.
* Better pointer handling.
* Better handling of bitmap alpha channels.
* Improved status bar.
* Added scale support.
* Improved mouse click handling.
* Keyboard shortcuts added.
* Added search bar.
* Added HelpHints.
* Theme and icon improvements.
* Handle more exotic formats dropped on text boxes, using DataTypes.
* Favicon display.
* Added search box on toolbar.

### Framebuffer-specific

* Framebuffer handling abstracted into libnsfb library.
* Fixed throbber to show inactivity correctly.
* Improved handling of text entry into URL bar.
* Fixed redraw issues while scrolled from top and while scrolling.
* Fixed redraw issues with animated GIFs and text areas.
* Improved mouse handling.
* Fixed zero size iframe crash.
* Added full range of pointer sprites.
* Better keyboard input handling.
* Fix continual reload on pages with meta refresh.
* Improve look and placement of toolbar, icons and scrollbars.
* Internal font changed.  Includes more glyphs and bold/italic.
* Improved stability.

* LibNSFB library (NetSurf Framebuffer):
  - New library.

### Windows-specific

* New front end, considered pre-alpha.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.1
-----------

### Core / All platforms

* Fix detection of active link areas (avoids entire page being treated
  as a link in certain cases).
* Support height on TR.
* Fix numbering of floated list items.
* Make borders/margins/padding on form elements match other browsers.
* Updated Italian translation.
* German welcome page.
* Fix handling of percentage height on HTML and BODY.
* Partially handle percentage heights on TABLE

### RISC OS-specific

* Set bit 4 of extended window flags byte.

### GTK-specific

* Fix buffer overflows in options code.

### BeOS/Haiku-specific

* Implement path plotter for SVG rendering.

### AmigaOS-specific

* Improved packaging and installation rules.
* Speed up scrollwheel scrolling.
* Cache scaled bitmaps in memory by default.
* Support exporting of images as IFF.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 2.0
-----------

### Core / All platforms

* Completely new HTML parser (Hubbub).
* SVG support. (Can be used as normal images and background images.)
* PDF export support.
* Many improvements to page layout and rendering.
* More CSS properties supported.
* Improved text selection behaviour.
* Charset fixes.
* Portability improvements.
* Configurable multi-platform build system.
* Tabbed browsing awareness.
* Corrected behaviour of backgrounds on root element.
* Many fixes related to floated content.
* More lenient meta refresh delay handling.
* Improvements to cut, copy and paste behaviour.
* Better cookie handling.
* Improved form functionality.
* Improved form control display.
* Stability fixes for handling nonsense data.
* Better handling of HTML attributes.
* Fix broken layout on systems which don't use '.' as the decimal
  separator.
* Fix inline height not affecting line height.
* Handle whitespace in URL bar.
* New throbber animation.
* Fixed occasional white flicker of page area during layout.
* New default homepage.

* Hubbub library 0.0.1 (HTML parser):
  - First release.

* LibParserUtils library 0.0.1 (parser building utility functions):
  - First release.

### RISC OS-specific

* Restructured menu with access to more functionality.
* Remembers save / download paths.
* Improved style-guide compliance.
* Wimpslot size adjusted at build time.
* Alt+click on close icon closes all NetSurf windows.
* Escape aborts drag save.
* New object > link submenu.
* SVG to Drawfile conversion.
* Zoom/scale with shift+scrollwheel.
* Improved drawfile export.

### GTK-specific

* Tabbed browsing.
* Global history.
* Download manager.
* Enhanced interface.
* Text selection.
* Print support.
* Improved context menu.
* Handle desktop DPI setting.
* Can support RISC OS Sprite bitmap format via librosprite.
* View source.
* Content language selection option.

### BeOS/Haiku-specific

* First release.

### AmigaOS-specific

* First release.

### Framebuffer-specific

* First release.

Also included are many smaller bug fixes, improvements and
documentation enhancements.


NetSurf 1.2
-----------

### Core / All platforms

* Overhaul documentation in line with new website.
* Many improvements to cookie handling.
* Minimum font size option is now obeyed by form elements.
* Add support for periodic reflow during fetching.
* Fixes for positioning of floated elements -- NetSurf now passes Acid1.
* Bring support for <center> and align= in line with specification.
* Updated SSL root certificate bundle.
* Various fixes for form element default styling and interaction with CSS.
* Rewritten, more tolerant, HTTP redirect handling.
* Fix crash when tabbing into hidden form fields.
* Fix background handling on <body> and <html>.
* Permit <style> in <body>.
* Relax restriction on location of @import rules.
* Improve CSS colour handling.
* Fix positioning of horizontal scrollbars within page.
* Fixes for inline-block boxes.
* Minor fixes to CSS parsing.
* NetBSD support.
* Fix GIF decoding to work correctly on big-endian processors.
* New build system and many compiler warnings fixed.

### RISC OS-specific

* Fix crashes on 'images.google.com'.
* Fix intermittent crashing when clicking in URL-completion menu.
* Fix search when input is '*'.
* Fix Shift+Adjust clicks.
* Permit disabling of interactive help.

### GTK-specific

* Improve form widget support.
* Improve support for core configuration options (memory cache size,
  proxy, etc).
* Fix unsightly flickering to white.
* Fix non-ASCII text input handling.
* Fix for caret positioning when clicking after the end of text in
  form inputs.

Also included are many and various smaller bug fixes, documentation
enhancements and updated translations.


NetSurf 1.1
-----------

### Core / All platforms

* Fix charset detection bugs, fixes #1726341 (WightLink).
* Add -v command line option to enable debugging output.
  (This is enabled by default in the RISC OS build via the !Run file).
* Rationalise header file includes.
* Various portability fixes to BSD and Solaris.
* Abstraction of schemes for fetchers.
* More encoding detection fixes.
* Fix cookie domain matching in unverifiable transactions.
* Fix cookie issues with 'launchpad.net'.
* Fix deadlock when fetching stylesheets.
* Fix bug where the referer header was being sent regardless.
* Fix bug where strange Accept-Language and other headers could be
  sent.
* Fix crash when building error page.
* Content scaling is now handled centrally, such that both GTK and
  RISC OS versions act in the same way.
* Licence for NetSurf changed to GPLv2 only with clause allowing
  linking to OpenSSL.

### RISC OS-specific

* Tweak default hotlist entries to be more up-to-date.
* Fix interaction between failed drag saves and pointer
  tracking (including page scrolling).
* Scroll wheel support on hotlist and history windows.
* Use proper background colour for AW files using transparency.
* Fix AWRenderer clipping issue (#1756230) and improve rendering
  performance.

### GTK-specific

* Add local file browsing MIME type guessing.  Also fixes
  directory browsing.
* Fix destruction of windows involving frames, fixes crashes
  on sites such as 'news.bbc.co.uk'.
* Reduce flicker when periodically reflowing during fetch.
* Fix bug that caused 100% CPU usage on some sites with frames.
* Fix scroll-position setting, allowing anchors and
  back/forward positions to be set/remembered.
* Fix problems with closing windows with fetches still in progress.
* New Window menu entry now functions.
* Open File... menu entry now functions.
* Fix reentrancy issues with event scheduling.
* Support for setting the path to find CA certificates (fixes SSL sites).
* Stop referer header being sent with URLs entered in the URL bar.
* Use default homepage if homepage option is blank.
* Full screen mode now functions.
* Popup version of the main menu.
* Removal of menubar/toolbar/statusbar now functions.
* Fix corruption on some animated GIFs due to new images not being
  created blank.
* Fix issues with scrolling from the keyboard.
* Add simple SSL certificate verification window.

Also included are many and various smaller bug fixes, documentation
enhancements and updated translations.


NetSurf 1.0
-----------

* First release.
