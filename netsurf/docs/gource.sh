#!/bin/bash
#
# This script generates a gource visualisation with some parameters.
#
# you need a recent gource and ffmpeg install for this to work

# Settings

TITLE="Netsurf"

# length and quality
TYPE="std" # sml, std, lrg

#Camera mode
CMODE=overview # overview, track

# standard monitor (suitable for video projector playback)
#OUTPUT_SIZE="1024x768"

# HD widescreen 720p
OUTPUT_SIZE="1280x720"

# HD widescreen 1080p
#OUTPUT_SIZE="1280x1080"

TMP_DIR=/home/video/gource/

######################################################################

#quality parameters
case ${TYPE} in
    "std")
    # standard overview
    QPARAM="-s 0.25 -i 600 -a 1"
    ;;
  
    "lrg")
    # large overview
    QPARAM="-s 0.5 -i 200 -a 5"
    ;;

    "sml")
    # rapid overview (3mins)
    QPARAM="-s 0.04 -i 30 -a 1"
    ;;

    *)
    # bad type
    echo "bad type"
    exit 1
    ;;

esac

echo "Generating ${TITLE}-gource-${TYPE}-${OUTPUT_SIZE}-${CMODE}"

TMP_PPM=${TMP_DIR}/${TITLE}-gource-${TYPE}-${OUTPUT_SIZE}-${CMODE}.ppm

# output filename
FILENAME=${TITLE}-gource-${TYPE}-${OUTPUT_SIZE}-${CMODE}.mp4

# filter some directories which are not interesting
FILEFILTER="\!NetSurf/|riscos/distribution/|gtk/res/|framebuffer/res/|amiga/resources/|beos/res/|cocoa/res/|windows/res/|atari/res|riscos/appdir/"

# generate
gource --title "NetSurf Development" -${OUTPUT_SIZE} ${QPARAM} --max-files 10000 --bloom-multiplier 0.10 --bloom-intensity 0.5 --title ${TITLE} --highlight-all-users --output-framerate 25 --hide filenames --stop-at-end --date-format "%d %B %Y" --bloom-intensity 0.2 --file-filter "${FILEFILTER}" --key --camera-mode ${CMODE} --output-ppm-stream - > ${TMP_PPM}

#convert the ppm to movie
ffmpeg -y -r 25 -f image2pipe -vcodec ppm -i ${TMP_PPM} -vcodec libx264 -b:v 2000k ${FILENAME}


