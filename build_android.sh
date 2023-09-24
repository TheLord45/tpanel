#!/bin/bash
#
# Set the following paths according to your installation.
#
QT_VERSION="6.5.2"
QT_VERSION_MAJOR=6
QT_PATH="/opt/Qt"
QT_ABI="x86_64"

QTBASE="${QT_PATH}/$QT_VERSION"
QTDIR="${QTBASE}/android_${QT_ABI}"

ANDROID_HOME="$HOME/Android/Sdk"
ANDROID_NDK_PLATFORM="30"
ANDROID_NDK_ROOT="${ANDROID_HOME}/ndk/25.2.9519653"
ANDROID_SDK_ROOT="${ANDROID_HOME}"
ANDROID_PLATFORM="android-$ANDROID_NDK_PLATFORM"
QTMACROS="${QT_PATH}/Tools/QtCreator/share/qtcreator/package-manager"
JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"

export PATH="${QT}/Tools/Ninja:${QT}/Tools/CMake/bin:${QTDIR}/bin:$PATH"
ANDROID_TOOLCHAIN="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"

KEYSTORE="$HOME/projects/tpanel/android/android_release.keystore"
KEYALIAS="tpanel"
PASSFILE="$HOME/.keypass"
LOGFILE="`pwd`/build.log"
BUILDDIR="tpanel-6-build"

export EXT_LIB_PATH="$HOME/Android/distribution"
export EXTRA_PATH="$HOME/Android/extras"
export SSL_PATH="$HOME/Android/openssl"
#export ANDROID_ABIS="arm64-v8a armeabi-v7a x86_64 x86"

# This programs must be taken from the Qt framework and the Android SDK
QMAKE="$QTDIR/bin/qmake"
CMAKE="${QT_PATH}/Tools/CMake/bin/cmake"
GREP="/usr/bin/grep"
SED="/usr/bin/sed"
ANDROIDDEPLOYQT="$QTBASE/gcc_64/bin/androiddeployqt"

#------------------------------------------------------------------------------
# DO NOT CHANGE ANYTHING BEYOND THIS LINE UNLESS YOU KNOW WHAT YOU'RE DOING!!
#------------------------------------------------------------------------------

# Getting the current directory
CURDIR=`pwd`

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

function usage() {
    echo "build_android.sh [clean] [debug] [deploy] [sign] [list-avds] [help|--help|-h]"
    echo "   clean     Delete old build, if there is one, and start a new clean build."
    echo "   debug     Create a binary with debugging enabled."
    echo "   deploy    Deploy the binary to an Android emulator."
    echo "   sign      Sign the resulting APK file. This requires a file named"
    echo "             \"\$HOME/.keypass\" containing the password and has read"
    echo "             permissions for the owner only (0400 or 0600). If this file"
    echo "             is missing, the script asks for the password."
    echo "   verbose | --verbose | -v"
    echo "             Prints the commands used to create the target file."
    echo "   list-avds List all available AVDs and exit."
    echo
    echo "   help | --help | -h   Displays this help screen and exit."
    echo
    echo "Without parameters the source is compiled, if there were changes, and then"
    echo "an Android package is created. This package will be an unsigned release APK."
}

function log() {
    echo "$@"
    echo "$@" >> ${LOGFILE}
}

> ${LOGFILE}

# Test command line for parameters and set options
OPT_CLEAN=0
OPT_DEBUG=0
OPT_DEPLOY=0
OPT_SIGN=0
OPT_VERBOSE=0

for par in "$@"
do
    if [ "$par" == "clean" ]
    then
        OPT_CLEAN=1
    elif [ "$par" == "debug" ]
    then
        OPT_DEBUG=1
    elif [ "$par" == "deploy" ]
    then
        OPT_DEPLOY=1
    elif [ "$par" == "sign" ]
    then
        OPT_SIGN=1
    elif [ "$par" == "list-avds" ] || [ "$par" == "-list-avds" ]
    then
        ${ANDROID_SDK_ROOT}/tools/emulator -list-avds
        exit 0
    elif [ "$par" == "--verbose" ] || [ "$par" == "-v" ] || [ "$par" == "verbose" ]
    then
        OPT_VERBOSE=1
    elif [ "$par" == "help" ] || [ "$par" == "--help" ] || [ "$par" == "-h" ]
    then
        usage
        exit 0
    fi
done

if [ $OPT_DEBUG -eq 1 ] && [ $OPT_SIGN -eq 1 ]
then
    log "You can't sign a debugging version!"
    exit 1
fi

if [ $OPT_CLEAN -eq 1 ]
then
    log "Starting a clean build by deleting a maybe existing old build ..."
    rm -rf "$BUILDDIR"
fi

if [ ! -d "$BUILDDIR" ] || [ ! -f "$BUILDDIR/build.ninja" ]
then
    if [ ! -d "$BUILDDIR" ]
    then
        log "Crating new build directory \"$BUILDDIR\" ..."
        mkdir "${BUILDDIR}" > /dev/null 2>&1
        mkdir -p "${BUILDDIR}/.qtc/package-manager" > /dev/null 2>&1

        if [ $? -ne 0 ]
        then
            log "Error creating a directory!"
            exit 1
        fi

        log "Copiing cmake macros from $QTMACROS ..."
        cp ${QTMACROS}/* ${BUILDDIR}/.qtc/package-manager > /dev/null 2>&1

        if [ $? -ne 0 ]
        then
            log "Error copiing cmake macros!"
            exit 1
        fi
    fi

    log "Changing into build directory \"$BUILDDIR\" ..."
    cd "$BUILDDIR"
    log "Creating Makefile ..."

    _dbg=""

    if [ $OPT_DEBUG -eq 1 ]
    then
        _dbg="-DCMAKE_BUILD_TYPE:STRING=Debug"
    else
        _dbg="-DCMAKE_BUILD_TYPE:STRING=Release"
    fi

    if [ $OPT_VERBOSE -eq 1 ]
    then
        echo "${CMAKE} -S ${CURDIR} -B ${CURDIR}/${BUILDDIR} -DCMAKE_GENERATOR:STRING=Ninja ${_dbg} -DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH=${CURDIR}/${BUILDDIR}/.qtc/package-manager/auto-setup.cmake -DQT_QMAKE_EXECUTABLE:FILEPATH=${QMAKE} -DCMAKE_PREFIX_PATH:PATH=${QTDIR} -DCMAKE_C_COMPILER:FILEPATH=${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ -DANDROID_PLATFORM:STRING=${ANDROID_PLATFORM} -DANDROID_NDK:PATH=${ANDROID_NDK_ROOT} -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${ANDROID_TOOLCHAIN} -DANDROID_USE_LEGACY_TOOLCHAIN_FILE:BOOL=OFF -DANDROID_ABI:STRING=${QT_ABI} -DANDROID_STL:STRING=c++_shared -DCMAKE_FIND_ROOT_PATH:PATH=${QTDIR} -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON -DQT_HOST_PATH:PATH=${QTBASE}/gcc_64 -DANDROID_SDK_ROOT:PATH=${ANDROID_HOME} -DQT_ANDROID_BUILD_ALL_ABIS:BOOL=ON"
    fi

    ${CMAKE} -S ${CURDIR} -B ${CURDIR}/${BUILDDIR} \
        -DCMAKE_GENERATOR:STRING=Ninja ${_dbg} \
        -DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH=${CURDIR}/${BUILDDIR}/.qtc/package-manager/auto-setup.cmake \
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QMAKE} \
        -DCMAKE_PREFIX_PATH:PATH=${QTDIR} \
        -DCMAKE_C_COMPILER:FILEPATH=${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang \
        -DCMAKE_CXX_COMPILER:FILEPATH=${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
        -DANDROID_PLATFORM:STRING=${ANDROID_PLATFORM} \
        -DANDROID_NDK:PATH=${ANDROID_NDK_ROOT} \
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${ANDROID_TOOLCHAIN} \
        -DANDROID_USE_LEGACY_TOOLCHAIN_FILE:BOOL=OFF \
        -DANDROID_ABI:STRING=${QT_ABI} \
        -DANDROID_STL:STRING=c++_shared \
        -DCMAKE_FIND_ROOT_PATH:PATH=${QTDIR} \
        -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON \
        -DQT_HOST_PATH:PATH=${QTBASE}/gcc_64 \
        -DANDROID_SDK_ROOT:PATH=${ANDROID_HOME} \
        -DQT_ANDROID_BUILD_ALL_ABIS:BOOL=ON \
        -DQT_ANDROID_ABIS:STRING="arm64-v8a;armeabi-v7a;x86;x86_64" \
        2>&1 | tee -a ${LOGFILE}

    if [ $? -ne 0 ]
    then
        log "Error creating a Makefile!"
        exit 1
    fi
else
    log "Changing into build directory \"$BUILDDIR\" ..."
    cd "$BUILDDIR"
fi

log "Compiling the source and using ${CPUS} cpus..."

if [ $OPT_VERBOSE -eq 1 ]
then
    echo "${CMAKE} --build "${CURDIR}/${BUILDDIR}" --target all -j${CPUS}"
fi

${CMAKE} --build "${CURDIR}/${BUILDDIR}" --target all -j${CPUS} 2>&1 | tee -a ${LOGFILE}

if [ $? -ne 0 ]
then
    log "Error compiling the code!"
    exit 1
fi

log "Creating an Android binary (apk) file ..."

_install=""
_pass=""

if [ $OPT_DEPLOY -eq 1 ]
then
    _install="--reinstall"
fi

if [ $OPT_DEBUG -eq 0 ] && [ $OPT_SIGN -eq 1 ]
then
    if [ -f "$HOME/.keypass" ]
    then
        _perm=`stat -c %a "$HOME/.keypass"`

        if [ $_perm -ne 400 ] && [ $_perm -ne 600 ]
        then
            echo "The file \"$HOME/.keypass\" must have permission 0400 or 0600. Ignoring file!"
            read -s -p "Password of Android keystore: " _pass
        else
            _pass=`cat $HOME/.keypass`
        fi
    else
        read -s -p "Password of Android keystore: " _pass
    fi

    if [ $OPT_VERBOSE -eq 1 ]
    then
        echo "${ANDROIDDEPLOYQT} --input ${CURDIR}/${BUILDDIR}/android-tpanel-deployment-settings.json --output ${CURDIR}/${BUILDDIR}/android-build --android-platform ${ANDROID_PLATFORM} --jdk ${JAVA_HOME} --gradle --release --verbose --sign ${KEYSTORE} ${KEYALIAS} --storepass "${_pass}" ${_install}"
    fi

    ${ANDROIDDEPLOYQT} --input ${CURDIR}/${BUILDDIR}/android-tpanel-deployment-settings.json \
        --output ${CURDIR}/${BUILDDIR}/android-build \
        --android-platform ${ANDROID_PLATFORM} \
        --jdk ${JAVA_HOME} \
        --gradle --release \
        --sign ${KEYSTORE} ${KEYALIAS} \
        --storepass "${_pass}" ${_install} 2>&1 | tee -a ${LOGFILE}
else
    if [ $OPT_VERBOSE -eq 1 ]
    then
        echo "${ANDROIDDEPLOYQT} --input ${CURDIR}/${BUILDDIR}/android-tpanel-deployment-settings.json --output ${CURDIR}/${BUILDDIR}/android-build --android-platform ${ANDROID_PLATFORM} --jdk ${JAVA_HOME} --gradle --verbose ${_install}"
    fi

    ${ANDROIDDEPLOYQT} --input ${CURDIR}/${BUILDDIR}/android-tpanel-deployment-settings.json \
        --output ${CURDIR}/${BUILDDIR}/android-build \
        --android-platform ${ANDROID_PLATFORM} \
        --jdk ${JAVA_HOME} \
        --gradle --verbose  ${_install} 2>&1 | tee -a ${LOGFILE}
fi

if [ $? -ne 0 ]
then
    log "Error building an Android binary file!"
    exit 1
fi

exit 0
