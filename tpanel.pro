# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

TARGET = tpanel

QT = core gui widgets multimedia multimediawidgets sensors positioning

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
   $$PWD/tsystembutton.h \
   $$PWD/tpalette.h \
   $$PWD/tprjresources.h \
   $$PWD/tqtmain.h \
   $$PWD/testmode.h \
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
   $$PWD/tqnetworkinfo.h \
   $$PWD/tqtwait.h \
   $$PWD/tqscrollarea.h \
   $$PWD/tbitmap.h \
   $$PWD/tintborder.h \
   $$PWD/tlock.h \
   $$PWD/ftplib/ftplib.h \
   $$PWD/tqsingleline.h \
   $$PWD/tqgesturefilter.h \
   $$PWD/tqmultiline.h

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
   $$PWD/testmode.cpp \
   $$PWD/tnameformat.cpp \
   $$PWD/tobject.cpp \
   $$PWD/tpage.cpp \
   $$PWD/tpagelist.cpp \
   $$PWD/tpagemanager.cpp \
   $$PWD/tpageinterface.cpp \
   $$PWD/tsystembutton.cpp \
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
   $$PWD/tqnetworkinfo.cpp \
   $$PWD/tqtwait.cpp \
   $$PWD/tqscrollarea.cpp \
   $$PWD/tbitmap.cpp \
   $$PWD/tintborder.cpp \
   $$PWD/ftplib/ftplib.cpp \
   $$PWD/tqsingleline.cpp \
   $$PWD/tqgesturefilter.cpp \
   $$PWD/tqmultiline.cpp

ios: {
    INCLUDEPATH += $$PWD/. \
                   $$PWD/ftplib \
                   /usr/local/include/skia \
                   $$EXT_LIB_PATH/pjsip/include
    versionAtMost(QT_VERSION, 6.5.0) {
        INCLUDEPATH += $$EXT_LIB_PATH/openssl/ssl_3/ios64/include
    } else {
        INCLUDEPATH += $$EXT_LIB_PATH/openssl/ssl_1.1/include
    }

    QMAKE_IOS_DEPLOYMENT_TARGET=16.1
}

CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17 -DPJ_AUTOCONF
QMAKE_LFLAGS += -std=c++17

ios: {
    equals(OS,iossim) {
        LIBS += -L$$EXT_LIB_PATH/pjsip/lib \
                -L$$EXT_LIB_PATH/skia/iossim

        versionAtMost(QT_VERSION, 6.5.0) {
            LIBS += -L$$EXT_LIB_PATH/openssl/ssl_3/iossim/lib
        } else {
            LIBS += -L$$EXT_LIB_PATH/openssl/ssl_1.1/iossim
        }
    } else {
        LIBS += -L$$EXT_LIB_PATH/pjsip/lib \
                -L$$EXT_LIB_PATH/skia/ios64

        versionAtMost(QT_VERSION, 6.5.0) {
            LIBS += -L$$EXT_LIB_PATH/openssl/ssl_3/ios64/lib
        } else {
            LIBS += -L$$EXT_LIB_PATH/openssl/ssl_1.1/ios
        }
    }

    LIBS += -lskia -lpj -lpjlib-util \
            -lpjmedia -lpjmedia-audiodev \
            -lpjmedia-codec -lpjnath \
            -lpjsip -lpjsip-simple \
            -lpjsip-ua -lpjsua -lpjsua2 \
            -lresample -lspeex \
            -lsrtp -lgsmcodec \
            -lwebrtc -lilbccodec \
            -framework CFNetwork

    INCLUDEPATH += $$(QTDIR)/include/QtGui/$$QT_VERSION/QtGui

    OBJECTIVE_HEADERS = $$PWD/ios/QASettings.h \
                        $$PWD/ios/tiosrotate.h \
                        $$PWD/ios/tiosbattery.h

    OBJECTIVE_SOURCES = $$PWD/ios/QASettings.mm \
                        $$PWD/ios/tiosrotate.mm \
                        $$PWD/ios/tiosbattery.mm

    QMAKE_INFO_PLIST = $$PWD/ios/Info.plist

    app_launch_images.files = $$files($$PWD/images/theosys_logo.png)
    app_launch_files.files = $$files($$PWD/ios/Settings.bundle)
    ios_icon.files = $$files($$PWD/images/icon*.png)
    QMAKE_BUNDLE_DATA += app_launch_files ios_icon app_launch_images
}


# Additional definitions possible:
#  * _SCALE_SKIA_   If this is defined the scaling is done by the Skia
#                   library. While the result is the same, it is known to be
#                   slower. Beside this, this feature is not well tested.
#
#  * _OPAQUE_SKIA_  If set (default) the calculation for the opaque value is
#                   done by Skia. This works well but leaves a gray tinge.
#                   The Qt calculation is currently not working!
#                   THIS IS DEPRECATED AND WILL BE REMOVED!
#
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00 _OPAQUE_SKIA_

RESOURCES += \
    tpanel.qrc

FORMS += \
    keyboard.ui \
    keypad.ui \
    tqtsettings.ui \
    tqtphone.ui \
    download.ui \
    wait.ui

LIBS += -lcrypto -lssl -liconv

# Add the ftp library
DEPENDPATH += $$PWD/ftplib
