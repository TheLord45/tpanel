#!/bin/bash
#
# Set the following paths according to your installation.
#
export QTDIR="/opt/Qt/5.15.2/android"
export ANDROID_HOME="$HOME/Android/Sdk"
export ANDROID_NDK_HOST="linux-x86_64"
export ANDROID_NDK_PLATFORM="30"
export ANDROID_NDK_ROOT="${ANDROID_HOME}/ndk/23.2.8568313"
export ANDROID_SDK_ROOT="${ANDROID_HOME}"
export ANDROID_API_VERSION="android-30"
export JAVA_HOME="/usr/lib/jvm/java-14-openjdk-amd64"
export PATH="${QTDIR}/bin:$PATH"

KEYSTORE="$HOME/projects/tpanel/android/android_release.keystore"
KEYALIAS="tpanel"
LOGFILE="`pwd`/build.log"

_EXT_LIB_PATH="/home/andreas/Android/distribution"
_EXTRA_PATH="/home/andreas/Android/extras"
_SSL_PATH="/home/andreas/Android/openssl"
_ANDROID_ABIS="armeabi-v7a arm64-v8a x86 x86_64"

# This programs must be taken from the Qt framework and the Android SDK
QMAKE="/opt/Qt/5.15.2/android/bin/qmake"
MAKE="$HOME/Android/Sdk/ndk/23.2.8568313/prebuilt/linux-x86_64/bin/make"

#------------------------------------------------------------------------------
# DO NOT CHANGE ANYTHING BEYOND THIS LINE UNLESS YOU KNOW WHAT YOU'RE DOING!!
#------------------------------------------------------------------------------
#
# The following statement determines the number of CPUs. This is used for the
# make command to let run the compiler in parallel.
#
CPUS=`lscpu | egrep '^CPU\(s\)' | cut -c30-34`

function usage() {
    echo "build_android.sh [clean] [debug] [deploy] [list-avds]"
    echo "   clean     Delete old build, if there is one, and start a new clean build."
    echo "   debug     Create a binary with debugging enabled."
    echo "   deploy    Deploy the binary to an Android emulator."
    echo "   list-avds List all available AVDs."
    echo
    echo "Without parameters the source is compiled, if there were changes, and then"
    echo "an Android package is created."
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
    elif [ "$par" == "list-avds" ] || [ "$par" == "-list-avds" ]
    then
        ${ANDROID_SDK_ROOT}/tools/emulator -list-avds
	exit 0
    elif [ "$par" == "help" ] || [ "$par" == "--help" ] || [ "$par" == "-h" ]
    then
        usage
	exit 0
    fi
done

if [ $OPT_CLEAN -eq 1 ]
then
    log "Starting a clean build by deleting a maybe existing old build ..."
    rm -rf "tpanel-5.15-build"
fi

if [ ! -d "tpanel-5.15-build" ] || [ ! -f "tpanel-5.15-build/Makefile" ]
then
    log "Crating new build directory \"tpanel-5.15-build\" ..."

    if [ ! -d "tpanel-5.15-build" ]
    then
        mkdir "tpanel-5.15-build" > /dev/null 2>&1

        if [ $? -ne 0 ]
        then
            log "Error creating a directory!"
            exit 1
        fi
    fi

    log "Changing into build directory \"tpanel-5.15-build\" ..."
    cd "tpanel-5.15-build"
    log "Creating Makefile ..."

    _dbg=""

    if [ $OPT_DEBUG -eq 1 ]
    then
        _dbg="DEBUG QDEBUG"
    fi

    ${QMAKE} -spec android-clang CONFIG+=qtquickcompiler ${_dbg} SDK_PATH="${ANDROID_SDK_ROOT}" EXT_LIB_PATH="${_EXT_LIB_PATH}" EXTRA_PATH="${_EXTRA_PATH}" SSL_PATH="${_SSL_PATH}" ANDROID_ABIS="${_ANDROID_ABIS}" ../tpanel.pro 2>&1 | tee -a ${LOGFILE}

    if [ $? -ne 0 ]
    then
        log "Error creating a Makefile!"
        exit 1
    fi
else
    log "Changing into build directory \"tpanel-5.15-build\" ..."
    cd "tpanel-5.15-build"
fi

log "Compiling the source ..."
${MAKE} -j ${CPUS} apk_install_target 2>&1 | tee -a ${LOGFILE}

if [ $? -ne 0 ]
then
    log "Error compiling the code!"
    exit 1
fi

log "Creating an Android binary (apk) file ..."

_install=""
_sign=""
_pass=""

if [ $OPT_DEPLOY -eq 1 ]
then
    _install="--reinstall"
fi

if [ $OPT_DEBUG -eq 0 ]
then
    if [ -f "$HOME/.keypass" ]
    then
        _pass=`cat $HOME/.keypass`
    else
        read -s -p "Password of Android keystore: " _pass
    fi

    _sign="--release --sign"
    ${QTDIR}/bin/androiddeployqt --input android-tpanel-deployment-settings.json --output android-build --android-platform ${ANDROID_API_VERSION} --jdk ${JAVA_HOME} ${_sign} ${KEYSTORE} ${KEYALIAS} --storepass "${_pass}" ${_install} --gradle --verbose  2>&1 | tee -a ${LOGFILE}
else
    ${QTDIR}/bin/androiddeployqt --input android-tpanel-deployment-settings.json --output android-build --android-platform ${ANDROID_API_VERSION} --jdk ${JAVA_HOME} ${_install} --gradle --verbose  2>&1 | tee -a ${LOGFILE}
fi

if [ $? -ne 0 ]
then
    log "Error building an Android binary file!"
    exit 1
fi

exit 0
