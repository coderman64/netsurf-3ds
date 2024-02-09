# This shell fragment is intended for use in `bash` or `zsh`.  While it
# may work in other shells it is not meant to, and any misbehaviour is not
# considered a bug in that case.
#
# NetSurf Library, tool and browser development support script
#
# Copyright 2013-2017 Vincent Sanders <vince@netsurf-browser.org>
# Released under the MIT Licence
#
# This script allows NetSurf and its libraries to be built without
#   requiring installation into a system.
#
# Usage: source env.sh
#
# Controlling variables
#   HOST sets the target architecture for library builds
#   BUILD sets the building machines architecture
#   TARGET_WORKSPACE is the workspace directory to keep the sandboxes
#
# The use of HOST and BUILD here is directly comprable to the GCC
#   usage as described at:
#     http://gcc.gnu.org/onlinedocs/gccint/Configure-Terms.html
#

###############################################################################
# OS Package installation
###############################################################################

# deb packages for dpkg based systems
NS_DEV_DEB="build-essential pkg-config git gperf libcurl3-dev libpng-dev libjpeg-dev"
NS_TOOL_DEB="flex bison libhtml-parser-perl"
if [ "x${NETSURF_GTK_MAJOR}" = "x3" ]; then
    NS_GTK_DEB="libgtk-3-dev librsvg2-dev"
else
    NS_GTK_DEB="libgtk2.0-dev librsvg2-dev"
fi

# apt get commandline to install necessary dev packages
ns-apt-get-install()
{
    LIBCURL_OPENSSL_CONFLICTS="$(/usr/bin/apt-cache show libcurl4-openssl-dev | grep Conflicts | grep -o libssl1.0-dev)"
    if [ "x${LIBCURL_OPENSSL_CONFLICTS}" != "x" ]; then
        NS_DEV_DEB="${NS_DEV_DEB} libssl-dev"
    elif /usr/bin/apt-cache show libssl1.0-dev >/dev/null 2>&1; then
        NS_DEV_DEB="${NS_DEV_DEB} libssl1.0-dev"
    else
        NS_DEV_DEB="${NS_DEV_DEB} libssl-dev"
    fi
    sudo apt-get install $(echo ${NS_DEV_DEB} ${NS_TOOL_DEB} ${NS_GTK_DEB})
}


# packages for yum installer RPM based systems (tested on fedora 20)
NS_DEV_YUM_RPM="git gcc pkgconfig expat-devel openssl-devel gperf libcurl-devel perl-Digest-MD5-File libjpeg-devel libpng-devel"
NS_TOOL_YUM_RPM="flex bison"
if [ "x${NETSURF_GTK_MAJOR}" = "x3" ]; then
    NS_GTK_YUM_RPM="gtk3-devel librsvg2-devel"
else
    NS_GTK_YUM_RPM="gtk2-devel librsvg2-devel"
fi

# yum commandline to install necessary dev packages
ns-yum-install()
{
    sudo yum -y install $(echo ${NS_DEV_YUM_RPM} ${NS_TOOL_YUM_RPM} ${NS_GTK_YUM_RPM})
}


# packages for dnf installer RPM based systems (tested on fedora 25)
NS_DEV_DNF_RPM="java-1.8.0-openjdk-headless gcc clang pkgconfig libcurl-devel libjpeg-devel expat-devel libpng-devel openssl-devel gperf perl-HTML-Parser"
NS_TOOL_DNF_RPM="git flex bison ccache screen"
if [ "x${NETSURF_GTK_MAJOR}" = "x3" ]; then
    NS_GTK_DNF_RPM="gtk3-devel"
else
    NS_GTK_DNF_RPM="gtk2-devel"
fi

# dnf commandline to install necessary dev packages
ns-dnf-install()
{
    sudo dnf install $(echo ${NS_DEV_DNF_RPM} ${NS_TOOL_DNF_RPM} ${NS_GTK_DNF_RPM})
}


# packages for zypper installer RPM based systems (tested on openSUSE leap 42)
NS_DEV_ZYP_RPM="java-1_8_0-openjdk-headless gcc clang pkgconfig libcurl-devel libjpeg-devel libexpat-devel libpng-devel openssl-devel gperf perl-HTML-Parser"
NS_TOOL_ZYP_RPM="git flex bison gperf ccache screen"
if [ "x${NETSURF_GTK_MAJOR}" = "x3" ]; then
    NS_GTK_ZYP_RPM="gtk3-devel"
else
    NS_GTK_ZYP_RPM="gtk2-devel"
fi

# zypper commandline to install necessary dev packages
ns-zypper-install()
{
    sudo zypper install -y $(echo ${NS_DEV_ZYP_RPM} ${NS_TOOL_ZYP_RPM} ${NS_GTK_ZYP_RPM})
}


# Packages for Haiku install

# Haiku secondary arch suffix:
# empty for primary (gcc2 on x86) or "_x86" for gcc4 secondary.
HA=_x86

NS_DEV_HPKG="devel:libcurl${HA} devel:libpng${HA} devel:libjpeg${HA} devel:libcrypto${HA} devel:libiconv${HA} devel:libexpat${HA} cmd:pkg_config${HA} cmd:gperf html_parser"

# pkgman commandline to install necessary dev packages
ns-pkgman-install()
{
    pkgman install $(echo ${NS_DEV_HPKG})
}


# MAC OS X
NS_DEV_MACPORT="git expat openssl curl libjpeg-turbo libpng"

ns-macport-install()
{
    PATH=/opt/local/bin:/opt/local/sbin:$PATH sudo /opt/local/bin/port install $(echo ${NS_DEV_MACPORT})
}


# packages for FreeBSD install
NS_DEV_FREEBSDPKG="gmake curl"

# FreeBSD package install
ns-freebsdpkg-install()
{
    pkg install $(echo ${NS_DEV_FREEBSDPKG})
}


# generic for help text
NS_DEV_GEN="git, gcc, pkgconfig, expat library, openssl library, libcurl, perl, perl MD5 digest, libjpeg library, libpng library"
NS_TOOL_GEN="flex tool, bison tool"
if [ "x${NETSURF_GTK_MAJOR}" = "x3" ]; then
    NS_GTK_GEN="gtk+ 3 toolkit library, librsvg2 library"
else
    NS_GTK_GEN="gtk+ 2 toolkit library, librsvg2 library"
fi

# Generic OS package install
#  looks for package managers and tries to use them if present
ns-package-install()
{
    if [ -x "/usr/bin/zypper" ]; then
        ns-zypper-install
    elif [ -x "/usr/bin/apt-get" ]; then
        ns-apt-get-install
    elif [ -x "/usr/bin/dnf" ]; then
        ns-dnf-install
    elif [ -x "/usr/bin/yum" ]; then
        ns-yum-install
    elif [ -x "/bin/pkgman" ]; then
        ns-pkgman-install
    elif [ -x "/opt/local/bin/port" ]; then
        ns-macport-install
    elif [ -x "/usr/sbin/pkg" ]; then
        ns-freebsdpkg-install
    else
        echo "Unable to determine OS packaging system in use."
        echo "Please ensure development packages are installed for:"
        echo ${NS_DEV_GEN}"," ${NS_TOOL_GEN}"," ${NS_GTK_GEN}
    fi
}

###############################################################################
# Setup environment
###############################################################################

# find which command used to find everything else on path
if [ -x /usr/bin/which ]; then
    WHICH_CMD=/usr/bin/which
else
    WHICH_CMD=/bin/which
fi

# environment parameters

# The system doing the building
if [ "x${BUILD}" = "x" ]; then
    BUILD_CC=$(${WHICH_CMD} cc)
    if [ $? -eq 0 ];then
        BUILD=$(cc -dumpmachine)
    else
       echo "Unable to locate a compiler. Perhaps run ns-package-install"
       return 1
    fi
fi

# Get the host build if unset
if [ "x${HOST}" = "x" ]; then
    if [ "x${TARGET_ABI}" = "x" ]; then
        HOST=${BUILD}
    else
        HOST=${TARGET_ABI}
    fi
else
    HOST_CC_LIST="/opt/netsurf/${HOST}/cross/bin/${HOST}-cc /opt/netsurf/${HOST}/cross/bin/${HOST}-gcc ${HOST}-cc ${HOST}-gcc"
    for HOST_CC_V in $(echo ${HOST_CC_LIST});do
        HOST_CC=$(${WHICH_CMD} ${HOST_CC_V})
        if [ "x${HOST_CC}" != "x" ];then
            break
        fi
    done
    if [ "x${HOST_CC}" = "x" ];then
        echo "Unable to execute host compiler for HOST=${HOST}. is it set correctly?"
        return 1
    fi

    HOST_CC_MACHINE=$(${HOST_CC} -dumpmachine 2>/dev/null)

    if [ "${HOST_CC_MACHINE}" != "${HOST}" ];then
        echo "Compiler dumpmachine differs from HOST setting"
        return 2
    fi

    NS_ENV_CC="${HOST_CC}"
    export NS_ENV_CC

    unset HOST_CC_LIST HOST_CC_V HOST_CC HOST_CC_MACHINE
fi

# set up a default target workspace
if [ "x${TARGET_WORKSPACE}" = "x" ]; then
    TARGET_WORKSPACE=${HOME}/dev-netsurf/workspace
fi

# set up default parallelism
if [ "x${USE_CPUS}" = "x" ]; then
    NCPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null)
    NCPUS="${NCPUS:-1}"
    NCPUS=$((NCPUS * 2))
    USE_CPUS="-j${NCPUS}"
fi

# The GTK version to build for (either 2 or 3 currently)
if [ "x${NETSURF_GTK_MAJOR}" = "x" ]; then
    NETSURF_GTK_MAJOR=2
fi

# report to user
echo "BUILD=${BUILD}"
echo "HOST=${HOST}"
echo "TARGET_WORKSPACE=${TARGET_WORKSPACE}"
echo "USE_CPUS=${USE_CPUS}"

export PREFIX=${TARGET_WORKSPACE}/inst-${HOST}
export BUILD_PREFIX=${TARGET_WORKSPACE}/inst-${BUILD}
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH}::
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PREFIX}/lib
export PATH=${PATH}:${BUILD_PREFIX}/bin
export NETSURF_GTK_MAJOR

# make tool
MAKE=make

# NetSurf GIT repositories
NS_GIT="git://git.netsurf-browser.org"

# Buildsystem: everything depends on this
NS_BUILDSYSTEM="buildsystem"

NS_TOOLS=""
NS_FRONTEND_LIBS=""

BUILD_TARGET="${TARGET:-netsurf}"

case "$BUILD_TARGET" in
    libhubbub)
        NS_INTERNAL_LIBS="libparserutils"
        ;;

    libdom)
        NS_INTERNAL_LIBS="libwapcaplet libparserutils libhubbub"
        ;;

    libcss)
        NS_INTERNAL_LIBS="libwapcaplet libparserutils"
        ;;

    netsurf)
        # internal libraries all frontends require (order is important)
        NS_INTERNAL_LIBS="libwapcaplet libparserutils libhubbub libdom libcss libnsgif libnsbmp libutf8proc libnsutils libnspsl libnslog"

        # add target specific libraries
        case "${HOST}" in
            i586-pc-haiku)
                # tools required to build the browser for haiku (beos)
                NS_TOOLS="nsgenbind"
                # libraries required for the haiku target abi
                NS_FRONTEND_LIBS="libsvgtiny"
                ;;
            *arwin*)
                # tools required to build the browser for OS X
                NS_TOOLS=""
                # libraries required for the Darwin target abi
                NS_FRONTEND_LIBS="libsvgtiny libnsfb"
                ;;
            arm-unknown-riscos|arm-riscos-gnueabi*)
                # tools required to build the browser for RISC OS
                NS_TOOLS="nsgenbind"
                # libraries required for the risc os target abi
                NS_FRONTEND_LIBS="libsvgtiny librufl libpencil librosprite"
                ;;
            *-atari-mint)
                # tools required to build the browser for atari
                NS_TOOLS=""
                # libraries required for the atari frontend
                NS_FRONTEND_LIBS=""
                ;;
            ppc-amigaos)
                # default tools required to build the browser
                NS_TOOLS="nsgenbind"
                # default additional internal libraries
                NS_FRONTEND_LIBS="libsvgtiny"
                ;;
            m68k-unknown-amigaos)
                # default tools required to build the browser
                NS_TOOLS="nsgenbind"
                # default additional internal libraries
                NS_FRONTEND_LIBS="libsvgtiny"
                ;;
            *-unknown-freebsd*)
                # tools required to build the browser for freebsd
                NS_TOOLS=""
                # libraries required for the freebsd frontend
                NS_FRONTEND_LIBS=""
                # select gnu make
                MAKE=gmake
                ;;
            *)
                # default tools required to build the browser
                NS_TOOLS="nsgenbind"
                # default additional internal libraries
                NS_FRONTEND_LIBS="libsvgtiny libnsfb"
                ;;
        esac
        ;;
esac

export MAKE

################ Development helpers ################

# git pull in all repos parameters are passed to git pull
ns-pull()
{
    for REPO in $(echo ${NS_BUILDSYSTEM} ${NS_INTERNAL_LIBS} ${NS_FRONTEND_LIBS} ${NS_TOOLS} ${BUILD_TARGET}) ; do
        echo -n "     GIT: Pulling ${REPO}: "
        if [ -f "${TARGET_WORKSPACE}/${REPO}/.git/config" ]; then
            (cd ${TARGET_WORKSPACE}/${REPO} && git pull $*; )
        else
            echo "Repository not present"
        fi
    done
}

# clone all repositories
ns-clone()
{
    SHALLOW=""
    SKIP=""
    while [ $# -gt 0 ]
    do
        case "$1" in
            -d | --deps-only) SKIP="${BUILD_TARGET}"
                                shift
                                ;;
            -s | --shallow) SHALLOW="--depth 1"
                            shift
                            ;;
            -*) echo "Error: Unknown option: $1" >&2
                exit 1
                ;;
            *) # No more options
               break
               ;;
        esac
    done

    mkdir -p ${TARGET_WORKSPACE}
    for REPO in $(echo ${NS_BUILDSYSTEM} ${NS_INTERNAL_LIBS} ${NS_FRONTEND_LIBS} ${NS_RISCOS_LIBS} ${NS_TOOLS} ${BUILD_TARGET}) ; do
        [ "x${REPO}" != "x${SKIP}" ] || continue
        echo -n "     GIT: Cloning ${REPO}: "
        if [ -f ${TARGET_WORKSPACE}/${REPO}/.git/config ]; then
            echo "Repository already present"
        else
            (cd ${TARGET_WORKSPACE} && git clone ${SHALLOW} ${NS_GIT}/${REPO}.git; )
        fi
    done

    # put current env.sh in place in workspace
    if [ "x$NS_BROWSER" = "x" ]; then
        if [ ! -f "${TARGET_WORKSPACE}/env.sh" -a -f ${TARGET_WORKSPACE}/${NS_BROWSER}/docs/env.sh ]; then
            cp ${TARGET_WORKSPACE}/${NS_BROWSER}/docs/env.sh ${TARGET_WORKSPACE}/env.sh
        fi
    fi
}

# issues a make command to all libraries
ns-make-libs()
{
    for REPO in $(echo ${NS_BUILDSYSTEM} ${NS_INTERNAL_LIBS} ${NS_FRONTEND_LIBS}); do
        echo "    MAKE: make -C ${REPO} $USE_CPUS $*"
        ${MAKE} -C ${TARGET_WORKSPACE}/${REPO} HOST=${HOST} $USE_CPUS $*
        if [ $? -ne 0 ]; then
            return $?
        fi
    done
}

# issues make command for all tools
ns-make-tools()
{
    for REPO in $(echo ${NS_BUILDSYSTEM} ${NS_TOOLS}); do
        echo "    MAKE: make -C ${REPO} $USE_CPUS $*"
        ${MAKE} -C ${TARGET_WORKSPACE}/${REPO} PREFIX=${BUILD_PREFIX} HOST=${BUILD} $USE_CPUS $*
        if [ $? -ne 0 ]; then
            return $?
        fi
    done
}

# issues a make command for framebuffer libraries
ns-make-libnsfb()
{
    echo "    MAKE: make -C libnsfb $USE_CPUS $*"
    ${MAKE} -C ${TARGET_WORKSPACE}/libnsfb HOST=${HOST} $USE_CPUS $*
}

# pulls all repos and makes and installs the libraries and tools
ns-pull-install()
{
    ns-pull $*

    ns-make-tools install
    ns-make-libs install
}

# Passes appropriate flags to make
ns-make()
{
    ${MAKE} $USE_CPUS "$@"
}
