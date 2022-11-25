# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

# Define the following to either 5 or 6. It defines for which Qt major
# version the code should be compiled and linked.
# As of writing this lines, it was not possible to build a running
# Android version with Qt6. Up to version Qt 6.4 it's possible to compile
# an Android version for a particular processor type, but the resulting
# binary doesn't run. But it is up to you to try with any newer version.
#

# ------------------------------------------------------------------------
#       DO NOT EDIT ANYTHING BELOW THIS LINE UNTIL YOU KNOW WHAT
#                            YOU ARE DOING!!
# ------------------------------------------------------------------------

TARGET = tpanel

equals(OS,osx) {
versionAtMost(QT_VERSION, 5.15.2) {
QT = core core-private gui gui-private widgets multimedia multimediawidgets sensors qml quickwidgets
} else {
QT = core core-private gui widgets qml quickwidget
}}
equals(OS,ios) {
versionAtMost(QT_VERSION, 5.15.2) {
QT = core core-private gui gui-private widgets multimedia multimediawidgets sensors qml quickwidgets
} else {
QT = core core-private gui widgets multimedia multimediawidgets sensors qml quickwidgets
}}
isEmpty(OS) {
versionAtMost(QT_VERSION, 5.15.2) {
QT = core core-private gui gui-private widgets multimedia multimediawidgets sensors qml quickwidgets androidextras
} else {
QT = core core-private gui widgets multimedia multimediawidgets sensors qml quickwidgets
}}

# The main application
HEADERS = \
   $$PWD/base64.h \
   $$PWD/config.h \
   $$PWD/tamxcommands.h \
   $$PWD/tmap.h \
   $$PWD/tamxnet.h \
   $$PWD/tsocket.h \
   $$PWD/tbutton.h \
   $$PWD/tcolor.h \
   $$PWD/tconfig.h \
   $$PWD/tdirectory.h \
   $$PWD/terror.h \
   $$PWD/texpand.h \
   $$PWD/tfont.h \
   $$PWD/thttpclient.h \
   $$PWD/ticons.h \
   $$PWD/timagerefresh.h \
   $$PWD/tnameformat.h \
   $$PWD/tobject.h \
   $$PWD/tpage.h \
   $$PWD/tpagelist.h \
   $$PWD/tpagemanager.h \
   $$PWD/tpageinterface.h \
   $$PWD/tpalette.h \
   $$PWD/tprjresources.h \
   $$PWD/tqtmain.h \
   $$PWD/tqtsettings.h \
   $$PWD/tresources.h \
   $$PWD/tsettings.h \
   $$PWD/tsubpage.h \
   $$PWD/tdrawimage.h \
   $$PWD/ttimer.h \
   $$PWD/tvalidatefile.h \
   $$PWD/ttpinit.h \
   $$PWD/tfsfreader.h \
   $$PWD/expand.h \
   $$PWD/readtp4.h \
   $$PWD/texternal.h \
   $$PWD/tqemitqueue.h \
   $$PWD/tqkeyboard.h \
   $$PWD/tqkeypad.h \
   $$PWD/tqtphone.h \
   $$PWD/tqdownload.h \
   $$PWD/texcept.h \
   $$PWD/tsipclient.h \
   $$PWD/timgcache.h \
   $$PWD/tsystemsound.h \
   $$PWD/tsystemdraw.h \
   $$PWD/tqeditline.h \
   $$PWD/tsystem.h \
   $$PWD/texpat++.h \
   $$PWD/tvector.h \
   $$PWD/turl.h \
   $$PWD/ftplib/ftplib.h

SOURCES = \
   $$PWD/base64.cpp \
   $$PWD/tamxcommands.cpp \
   $$PWD/tmap.cpp \
   $$PWD/tamxnet.cpp \
   $$PWD/tsocket.cpp \
   $$PWD/tbutton.cpp \
   $$PWD/tcolor.cpp \
   $$PWD/tconfig.cpp \
   $$PWD/tdirectory.cpp \
   $$PWD/terror.cpp \
   $$PWD/texpand.cpp \
   $$PWD/tfont.cpp \
   $$PWD/thttpclient.cpp \
   $$PWD/ticons.cpp \
   $$PWD/timagerefresh.cpp \
   $$PWD/tmain.cpp \
   $$PWD/tnameformat.cpp \
   $$PWD/tobject.cpp \
   $$PWD/tpage.cpp \
   $$PWD/tpagelist.cpp \
   $$PWD/tpagemanager.cpp \
   $$PWD/tpageinterface.cpp \
   $$PWD/tpalette.cpp \
   $$PWD/tprjresources.cpp \
   $$PWD/tqtmain.cpp \
   $$PWD/tqtsettings.cpp \
   $$PWD/tresources.cpp \
   $$PWD/tsettings.cpp \
   $$PWD/tsubpage.cpp \
   $$PWD/tdrawimage.cpp \
   $$PWD/ttimer.cpp \
   $$PWD/tvalidatefile.cpp \
   $$PWD/ttpinit.cpp \
   $$PWD/tfsfreader.cpp \
   $$PWD/expand.cpp \
   $$PWD/readtp4.cpp \
   $$PWD/texternal.cpp \
   $$PWD/tqemitqueue.cpp \
   $$PWD/tqkeyboard.cpp \
   $$PWD/tqkeypad.cpp \
   $$PWD/tqtphone.cpp \
   $$PWD/tqdownload.cpp \
   $$PWD/tsipclient.cpp \
   $$PWD/texcept.cpp \
   $$PWD/timgcache.cpp \
   $$PWD/tsystemsound.cpp \
   $$PWD/tsystemdraw.cpp \
   $$PWD/tqeditline.cpp \
   $$PWD/tsystem.cpp \
   $$PWD/texpat++.cpp \
   $$PWD/turl.cpp \
   $$PWD/ftplib/ftplib.cpp

isEmpty(OS) {
OTHER_FILES += \
        $$PWD/android/src/org/qtproject/theosys/BatteryState.java \
        $$PWD/android/src/org/qtproject/theosys/NetworkStatus.java \
        $$PWD/android/src/org/qtproject/theosys/PhoneCallState.java \
        $$PWD/android/src/org/qtproject/theosys/UriToPath.java \
        $$PWD/android/src/org/qtproject/theosys/Logger.java \
        $$PWD/android/src/org/qtproject/theosys/Orientation.java

INCLUDEPATH = \
    $$PWD/. \
    $$PWD/ftplib \
    $$EXT_LIB_PATH/skia \
    $$EXT_LIB_PATH/pjsip/include \
    $$EXTRA_PATH/expat/include
}
equals(OS,osx) {
    INCLUDEPATH += $$PWD/. \
                   $$PWD/ftplib \
                   /opt/homebrew/include \
                   /opt/homebrew/opt/openssl@1.1/include \
                   /usr/local/include \
                   /usr/local/include/skia

    CONFIG += c++17
#    CONFIG += hide_symbols
}
equals(OS,ios) {
    INCLUDEPATH += $$PWD/. \
                   $$PWD/ftplib \
                   /usr/local/include/skia \
                   $$EXT_LIB_PATH/pjsip/include \
                   $$EXT_LIB_PATH/openssl/include

    CONFIG += c++17
    QMAKE_IOS_DEPLOYMENT_TARGET=13.0
}

QMAKE_CXXFLAGS += -std=c++17 -DPJ_AUTOCONF
QMAKE_LFLAGS += -std=c++17

android: include($$SDK_PATH/android_openssl/openssl.pri)

equals(ANDROID_TARGET_ARCH,arm64-v8a) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/arm64-v8a/include

    LIBS += $$EXT_LIB_PATH/skia/arm64/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/arm64 \
    $$EXT_LIB_PATH/pjsip/lib/libpj-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjlib-util-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-audiodev-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-codec-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjnath-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-simple-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-ua-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsua-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libresample-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libspeex-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libsrtp-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libgsmcodec-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libwebrtc-aarch64-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libilbccodec-aarch64-unknown-linux-android.a
}

equals(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/armeabi-v7a/include

    LIBS += $$EXT_LIB_PATH/skia/arm/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/arm \
    $$EXT_LIB_PATH/pjsip/lib/libpj-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjlib-util-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-audiodev-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-codec-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjnath-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-simple-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-ua-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsua-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libresample-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libspeex-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libsrtp-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libgsmcodec-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libwebrtc-armv7-unknown-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libilbccodec-armv7-unknown-linux-android.a
}

equals(ANDROID_TARGET_ARCH,x86) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/x86/include

    LIBS += $$EXT_LIB_PATH/skia/x86/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/x86 \
    $$EXT_LIB_PATH/pjsip/lib/libpj-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjlib-util-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-audiodev-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-codec-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjnath-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-simple-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-ua-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsua-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libresample-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libspeex-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libsrtp-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libgsmcodec-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libwebrtc-i686-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libilbccodec-i686-pc-linux-android.a
}

equals(ANDROID_TARGET_ARCH,x86_64) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/x86_64/include

    LIBS += $$EXT_LIB_PATH/skia/x86_64/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/x86_64 \
    $$EXT_LIB_PATH/pjsip/lib/libpj-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjlib-util-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-audiodev-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjmedia-codec-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjnath-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-simple-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsip-ua-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libpjsua-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libresample-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libspeex-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libsrtp-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libgsmcodec-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libwebrtc-x86_64-pc-linux-android.a \
    $$EXT_LIB_PATH/pjsip/lib/libilbccodec-x86_64-pc-linux-android.a
}

ios {

    LIBS += -L$$EXT_LIB_PATH/pjsip/lib \
            -L$$EXT_LIB_PATH/skia/iossim \
            -L$$EXT_LIB_PATH/openssl/iossim

    LIBS += -lskia -lpj -lpjlib-util \
            -lpjmedia -lpjmedia-audiodev \
            -lpjmedia-codec -lpjnath \
            -lpjsip -lpjsip-simple \
            -lpjsip-ua -lpjsua -lpjsua2 \
            -lresample -lspeex \
            -lsrtp -lgsmcodec \
            -lwebrtc -lilbccodec \
            -framework CFNetwork

    OBJECTIVE_SOURCES = $$PWD/QASettings.mm
    QTPLUGIN += qtsensors_ios
    QMAKE_INFO_PLIST = $$PWD/ios/Info.plist

    app_launch_files.files = $$files($$PWD/ios/Settings.bundle)
    QMAKE_BUNDLE_DATA += app_launch_files
}

equals(OS,osx) {
    INCLUDEPATH += /opt/homebrew/include /usr/local/include

    LIBS += -lskia -llibpj-arm-apple-darwin22.1.0.a \
            -llibpjlib-util-arm-apple-darwin22.1.0.a \
            -llibpjmedia-arm-apple-darwin22.1.0.a \
            -llibpjmedia-audiodev-arm-apple-darwin22.1.0.a \
            -llibpjmedia-codec-arm-apple-darwin22.1.0.a \
            -llibpjnath-arm-apple-darwin22.1.0.a \
            -llibpjsip-arm-apple-darwin22.1.0.a \
            -llibpjsip-simple-arm-apple-darwin22.1.0.a \
            -llibpjsip-ua-arm-apple-darwin22.1.0.a \
            -llibpjsua-arm-apple-darwin22.1.0.a \
            -llibresample-arm-apple-darwin22.1.0.a \
            -llibspeex-arm-apple-darwin22.1.0.a \
            -llibsrtp-arm-apple-darwin22.1.0.a \
            -llibgsmcodec-arm-apple-darwin22.1.0.a \
            -llibwebrtc-arm-apple-darwin22.1.0.a \
            -llibilbccodec-arm-apple-darwin22.1.0.a
}

# Define with the QT5_LINUX and QT6_LINUX definitions for which of the two
# Qt major versions you want to compile.
#
#  * QT5_LINUX     If defined the code for QT5.15 will be used
#  * QT6_LINUX     If defined the code for QT6.x will be used
#
# Never define both identifiers!
#
# Additional definitions possible:
#  * _SCALE_SKIA_   If this is defined the scaling is done by the Skia
#                   library. While the result is the same, it is known to be
#                   slower. Beside this, this feature is not well tested.
#
#  * QTSETTINGS     If this is set a Qt dialog is used for the settings. This
#                   dialog is scaled depending on the size of the screen. It
#                   is possible that the text is not readable or the fonts are
#                   not well sized. Beside this on Android the ComboBoxes may
#                   have wrong colors like black text on black background!
#
versionAtMost(QT_VERSION, 5.15.2) {
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00 QT5_LINUX=1
} else {
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00 QT6_LINUX=1
}

RESOURCES += \
    tpanel.qrc

FORMS += \
    keyboard.ui \
    keypad.ui \
    tqtsettings.ui \
    tqtphone.ui \
    download.ui

isEmpty(OS) {
    LIBS += -lcrypto_1_1 -lssl_1_1 -lEGL -landroid -lmediandk

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/gradle.properties \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/values/libs.xml

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
} else {
    LIBS += -lcrypto -lssl -liconv
}

# Add the ftp library
DEPENDPATH += $$PWD/ftplib
isEmpty(OS) {
# Add openSSL library for Android
android: include($$SDK_PATH/android_openssl/openssl.pri)
}
