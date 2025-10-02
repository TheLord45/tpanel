#!/bin/bash
# errors are propagated when using the pipe to concatenate commands
set -o pipefail
# determine script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROOT_DIR="${SCRIPT_DIR}/.."
VERSION_DIR="${ROOT_DIR}/../../../.."
LOG_FILE="${ROOT_DIR}/workflow.log"
VERSION="2.0.0"
RELEASE="1"

# the first parameter is the branch, that is currently being built
BRANCH="main"

# Log the message.
# parameters:
#    message ... the message to log
log()
{
    local message="$1"
    local currentTime=$(date +"%y-%m-%d %H:%M:%S")
    echo "${currentTime}: ${message}" | tee -a ${LOG_FILE}
}

makeVersion()
{
    if [ ! -f "${VERSION_DIR}/.version" ]
    then
        echo -n "$VERSION" > "${VERSION_DIR}/.version"
        echo -n "$RELEASE" > "${VERSION_DIR}/.release"
        log "Using initial release $VERSION-$RELEASE"
    else
        VERSION=$(cat "${VERSION_DIR}/.version")
        RELEASE=$(cat "${VERSION_DIR}/.release")
        RELEASE=$((RELEASE + 1))
        echo -n "$RELEASE" > "${VERSION_DIR}/.release"
        log "Using release $VERSION with build $RELEASE."
    fi
}

log "Checking/Compiling: $GITHUB_REF"

if [ ! -z "$GITHUB_REF_NAME" ]
then
    BRANCH="$GITHUB_REF_NAME"
fi

CPUS=2
type nproc > /dev/null 2>&1

if [ $? -eq 0 ]
then
    CPUS=`nproc`
fi

log "Changing to directory $ROOT_DIR"
cd ${ROOT_DIR}

if [ "$BRANCH" == "main" ]
then
    # Build a debian package
    log "Building a debian package"
    makeVersion
    BASE_PATH="${ROOT_DIR}/tpanel_$VERSION-$RELEASE"
    rm -rf "${ROOT_DIR}/tpanel_"*

    mkdir -p "${BASE_PATH}/DEBIAN" >> ${LOG_FILE} 2>&1
    mkdir -p "${BASE_PATH}/usr/bin" >> ${LOG_FILE} 2>&1
    mkdir -p "${BASE_PATH}/usr/lib" >> ${LOG_FILE} 2>&1
    (cd "${ROOT_DIR}/build/ftplib";tar -cf - libftp*) | (cd "${BASE_PATH}/usr/lib";tar -xf -)
    cp "${ROOT_DIR}/build/src/tpanel" "${BASE_PATH}/usr/bin"
    cp "/usr/local/lib/skia/libskia.so" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjsua.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjsip-simple.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjsip.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjmedia.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpj.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjsip-ua.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjmedia-audiodev.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjmedia-codec.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjnath.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libpjlib-util.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libsrtp.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libresample.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libspeex.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libwebrtc.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libgsmcodec.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libilbccodec.so.2" "${BASE_PATH}/usr/lib"
    cp "/usr/local/lib/libsndfile.so.1" "${BASE_PATH}/usr/lib"

    echo "Package: tpanel" > "${BASE_PATH}/DEBIAN/control"
    echo "Version: $VERSION-$RELEASE" >> "${BASE_PATH}/DEBIAN/control"
    echo "Section: misc" >> "${BASE_PATH}/DEBIAN/control"
    echo "Priority: optional" >> "${BASE_PATH}/DEBIAN/control"
    echo "Architecture: amd64" >> "${BASE_PATH}/DEBIAN/control"
    echo "Depends: Qt6MultimediaWidgets,ssl,expat,uuid,Qt6Multimedia,Qt6Network,Qt6Widgets,Qt6Gui,Qt6Core,crypto,freetype,jpeg,png16,Qt6DBus,asound,FLAC,vorbis,ogg,mpg123" >> "${BASE_PATH}/DEBIAN/control"
    echo "Summary: Implementation of AMX touch panels" >> "${BASE_PATH}/DEBIAN/control"
    echo "License: GPLv3" >> "${BASE_PATH}/DEBIAN/control"
    echo "Homepage: https://github.com/TheLord45/tpanel" >> "${BASE_PATH}/DEBIAN/control"
    echo "Maintainer: Andreas Theofilu <andreas@theosys.at>" >> "${BASE_PATH}/DEBIAN/control"
    echo "Description: TPanel is an implementation of some AMX G4/G5 touch panels." >> "${BASE_PATH}/DEBIAN/control"
    echo "  The software uses internally the Skia library for drawing all objects" >> "${BASE_PATH}/DEBIAN/control"
    echo "  and the Qt 6.x library to display the objects. TPanel is written in C++." >> "${BASE_PATH}/DEBIAN/control"
    echo "  This makes it even on mobile platforms fast and reliable." >> "${BASE_PATH}/DEBIAN/control"

    dpkg --build "${BASE_PATH}" >> ${LOG_FILE} 2>&1

    if [ $? -ne 0 ]
    then
        log "Error building a debian package!"
        exit 1
    fi

    log "Package tpanel_${VERSION}-${RELEASE}_amd64.deb was build successfuly"
else
    log "Don't building a package for branch $BRANCH"
fi

exit 0
