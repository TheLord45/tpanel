cmake_minimum_required(VERSION 3.10)

project(tpanel VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_USE_QTMULTIMEDIA TRUE)
set(QT_USE_QTMULTIMEDIAWIDGETS TRUE)

if(LINUX)
    option(USE_CLANG "Build application with clang" ON) # OFF is the default
    message(STATUS "Build application with clang: ${USE_CLANG}")
endif(LINUX)

if(IOS)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui Multimedia MultimediaWidgets Positioning Sensors)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui Multimedia MultimediaWidgets Positioning Sensors)
elseif(ANDROID)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui Multimedia MultimediaWidgets Quick Positioning)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui Multimedia MultimediaWidgets Quick Positioning)
else(IOS)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui Multimedia MultimediaWidgets Positioning)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui Multimedia MultimediaWidgets Positioning)
endif(IOS)

if(ANDROID)
    if(DEFINED $ENV{EXTRA_PATH})
        set(EXTRA_PATH $ENV{EXTRA_PATH})
    else()
        set(EXTRA_PATH $ENV{HOME}/Android/extras)
    endif()

    if(DEFINED $ENV{EXT_LIB_PATH})
        set(EXT_LIB_PATH $ENV{EXT_LIB_PATH})
    else()
        set(EXT_LIB_PATH $ENV{HOME}/Android/distribution)
    endif()

    if(DEFINED $ENV{SSL_PATH})
        set(SSL_PATH $ENV{SSL_PATH})
    else()
        set(SSL_PATH $ENV{HOME}/Android/android_openssl)
    endif()

    include_directories(${EXTRA_PATH}/expat/include)

    include_directories(${EXT_LIB_PATH}/skia/include)
    link_directories(${EXT_LIB_PATH}/skia/${ANDROID_ABI})

    include_directories(${EXT_LIB_PATH}/pjsip/include)
    include_directories(${SSL_PATH}/ssl_3/include)
    link_directories(${SSL_PATH}/ssl_3/${ANDROID_ABI})
    link_libraries(z ssl_3 crypto_3 mediandk expat libskia.a)

    link_directories(${EXT_LIB_PATH}/pjsip/lib)

    if(ANDROID_ABI STREQUAL "arm64-v8a")
        link_libraries(
            pj-aarch64-unknown-linux-android
            pjlib-util-aarch64-unknown-linux-android
            pjmedia-aarch64-unknown-linux-android
            pjmedia-audiodev-aarch64-unknown-linux-android
            pjmedia-codec-aarch64-unknown-linux-android
            pjnath-aarch64-unknown-linux-android
            pjsip-aarch64-unknown-linux-android
            pjsip-simple-aarch64-unknown-linux-android
            pjsip-ua-aarch64-unknown-linux-android
            pjsua-aarch64-unknown-linux-android
            resample-aarch64-unknown-linux-android
            speex-aarch64-unknown-linux-android
            srtp-aarch64-unknown-linux-android
            gsmcodec-aarch64-unknown-linux-android
            webrtc-aarch64-unknown-linux-android
            ilbccodec-aarch64-unknown-linux-android
        )
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        link_libraries(
            pj-armv7-unknown-linux-android
            pjlib-util-armv7-unknown-linux-android
            pjmedia-armv7-unknown-linux-android
            pjmedia-audiodev-armv7-unknown-linux-android
            pjmedia-codec-armv7-unknown-linux-android
            pjnath-armv7-unknown-linux-android
            pjsip-armv7-unknown-linux-android
            pjsip-simple-armv7-unknown-linux-android
            pjsip-ua-armv7-unknown-linux-android
            pjsua-armv7-unknown-linux-android
            resample-armv7-unknown-linux-android
            speex-armv7-unknown-linux-android
            srtp-armv7-unknown-linux-android
            gsmcodec-armv7-unknown-linux-android
            webrtc-armv7-unknown-linux-android
            ilbccodec-armv7-unknown-linux-android
        )
    elseif(ANDROID_ABI STREQUAL "x86_64")
        link_libraries(
            pj-x86_64-pc-linux-android
            pjlib-util-x86_64-pc-linux-android
            pjmedia-x86_64-pc-linux-android
            pjmedia-audiodev-x86_64-pc-linux-android
            pjmedia-codec-x86_64-pc-linux-android
            pjnath-x86_64-pc-linux-android
            pjsip-x86_64-pc-linux-android
            pjsip-simple-x86_64-pc-linux-android
            pjsip-ua-x86_64-pc-linux-android
            pjsua-x86_64-pc-linux-android
            resample-x86_64-pc-linux-android
            speex-x86_64-pc-linux-android
            srtp-x86_64-pc-linux-android
            gsmcodec-x86_64-pc-linux-android
            webrtc-x86_64-pc-linux-android
            ilbccodec-x86_64-pc-linux-android
        )
    elseif(ANDROID_ABI STREQUAL "x86")
        link_libraries(
            pj-i686-pc-linux-android
            pjlib-util-i686-pc-linux-android
            pjmedia-i686-pc-linux-android
            pjmedia-audiodev-i686-pc-linux-android
            pjmedia-codec-i686-pc-linux-android
            pjnath-i686-pc-linux-android
            pjsip-i686-pc-linux-android
            pjsip-simple-i686-pc-linux-android
            pjsip-ua-i686-pc-linux-android
            pjsua-i686-pc-linux-android
            resample-i686-pc-linux-android
            speex-i686-pc-linux-android
            srtp-i686-pc-linux-android
            gsmcodec-i686-pc-linux-android
            webrtc-i686-pc-linux-android
            ilbccodec-i686-pc-linux-android
        )
    endif()
elseif(IOS)
    if(DEFINED $ENV{EXT_LIB_PATH})
        set(EXT_LIB_PATH $ENV{EXT_LIB_PATH})
    else()
        set(EXT_LIB_PATH $ENV{HOME}/SDKs)
    endif()

    if(${CMAKE_OSX_SYSROOT} MATCHES "iphonesimulator")
        link_directories(${EXT_LIB_PATH}/openssl/ssl_3/iossim/lib
                ${EXT_LIB_PATH}/pjsip/lib
                ${EXT_LIB_PATH}/skia/iossim-apple)
        include_directories(${EXT_LIB_PATH}/openssl/ssl_3/iossim/include
                ${EXT_LIB_PATH}/pjsip/include
                ${EXT_LIB_PATH}/skia)
    else()
        link_directories(${EXT_LIB_PATH}/openssl/ssl_3/ios64/lib
                ${EXT_LIB_PATH}/pjsip/lib
                ${EXT_LIB_PATH}/skia/ios64)
        include_directories(${EXT_LIB_PATH}/openssl/ssl_3/ios64/include
                ${EXT_LIB_PATH}/pjsip/include
                ${EXT_LIB_PATH}/skia)
    endif()

    if(NOT DEFINED IPHONEOS_DEPLOYMENT_TARGET)
        set(IPHONEOS_DEPLOYMENT_TARGET "17.0")
    endif()

    link_libraries(z ssl crypto expat libskia.a)
    link_directories(${EXT_LIB_PATH}/pjsip/lib)

    link_libraries(pj pjlib-util
                   pjmedia pjmedia-audiodev
                   pjmedia-codec pjnath
                   pjsip pjsip-simple
                   pjsip-ua pjsua pjsua2
                   resample speex
                   srtp gsmcodec
                   webrtc ilbccodec
    )

    set(OBJECTIVE_HEADERS
        ios/QASettings.h
        ios/tiosrotate.h
        ios/tiosbattery.h
    )
    set(OBJECTIVE_SOURCES
        ios/QASettings.mm
        ios/tiosrotate.mm
        ios/tiosbattery.mm
    )

    set(settings_bundle "Settings.bundle")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    add_link_options(-framework CFNetwork)
 else()
    find_library(LIBSKIA_LIBRARY NAMES skia HINTS /usr/local/lib/skia)
    include_directories("/usr/local/include/skia")

    if(APPLE)
        link_directories("/usr/local/lib")
    else(APPLE)
        link_directories("/usr/local/lib/skia")
    endif(APPLE)

    link_libraries(skia)

    if(APPLE)
        link_libraries(freetype)
        find_package(ZLIB)
        find_package(EXPAT)
    else(APPLE)
        pkg_check_modules(ZLIB REQUIRED zlib)
        include_directories(${ZLIB_INCLUDE_DIRS})
        link_directories(${ZLIB_LIBRARY_DIRS})
        set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

        pkg_check_modules(EXPAT REQUIRED expat)
        include_directories(${EXPAT_INCLUDE_DIRS})
        link_directories(${EXPAT_LIBRARY_DIRS})
        set(LIBS ${LIBS} ${EXPAT_LIBRARIES})
    endif(APPLE)

    # This is for telephone libraries PJSIP
    pkg_check_modules(PJSIP REQUIRED libpjproject)
    message(STATUS "PJSIP: Includes: ${PJSIP_INCLUDE_DIRS}")
    message(STATUS "PJSIP: Libraries: ${PJSIP_LIBRARY_DIRS}")
    include_directories(${PJSIP_INCLUDE_DIRS})
    link_directories(${PJSIP_LIBRARY_DIRS})

    if(APPLE)
        # To avoid path problems on mac we add SIP as static libraries.
        link_libraries(
            libpjsua2-arm-apple-darwin23.0.0.a
            libpjsua-arm-apple-darwin23.0.0.a
            libpjsip-ua-arm-apple-darwin23.0.0.a
            libpjsip-simple-arm-apple-darwin23.0.0.a
            libpjsip-arm-apple-darwin23.0.0.a
            libpjmedia-codec-arm-apple-darwin23.0.0.a
            libpjmedia-arm-apple-darwin23.0.0.a
            libpjmedia-audiodev-arm-apple-darwin23.0.0.a
            libpjnath-arm-apple-darwin23.0.0.a
            libpjlib-util-arm-apple-darwin23.0.0.a
            libpj-arm-apple-darwin23.0.0.a
            libgsmcodec-arm-apple-darwin23.0.0.a
            libilbccodec-arm-apple-darwin23.0.0.a
            libspeex-arm-apple-darwin23.0.0.a
            libg7221codec-arm-apple-darwin23.0.0.a
            libsrtp-arm-apple-darwin23.0.0.a
            libresample-arm-apple-darwin23.0.0.a
            libwebrtc-arm-apple-darwin23.0.0.a
        )
    else(APPLE)
        set(LIBS ${LIBS} -L/usr/local/lib)
        link_libraries(
            pjsua2 pjsua pjsip-ua pjsip-simple pjsip
            pjmedia-codec pjmedia pjmedia-audiodev
            pjnath pjlib-util pj
            gsmcodec ilbccodec speex g7221codec
            srtp resample yuv webrtc
            opus asound)
    endif(APPLE)

    find_library(UUID_LIBRARY NAMES uuid)
    set(LIBS ${LIBS} ${UUID_LIBRARY})
    find_package(OpenSSL REQUIRED)

    if (APPLE)
        set(CMAKE_INSTALL_PREFIX /usr/local)
        # In case you've installed homebrew at another location adapt
        # the following paths!
        set(OSX_INCLUDE -I/opt/homebrew/include -I/usr/local/include)
        add_definitions(${OSX_INCLUDE})

        # The mac requires some additional frameworks for audio.
        find_library(COREAUDIO_LIBRARY CoreAudio)
        find_library(AUDIOUNIT_LIBRARY AudioUnit)
        find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
        mark_as_advanced(COREAUDIO_LIBRARY AUDIOUNIT_LIBRARY AUDIOTOOLBOX_LIBRARY)

        set(OSX_LIB -L/opt/homebrew/lib -L/usr/local/lib -liconv -lexpat -lz)
        set(LIBS ${LIBS} ${OSX_LIB} ${COREAUDIO_LIBRARY} ${AUDIOUNIT_LIBRARY} ${AUDIOTOOLBOX_LIBRARY})
    else(APPLE)
        set(CMAKE_INSTALL_PREFIX /usr)
    endif(APPLE)
endif()

set(PROJECT_SOURCES
        tmain.cpp
        tconfig.cpp
        tconfig.h
        terror.cpp
        terror.h
        tsocket.cpp
        tsocket.h
        tnameformat.cpp
        tnameformat.h
        tsettings.cpp
        tsettings.h
        tpagelist.cpp
        tpagelist.h
        tpagemanager.cpp
        tpagemanager.h
        tpage.cpp
        tpage.h
        tsubpage.cpp
        tsubpage.h
        tpageinterface.cpp
        tpageinterface.h
        tbutton.cpp
        tbutton.h
        tbuttonstates.cpp
        tbuttonstates.h
        tsystembutton.cpp
        tsystembutton.h
        tvalidatefile.cpp
        tvalidatefile.h
        tresources.cpp
        tresources.h
        tcrc32.cpp
        tcrc32.h
        tcolor.cpp
        tcolor.h
        tpalette.cpp
        tpalette.h
        tobject.cpp
        tobject.h
        tfont.cpp
        tfont.h
        ticons.cpp
        ticons.h
        tprjresources.cpp
        tprjresources.h
        thttpclient.cpp
        thttpclient.h
        timagerefresh.cpp
        timagerefresh.h
        tqtmain.cpp
        tqtmain.h
        tqtsettings.cpp
        tqtsettings.h
        tqtsettings.ui
        tqkeyboard.cpp
        tqkeyboard.h
        keyboard.ui
        tqkeypad.cpp
        tqkeypad.h
        keypad.ui
        tqtphone.cpp
        tqtphone.h
        tqtphone.ui
        tqtinputline.cpp
        tqtinputline.h
        tqtinputline.ui
        tqmarquee.cpp
        tqmarquee.h
        tqgesturefilter.cpp
        tqgesturefilter.h
        tamxnet.cpp
        tamxnet.h
        tamxcommands.cpp
        tamxcommands.h
        tmap.cpp
        tmap.h
        tdrawimage.cpp
        tdrawimage.h
        tdirectory.cpp
        tdirectory.h
        texpand.cpp
        texpand.h
        ttimer.cpp
        ttimer.h
        ttpinit.cpp
        ttpinit.h
        texternal.cpp
        texternal.h
        tsystemsound.cpp
        tsystemsound.h
        tsystemdraw.cpp
        tsystemdraw.h
        timgcache.cpp
        timgcache.h
        tfsfreader.cpp
        tfsfreader.h
        tscramble.cpp
        tscramble.h
        readtp4.cpp
        readtp4.h
        expand.cpp
        expand.h
        tsipclient.cpp
        tsipclient.h
        texcept.cpp
        texcept.h
        tsystem.cpp
        tsystem.h
        turl.cpp
        turl.h
        base64.cpp
        base64.h
        texpat++.cpp
        texpat++.h
        tqdownload.cpp
        tqdownload.h
        download.ui
        tqeditline.cpp
        tqeditline.h
        tqsingleline.cpp
        tqsingleline.h
        tqmultiline.cpp
        tqmultiline.h
        tqintercom.cpp
        tqintercom.h
        taudioconvert.cpp
        taudioconvert.h
        tqtwait.cpp
        tqtwait.h
        wait.ui
        tqscrollarea.cpp
        tqscrollarea.h
        tbitmap.cpp
        tbitmap.h
        tintborder.cpp
        tintborder.h
        tbytearray.cpp
        tbytearray.h
        tvector.h
        tlock.h
        testmode.cpp
        testmode.h
        tapps.cpp
        tapps.h
        tlauncher.cpp
        tlauncher.h
        tpanel.qrc
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
endif()

add_definitions(-D_REENTRANT)
add_definitions(-D_GNU_SOURCE)
add_definitions(-DPJ_AUTOCONF)
add_definitions(-D_OPAQUE_SKIA_)

if(ANDROID)
    add_definitions(-pedantic -fexceptions -Wextra -Wno-attributes -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-gnu-zero-variadic-macro-arguments)
elseif(IOS)
    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        ${OBJECTIVE_HEADERS}
        ${OBJECTIVE_SOURCES}
    )
    add_definitions(-Wno-gnu-zero-variadic-macro-arguments -Wshorten-64-to-32)
else()
    add_definitions(-DTESTMODE=1 -DSKIA_20250812)

    if(USE_CLANG)
        add_definitions(-pedantic -fexceptions -Wextra -Wno-attributes)
    else(USE_CLANG)
        add_definitions(-pedantic -fexceptions -Wextra -Wno-gnu-zero-variadic-macro-arguments -Wno-attributes -Wno-nested-anon-types)
    endif(USE_CLANG)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    if(IOS)
        qt_add_executable(tpanel
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            "${settings_bundle}"
        )
    else()
        qt_add_executable(tpanel
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
        )
    endif()

    if(ANDROID)
    # Define target properties for Android with Qt 6 as:
        set_property(TARGET tpanel APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                     ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
    endif()
else() # The following is deprecated! Qt5 is no longer supported.
    if(ANDROID)
        add_library(tpanel SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
        set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(tpanel
            ${PROJECT_SOURCES}
        )
    endif()
endif()

if(ANDROID)
    if(DEFINED SSL_PATH)
        set_property(TARGET tpanel PROPERTY QT_ANDROID_EXTRA_LIBS
            ${SSL_PATH}/ssl_3/${ANDROID_ABI}/libcrypto_3.so
            ${SSL_PATH}/ssl_3/${ANDROID_ABI}/libssl_3.so
        )
        message("++ Added openSSL libraries from \"${SSL_PATH}/ssl_3/${ANDROID_ABI}.\"")
    else()
        message("## ERROR: Missing SSL_PATH variable!")
    endif()
endif()

if(IOS)
    target_link_libraries(tpanel PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::MultimediaWidgets
        Qt${QT_VERSION_MAJOR}::Positioning
        Qt${QT_VERSION_MAJOR}::Sensors
        ftp ${LIBS}
    )
    if (${QT_VERSION_MAJOR} EQUAL 6 AND ${QT_VERSION_MINOR} GREATER_EQUAL 8)
        qt_add_ios_ffmpeg_libraries(tpanel)
    else()
        qt6_add_ios_ffmpeg_libraries(tpanel)
    endif()
elseif(ANDROID)
    target_link_libraries(tpanel PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::MultimediaWidgets
        Qt${QT_VERSION_MAJOR}::Positioning
        Qt${QT_VERSION_MAJOR}::Quick
        ftp ${LIBS}
    )
elseif(APPLE)
    target_link_libraries(tpanel PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::MultimediaWidgets
        Qt${QT_VERSION_MAJOR}::Positioning
        ftp ssl crypto ${LIBS}
    )
else()
    target_link_libraries(tpanel PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::MultimediaWidgets
        Qt${QT_VERSION_MAJOR}::Positioning
        OpenSSL::SSL
        ftp ${LIBS}
    )
endif()

if(IOS)
    # Icons
    set(asset_catalog_path "Assets.xcassets")
    file(GLOB_RECURSE app_icon_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/images/AppIcon*.png")
    file(GLOB_RECURSE bgimages_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/images/LaunchImage-*.png")
    list(APPEND app_icon_files "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/AppIcon.appiconset/Contents.json")
    list(APPEND bgimages_portrait "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/Images-portrait.imageset")
    list(APPEND bgimages_portrait "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/Images-portrait.imageset/Contents.json")
    list(APPEND bgimages_landscape "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/Images-landscape.imageset")
    list(APPEND bgimages_landscape "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/Images-landscape.imageset/Contents.json")
    target_sources(tpanel PRIVATE ${app_icon_files} "${asset_catalog_path}" "${bgimages_files}")
    list(APPEND app_icon_set "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/AppIcon.appiconset")
    list(APPEND app_icon_set "${CMAKE_CURRENT_SOURCE_DIR}/${asset_catalog_path}/Contents.json")
    set_source_files_properties(${app_icon_files} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${app_icon_set} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${asset_catalog_path}/AppIcon.appiconset")
    set_source_files_properties(${bgimages_files} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${bgimages_portrait} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${asset_catalog_path}/Images-portrait.imageset")
    set_source_files_properties(${bgimages_landscape} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${asset_catalog_path}/Images-landscape.imageset")
    # Settings bundle
    target_sources(tpanel PRIVATE "${settings_bundle}")
    set_source_files_properties(${settings_bundle} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # Launch screen
#    set(launch_screen "ios/LaunchScreen.storyboard")
#    target_sources(tpanel PRIVATE ${launch_screen})
#    set_source_files_properties(${launch_screen} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${launch_screen}")

    set_target_properties(tpanel PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER org.qtproject.theosys
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        WIN32_EXECUTABLE FALSE
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
        QT_NO_SET_IOS_LAUNCH_SCREEN TRUE
#        QT_IOS_LAUNCH_SCREEN "${CMAKE_CURRENT_SOURCE_DIR}/${launch_screen}"
    )
else()
    set_target_properties(tpanel PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER org.qtproject.theosys
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE FALSE
    )
endif()

install(TARGETS tpanel
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(tpanel)
endif()
