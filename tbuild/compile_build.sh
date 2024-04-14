#!/bin/bash
# errors are propagated when using the pipe to concatenate commands
set -o pipefail
# determine script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROOT_DIR="${SCRIPT_DIR}/.."
LOG_FILE="${ROOT_DIR}/workflow.log"
VERSION="1.4.2"
RELEASE="1"
export QT_DIR="/opt/Qt/6.6.2/gcc_64"
export CMAKE_PREFIX_PATH="${QT_DIR}/lib/cmake"

# the first parameter is the branch, that is currently being built
BRANCH="$1"

# Log the message.
# parameters:
#    message ... the message to log
log() {
    local message="$1"
    local currentTime=$(date +"%y-%m-%d %H:%M:%S")
    echo "${currentTime}: ${message}" | tee -a ${LOG_FILE}
}

if [ -f "${ROOT_DIR}/.version" ]
then
    . ${ROOT_DIR}/.version
fi

CPUS=2
type nproc > /dev/null 2>&1

if [ $? -eq 0 ]
then
    CPUS=`nproc`
fi

rm -rf "${ROOT_DIR}/build" > /dev/null 2>&1
mkdir "${ROOT_DIR}/build"

log "Changing to directory $ROOT_DIR"
cd ${ROOT_DIR}
log "Creating the Makefile"
cmake -B build -S . >> ${LOG_FILE} 2>&1

if [ $? -ne 0 ]
then
    log "ERROR: Could not create Makefile!"
    exit 1
fi

log "Changing to build directory"
cd build
log "Building the application ..."
make -j${CPUS} >> ${LOG_FILE} 2>&1

if [ $? -ne 0 ]
then
    echo "Error compiling the code!"
    exit 1
fi

# Build a debian package
log "Building a debian package"
BASE_PATH="${ROOT_DIR}/tpanel_${VERSION}-${RELEASE}_amd64"
rm -rf "${BASE_PATH}"
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
echo "Version: $VERSION" >> "${BASE_PATH}/DEBIAN/control"
echo "Section: misc" >> "${BASE_PATH}/DEBIAN/control"
echo "Priority: optional" >> "${BASE_PATH}/DEBIAN/control"
echo "Architecture: amd64" >> "${BASE_PATH}/DEBIAN/control"
echo "Depends: Qt6MultimediaWidgets,ssl,expat,uuid,Qt6Multimedia,Qt6Network,Qt6Widgets,Qt6Gui,Qt6Core,crypto,freetype,jpeg,png16,Qt6DBus,asound,FLAC,vorbis,ogg,mpg123" >> "${BASE_PATH}/DEBIAN/control"
echo "Summary: Emulation of AMX touch panels" >> "${BASE_PATH}/DEBIAN/control"
echo "License: GPLv3" >> "${BASE_PATH}/DEBIAN/control"
echo "Homepage: https://https://github.com/TheLord45/tpanel" >> "${BASE_PATH}/DEBIAN/control"
echo "Maintainer: Andreas Theofilu <andreas@theosys.at>" >> "${BASE_PATH}/DEBIAN/control"
echo "Description: TPanel is an emulation of some AMX G4/G5 touch panels." >> "${BASE_PATH}/DEBIAN/control"
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
exit 0
