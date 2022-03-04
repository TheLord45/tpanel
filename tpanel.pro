# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

TARGET = tpanel
QT = core gui widgets multimedia multimediawidgets qml quick quickwidgets androidextras

# The main application
HEADERS = \
   $$PWD/base64.h \
   $$PWD/config.h \
   $$PWD/tamxcommands.h \
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
   $$PWD/tqtbusy.h \
   $$PWD/texcept.h \
   $$PWD/tfsfreader.cpp \
   $$PWD/timgcache.h \
   $$PWD/tsystemsound.h \
   $$PWD/tsystemdraw.h \
   $$PWD/texpat++.h \
   $$PWD/ftplib/ftplib.h

SOURCES = \
   $$PWD/base64.cpp \
   $$PWD/tamxcommands.cpp \
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
   $$PWD/tqtbusy.cpp \
   $$PWD/tfsfreader.h \
   $$PWD/texcept.cpp \
   $$PWD/timgcache.cpp \
   $$PWD/tsystemsound.cpp \
   $$PWD/tsystemdraw.cpp \
   $$PWD/texpat++.cpp \
   $$PWD/ftplib/ftplib.cpp

OTHER_FILES += \
        $$PWD/android/src/org/qtproject/theosys/BatteryState.java \
        $$PWD/android/src/org/qtproject/theosys/NetworkStatus.java \
        $$PWD/android/src/org/qtproject/theosys/PhoneCallState.java \
        $$PWD/android/src/org/qtproject/theosys/UriToPath.java \
        $$PWD/android/src/org/qtproject/theosys/Logger.java

INCLUDEPATH = \
    $$PWD/. \
    $$PWD/ftplib \
    $$EXT_LIB_PATH/skia \
    $$EXTRA_PATH/expat/include

android: include($$SDK_PATH/android_openssl/openssl.pri)

equals(ANDROID_TARGET_ARCH,arm64-v8a) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/arm64-v8a/include

    LIBS += $$EXT_LIB_PATH/skia/arm64/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/arm64
}

equals(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/armeabi-v7a/include

    LIBS += $$EXT_LIB_PATH/skia/arm/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/arm
}

equals(ANDROID_TARGET_ARCH,x86) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/x86/include

    LIBS += $$EXT_LIB_PATH/skia/x86/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/x86
}

equals(ANDROID_TARGET_ARCH,x86_64) {
    INCLUDEPATH += $$EXT_LIB_PATH/openssl/x86_64/include

    LIBS += $$EXT_LIB_PATH/skia/x86_64/libskia.a \
    -L$$SDK_PATH/android_openssl/no-asm/latest/x86_64
}

RESOURCES += \
    tpanel.qrc

FORMS += \
    keyboard.ui \
    keypad.ui \
    tqtsettings.ui \
    busy.ui

LIBS += -lcrypto_1_1 -lssl_1_1 -lEGL -lGLESv2 -landroid -llog

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    qrc/BusyIndicator.qml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

# Add the ftp library
DEPENDPATH += $$PWD/ftplib
