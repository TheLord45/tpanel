#!/bin/bash
set -o pipefail

###########################################################################
# Adapt the below variables to your need                                  #
###########################################################################
QT_VERSION="6.6.1"
QT_VERSION_MAJOR=6
QT_PATH="$HOME/Qt"
QT_ARCHITECTURE="arm64"
QT_MACROS="${QT_PATH}/Qt?Creator.app/Contents/Resources/package-manager"

QTBASE="${QT_PATH}/$QT_VERSION"
QTDIR="${QTBASE}/ios"

IOS_VERSION="17.0"
BUILDPATH="tpanel-ios"
OSX_SYSROOT="iphoneos"
#OSX_SYSROOT="iphonesimulator"
SIGNING_IDENTITY="<YOUR_SIGNING_IDENTITY>"

SRCDIR="`pwd`"
LOGFILE="${SRCDIR}/build.log"
EXT_LIB_PATH="${SRCDIR}/SDKs"

###########################################################################
# DO NOT EDIT ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT YOU'RE DOING! #
###########################################################################

if [ -z "$OSTYPE" ]
then
    echo "Unknown operating system! This script must run on Mac OSX!"
    exit 1
fi

echo "$OSTYPE" | grep darwin > /dev/null

if [ $? -ne 0 ]
then
    echo "Unsupported OS $OSTYPE!"
    echo "This script must run on a Mac!"
    exit 1
fi

export EXT_LIB_PATH
GENERATOR="Xcode"
PROJECT_INCLUDE_BEFORE="${SRCDIR}/${BUILDPATH}/.qtc/package-manager/auto-setup.cmake"
QMAKE="${QTDIR}/bin/qmake"
PREFIX="${QTDIR}"
CC="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang"
CXX="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"
TOOLCHAIN="${QTDIR}/lib/cmake/Qt6/qt.toolchain.cmake"

function usage() {
     echo "build_ios.sh [clean] [debug] [sign] [id <ID>] [help|--help|-h]"
     echo "   clean     Delete old build, if there is one, and start a new clean build."
     echo "   debug     Create a binary with debugging enabled."
     echo "   sign      Sign the resulting app."
     echo "   id <ID>   The signing identity (team ID)"
     echo
     echo "   help | --help | -h   Displays this help screen and exit."
     echo
     echo "Without parameters the source is compiled, if there were changes, and then"
     echo "an iOS package is created."
}

function log() {
     echo "$@"
     echo "$@" >> ${LOGFILE}
}

> ${LOGFILE}

if [ "$OSX_SYSROOT" != "iphoneos" -a "$OSX_SYSROOT" != "iphonesimulator" ]
then
    log "Invalid target: $OSX_SYSROOT!"
    log "OSX_SYSROOT must be \"iphoneos\" or \"iphonesimulator\"."
    exit 1
fi

#
# The following statement determines the number of CPUs. This is used for the
# cmake command to let run the compiler in parallel.
#
CPUS=2
type nproc > /dev/null 2>&1

if [ $? -eq 0 ]
then
     CPUS=`nproc`
fi

OPT_CLEAN=0
OPT_DEBUG=0
OPT_SIGN=0
OPT_VERBOSE=0
_loop=0

for par in "$@"
do
    if [ $_loop -eq 1 ]
    then
        SIGNING_IDENTITY="$par"
        _loop=0
        continue
    fi

    if [ "$par" == "clean" ]
    then
        OPT_CLEAN=1
    elif [ "$par" == "debug" ]
    then
        OPT_DEBUG=1
    elif [ "$par" == "sign" ]
    then
        OPT_SIGN=1
    elif [ "$par" == "id" ]
    then
        _loop=1
        continue
    elif [ "$par" == "--verbose" ] || [ "$par" == "-v" ] || [ "$par" == "verbose" ]
    then
        OPT_VERBOSE=1
    elif [ "$par" == "help" ] || [ "$par" == "--help" ] || [ "$par" == "-h" ]
    then
        usage
        exit 0
    fi
done

if [ -z $TERM_PROGRAM ] || [ "$TERM_PROGRAM" != "Apple_Terminal" ]
then
    if [ $OPT_SIGN -eq 1 ]
    then
        log "WARNING: This script is not running directly in an Apple console on a Mac!"
        log "         This may cause signing to fail!"
        log
    fi
fi

if [ "${SIGNING_IDENTITY}" == "<YOUR_SIGNING_IDENTITY>" ]
then
    log "Missing the signing identity (team ID)!"
    log "Please enter it in the head of this script or set it with parameter \"id\"!"
    log
    security find-identity -vp codesigning 2>&1 | tee -a ${LOGFILE}

    if [ $? -ne 0 ]
    then
        log "Couldn't find any signing identities!"
        exit 1
    fi

    read -p "Enter the signing identity (team ID) [CTRL+C to stop script]: " SIGNING_IDENTITY

    if [ -z $SIGNING_IDENTITY ]
    then
        log "No signing identity!"
        exit 1
    fi

    security find-identity -vp codesigning | grep "${SIGNING_IDENTITY}" > /dev/null 2>&1

    if [ $? -ne 0 ]
    then
        log "The signing identity \"$SIGNING_IDENTITY\" was not found!"
        exit 1
    fi
fi

if [ $OPT_CLEAN -eq 1 ]
then
    if [ -d "${BUILDPATH}" ]
    then
        log "Deleting the build path at $BUILDPATH ..."
        rm -rf "${BUILDPATH}"
    fi
fi

if [ ! -d "${BUILDPATH}" ]
then
    log "Creating directory $BUILDPATH ..."
    mkdir -p "${BUILDPATH}/.qtc/package-manager"
    log "Copy macros from $QT_MACROS ..."
    cp ${QT_MACROS}/* "${BUILDPATH}/.qtc/package-manager"
fi

_extra=""

if [ $OPT_SIGN -eq 1 -a "$OSX_SYSROOT" == "iphoneos" ]
then
    _extra="-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM:STRING=${SIGNING_IDENTITY}"
elif [ "${OSX_SYSROOT}" == "iphonesimulator" ]
then
    log "The build for iPhone simulator is not signable!"
fi

_config="Release"

if [ $OPT_DEBUG -eq 1 ]
then
    _extra="$_extra -DCMAKE_VERBOSE_MAKEFILE:STRING=1"
    _config="Debug"
fi

log "Creating build files in $BUILDPATH ..."

if [ $OPT_DEBUG -eq 1 ]
then
    log "cmake -S "${SRCDIR}" -B \"${BUILDPATH}\" -DAPPLE:STRING=1 -DIOS:STRING=1 -DCMAKE_GENERATOR:STRING=\"$GENERATOR\" -DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH=\"${PROJECT_INCLUDE_BEFORE}\" -DQT_QMAKE_EXECUTABLE:FILEPATH=\"${QMAKE}\" -DCMAKE_PREFIX_PATH:PATH=\"${PREFIX}\" -DCMAKE_C_COMPILER:FILEPATH=\"${CC}\" -DCMAKE_CXX_COMPILER:FILEPATH=\"${CXX}\" -DCMAKE_TOOLCHAIN_FILE:FILEPATH=\"${TOOLCHAIN}\" -DCMAKE_OSX_ARCHITECTURES:STRING=\"$QT_ARCHITECTURE\" -DCMAKE_OSX_SYSROOT:STRING=\"${OSX_SYSROOT}\" -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=\"${IOS_VERSION}\" ${_extra}"
fi

cmake -S "${SRCDIR}" \
      -B "${BUILDPATH}" \
      -DAPPLE:STRING=1 \
      -DIOS:STRING=1 \
      -DCMAKE_GENERATOR:STRING="$GENERATOR" \
      -DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH="${PROJECT_INCLUDE_BEFORE}" \
      -DQT_QMAKE_EXECUTABLE:FILEPATH="${QMAKE}" \
      -DCMAKE_PREFIX_PATH:PATH="${PREFIX}" \
      -DCMAKE_C_COMPILER:FILEPATH="${CC}" \
      -DCMAKE_CXX_COMPILER:FILEPATH="${CXX}" \
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH="${TOOLCHAIN}" \
      -DCMAKE_OSX_ARCHITECTURES:STRING="$QT_ARCHITECTURE" \
      -DCMAKE_OSX_SYSROOT:STRING="${OSX_SYSROOT}" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING="${IOS_VERSION}" \
      ${_extra} 2>&1 | tee -a ${LOGFILE}

if [ $? -ne 0 ]
then
    log "Error configuring the build pipeline!"
    log "For details look at \"${LOGFILE}\"."
    exit 1
fi

log "Compiling the code ..."

if [ $OPT_DEBUG -eq 1 ]
then
    log "cmake --build \"${BUILDPATH}\" --target ALL_BUILD --config ${_config} -j$CPUS -- -allowProvisioningUpdates"
fi

cmake --build "${BUILDPATH}" --target ALL_BUILD --config ${_config} -j$CPUS -- -allowProvisioningUpdates -strictVerify=false 2>&1 | tee -a ${LOGFILE}

if [ $? -ne 0 ]
then
    log "Error compiling!"
    log "For details look at \"${LOGFILE}\"."
    exit 1
fi

log "** Success **"
log "You can find the details at \"${LOGFILE}\""
exit 0

