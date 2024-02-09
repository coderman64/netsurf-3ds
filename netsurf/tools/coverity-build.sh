#!/bin/bash
#
# Copyright © 2013 Vincent Sanders <vince@netsurf-browser.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
#   * The above copyright notice and this permission notice shall be included in
#     all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# CI system coverity build and submission script
#
# Usage: coverity-build.sh
#

# environment variables
#
# HOST The ABI to be compiled for
# COVERITY_TOKEN
# COVERITY_USER
# COVERITY_PREFIX path to tools else default is used
#
# either PREFIX or JENKINS_HOME

COVERITY_PROJECT="NetSurf+Browser"

# build gtk2, framebuffer and monkey frontend by default
TARGETS="gtk2 framebuffer monkey"

# setup build environment
export PREFIX=${PREFIX:-${JENKINS_HOME}/artifacts-${HOST}}
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PREFIX}/lib
export PATH=${PATH}:${PREFIX}/bin

# Coverity tools location 
COVERITY_PREFIX=${COVERITY_PREFIX:-/opt/coverity/cov-analysis-linux64-7.5.0}
COVERITY_VERSION=$(git rev-parse HEAD)

export PATH=${PATH}:${COVERITY_PREFIX}/bin

COVERITY_TAR=coverity-scan.tar

# cleanup before we start
rm -rf cov-int/ ${COVERITY_TAR} ${COVERITY_TAR}.gz

for TARGET in ${TARGETS}; do
  make clean TARGET=${TARGET}
done

# Do the builds using coverity data gathering tool
for TARGET in ${TARGETS}; do
    cov-build --dir cov-int make CCACHE= TARGET=${TARGET}
done

tar cf ${COVERITY_TAR} cov-int

gzip -9 ${COVERITY_TAR}

curl --form "project=${COVERITY_PROJECT}" --form "token=${COVERITY_TOKEN}" --form "email=${COVERITY_USER}" --form "file=@${COVERITY_TAR}.gz" --form "version=${COVERITY_VERSION}" --form "description=Git Head build" "https://scan.coverity.com/builds?project=${COVERITY_PROJECT}"
