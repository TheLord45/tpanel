#!/bin/bash
# Change the version of Qt to the one you intend to use.
QTVERSION="6.9.1"
# Set the path to your Qt installation
QTPATH="/opt/Qt/$QTVERSION"
# Set the path to your homebrew installation
HOMEBREW="/opt/homebrew"
# Set the directory where the source should be build
BUILDDIR="build"
LOGFILE="$(pwd)/buildmac.log"

#
# From here on you should have no need to change anything
#
function log() {
    echo "$@"
    echo "$@" >> ${LOGFILE}
}

function usage() {
    echo "build_mac.sh [clean] [help|--help|-h]"
    echo "   clean     Delete old build, if there is one, and start a new clean build."
    echo 
    echo "   help | --help | -h   Displays this help screen and exit."
    echo
    echo "Without parameters the source is compiled, if there were changes."
}

export QT_PATH="$QTPATH/macos/lib/cmake/Qt6"
OPT_CLEAN=0

for par in "$@"
do
    if [ "$par" == "clean" ]
    then
        OPT_CLEAN=1
    elif [ "$par" == "help" ] || [ "$par" == "--help" ] || [ "$par" == "-h" ]
    then
        usage
        exit 0
    fi
done

if [ $OPT_CLEAN -eq 1 ]
then
    log "Starting a clean build by deleting a maybe existing old build ..."
    rm -rf "$BUILDDIR"
fi

cmake -B build -DCMAKE_PREFIX_PATH="$QTPATH/macos/lib/cmake:$HOMEBREW/lib/cmake"

if [ $? -ne 0 ]
then
    exit 1
fi

cd build
exec make -j8
