#!/bin/bash
#
# NetSurf continuous integration build script for jenkins
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

# This script may be executed by jenkins jobs that use the core buildsystem
#
# Usage: jenkins-build.sh [install|test-install|coverage|static|docs]
#
# install - build and install
# test-install - build, test and install
# coverage - run coverage
# static - perform a static analysis
# coverity - perform a coverity scan
# docs - build doxygen docs
# sanitize - Run the test suite under ASAN and UBSAN

# HOST must be in the environment and set correctly
if [ "x${HOST}" = "x" ];then
    echo "HOST unset"
    exit 1
fi

# The target for built artifacts
#
# This requires the artifacts of a target must all be built on the same
#  jenkins slave instance.
ARTIFACT_HOME=${JENKINS_HOME}/artifacts-${HOST}

# Obtain the native build triplet if unset
if [ "x${BUILD}" = "x" ];then
    # This assumes the cc on the PATH is the native one.
    BUILD=$(cc -dumpmachine)
fi


# target defaults
TARGET_TEST=
TARGET_INSTALL=
TARGET_COVERAGE=
TARGET_STATIC=
TARGET_COVERITY=
TARGET_DOCS=

# Which variant is being generated
VARIANT="release"

# change target build according to parameter
case "$1" in
    "install")
	TARGET_INSTALL=${HOST}
	;;

    "coverage")
	TARGET_COVERAGE=${HOST}
	VARIANT="debug"
	# need to disable ccache on coverage builds
	export CCACHE=
	;;

    "static")
	TARGET_STATIC=${HOST}
	VARIANT="debug"
	# need to disable ccache on static builds
	export CCACHE=
	;;

    "coverity")
	TARGET_COVERITY=${HOST}
	VARIANT="debug"
	# need to disable ccache on coverity builds
	export CCACHE=
	;;

    "test-install")
	# Perfom test if being executed on native target
	TARGET_TEST=${BUILD}
	TARGET_INSTALL=${HOST}
	;;

    "docs")
	TARGET_DOCS=${HOST}
	;;

    "sanitize")
	TARGET_SANITIZE=${HOST}
	VARIANT="debug"
	;;

    "")
	# default is test only on Linux and install
	# Currently most tests do not work on targets except for Linux
	TARGET_TEST="x86_64-linux-gnu"
	TARGET_INSTALL=${HOST}
	;;

    *)
	cat <<EOF
Usage: jenkins-build.sh [install|test-install|coverage|static|docs|sanitize]

       install        build and install
       test-install   build, test and install
       coverage       run coverage
       static         perform a static anaysis
       coverity       perform a coverity scan
       docs           generate doxygen docs
       sanitize       run the tests under ASAN/UBSAN
EOF
	exit 1
	;;
esac


# adjust tools based on build
case ${BUILD} in

    amd64-unknown-openbsd*)
	MAKE=gmake
	;;

    x86_64-unknown-openbsd*)
	MAKE=gmake
	;;

    x86_64-unknown-freebsd*)
	MAKE=gmake
	;;    

    x86_64-apple-darwin14.5.0*)
        PATH="/opt/local/bin:/opt/local/sbin:$PATH"
	MAKE=make
        ;;

    *)
	MAKE=make
	;;
esac


# Ensure the artifact target directory exists
mkdir -p ${ARTIFACT_HOME}


# Configure all build paths relative to prefix
export PREFIX=${ARTIFACT_HOME}
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PREFIX}/lib
export PATH=${PATH}:${PREFIX}/bin


# execute the build steps

# clean target is always first
${MAKE} Q= clean HOST=${HOST} VARIANT=${VARIANT}


# build as per type requested
if [ "x${HOST}" = "x${TARGET_COVERAGE}" ]; then
    # Coverage Build
    CFLAGS=-Wno-error ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT} coverage
    gcovr -v -x -e 'test/.*' -r . -o coverage.xml

elif [ "x${HOST}" = "x${TARGET_STATIC}" ]; then
    # static build
    rm -rf clangScanBuildReports

    # build with static analysis without warnings causing errors
    CFLAGS=-Wno-error scan-build -o clangScanBuildReports -v -v --use-cc clang --use-analyzer=/usr/bin/clang ${MAKE} Q= VARIANT=${VARIANT}

    # clean up after
    ${MAKE} Q= clean HOST=${HOST} VARIANT=${VARIANT}


elif [ "x${HOST}" = "x${TARGET_COVERITY}" ]; then
    # coverity build

    # Check thses are set
    #
    # COVERITY_PROJECT
    # COVERITY_TOKEN
    # COVERITY_USER
    # COVERITY_PREFIX

    if [ -z "${COVERITY_PROJECT}" -o -z "${COVERITY_TOKEN}" -o -z "${COVERITY_USER}" -o -z "${COVERITY_PREFIX}" ]; then
	echo "Coverity parameters not set"
	exit 1
    fi

    # Coverity tools location
    COVERITY_PREFIX=${COVERITY_PREFIX:-/opt/coverity/cov-analysis-linux64-6.6.1}
    COVERITY_VERSION=$(git rev-parse HEAD)

    export PATH=${PATH}:${COVERITY_PREFIX}/bin

    # cleanup before we start
    rm -rf cov-int/ coverity-scan.tar.gz coverity-scan.tar

    cov-build --dir cov-int ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT}

    tar cf coverity-scan.tar cov-int

    gzip -9 coverity-scan.tar

    curl --form "project=${COVERITY_PROJECT}" --form "token=${COVERITY_TOKEN}" --form "email=${COVERITY_USER}" --form "file=@coverity-scan.tar.gz" --form "version=${COVERITY_VERSION}" --form "description=Git Head build" "https://scan.coverity.com/builds?project=${COVERITY_PROJECT}"


elif [ "x${HOST}" = "x${TARGET_DOCS}" ]; then
    # Documentation build
    ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT} docs

elif [ "x${HOST}" = "x${TARGET_SANITIZE}" ]; then
    # Sanitize build
    env UBSAN_OPTIONS=print_stacktrace=1 ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT} sanitize

else
    # Normal build
    ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT}


fi


# run tests if appropriate
if [ "x${HOST}" = "x${TARGET_TEST}" ]; then
    ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT} test
fi


# install the output 
if [ "x${HOST}" = "x${TARGET_INSTALL}" ]; then
    ${MAKE} Q= HOST=${HOST} VARIANT=${VARIANT} install
fi
