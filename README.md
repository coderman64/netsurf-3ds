# Netsurf 3DS

Homebrew 3DS port of the lightweight [NetSurf web browser](https://www.netsurf-browser.org/).

## What it can do:
- browse to http-only websites
- search on Google
- browse the SD card and open local text, html, and png files (go to file:///)
- most web 1.0 stuff

## Known issues
- SSL implementation is currently broken (i.e. no HTTPS pages).
- The screen only seems to refresh when you interact with it.
- No proper system to download things to SD card.
- some things (e.g. larger images) distort while scrolling.
- Only one very chonky font.
- JavaScript support is disabled in the build process
- Support for several media types is disabled. (SVG, WebP)
- Occasionally crashes (should be mostly fixed)

## What it will probably never do:
- Play HTML5/WebGL games
- Play videos/YouTube
- load complex Web 2.0 applications like Google Docs or Twitter
