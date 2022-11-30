cmake_minimum_required(VERSION 3.1.0)
project(tpanel VERSION 1.3.3)

find_package(PkgConfig)

if (APPLE)
	# ATTENTION!!
	# Correct this paths to your needs!
    set(QTDIR $ENV{HOME}/Qt/6.4.1/macos)
    set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt/6.4.1/macos/lib/cmake)
endif(APPLE)

find_package(Qt6Core)
find_package(Qt6Gui)
find_package(Qt6Widgets)

#if (NOT APPLE)
    find_package(Qt6Multimedia)
    find_package(Qt6MultimediaWidgets)
#endif(NOT APPLE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

pkg_check_modules(ZLIB REQUIRED zlib)
include_directories(${ZLIB_INCLUDE_DIRS})
link_directories(${ZLIB_LIBRARY_DIRS})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

pkg_check_modules(EXPAT REQUIRED expat)
include_directories(${EXPAT_INCLUDE_DIRS})
link_directories(${EXPAT_LIBRARY_DIRS})
set(LIBS ${LIBS} ${EXPAT_LIBRARIES})

# This is for telephone libraries PJSIP
pkg_check_modules(PJSIP REQUIRED libpjproject)
include_directories(${PJSIP_INCLUDE_DIRS})
link_directories(${PJSIP_LIBRARY_DIRS})
# Commented out to make sure no video is included (would require a lot more libs)
#set(LIBS ${LIBS} ${PJSIP_LIBRARIES})
# This are the phone libraries used/needed for PJSIP.
if(APPLE)
set(LIBS ${LIBS} -L/usr/local/lib
                 pjsua2 pjsua pjsip-ua pjsip-simple
                 pjsip pjmedia-codec pjmedia pjmedia-audiodev
                 pjnath pjlib-util pj gsmcodec
                 ilbccodec speex g7221codec
                 srtp resample webrtc)
else(APPLE)
set(LIBS ${LIBS} -L/usr/local/lib
                 pjsua2 pjsua pjsip-ua pjsip-simple pjsip
                 pjmedia-codec pjmedia pjmedia-audiodev
                 pjnath pjlib-util pj
                 gsmcodec ilbccodec speex g7221codec
                 srtp resample yuv webrtc
                 opus asound)
endif(APPLE)

find_library(UUID_LIBRARY NAMES uuid)
set(LIBS ${LIBS} ${UUID_LIBRARY})

if (APPLE)
	set(CMAKE_INSTALL_PREFIX /usr/local)
else()
	set(CMAKE_INSTALL_PREFIX /usr)
endif(APPLE)

set(QT_USE_QTMULTIMEDIA TRUE)
set(QT_USE_QTMULTIMEDIAWIDGETS TRUE)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Multimedia REQUIRED)
find_package(Qt6 COMPONENTS MultimediaWidgets REQUIRED)

if (APPLE)
	# In case you've installed homebrew at another location adapt
	# the following paths!
	set(OSX_INCLUDE -I/opt/homebrew/include -I/usr/local/include)
	add_definitions(${OSX_INCLUDE})
	set(OSX_LIB -L/opt/homebrew/lib -L/usr/local/lib -liconv)
	set(LIBS ${LIBS} ${OSX_LIB})
endif(APPLE)

add_subdirectory(ftplib)

set(SOURCES tmain.cpp
			tconfig.cpp
			terror.cpp
			tsocket.cpp
			tnameformat.cpp
			tsettings.cpp
			tpagelist.cpp
			tpagemanager.cpp
			tpage.cpp
			tsubpage.cpp
			tpageinterface.cpp
			tbutton.cpp
			tvalidatefile.cpp
			tresources.cpp
			tcolor.cpp
			tpalette.cpp
			tobject.cpp
			tfont.cpp
			ticons.cpp
			tprjresources.cpp
			thttpclient.cpp
			timagerefresh.cpp
			tqtmain.cpp
			tqtsettings.cpp
			tqkeyboard.cpp
			tqkeypad.cpp
			tqtphone.cpp
			tqemitqueue.cpp
			tamxnet.cpp
			tamxcommands.cpp
			tmap.cpp
			tdrawimage.cpp
			tdirectory.cpp
			texpand.cpp
			ttimer.cpp
			ttpinit.cpp
			texternal.cpp
			tsystemsound.cpp
			tsystemdraw.cpp
			timgcache.cpp
			tfsfreader.cpp
			readtp4.cpp
			expand.cpp
			tsipclient.cpp
			texcept.cpp
			tsystem.cpp
			turl.cpp
			base64.cpp
			texpat++.cpp
			tqdownload.cpp
			tqeditline.cpp
			tpanel.qrc)

add_executable(tpanel ${SOURCES})

add_definitions(-D_REENTRANT)
add_definitions(-D_GNU_SOURCE)
add_definitions(-DQT6_LINUX)
add_definitions(-DPJ_AUTOCONF)
add_definitions(${Qt6Widgets_DEFINITIONS})
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)
include_directories(/usr/local/include/skia)

if (DEBUG)
   add_definitions(-g)
endif(DEBUG)

add_definitions(-pedantic -fexceptions -Wextra -Wno-attributes -Wno-nested-anon-types -Wno-gnu-anonymous-struct)

find_library(LIBSKIA_LIBRARY NAMES skia HINTS /usr/local/lib/skia)

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
	target_link_libraries(tpanel stdc++fs)
endif()

target_link_libraries(tpanel m pthread ssl crypto ftp ${LIBS} ${LIBSKIA_LIBRARY} Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)

if (APPLE)
    add_definitions(-Wno-gnu-zero-variadic-macro-arguments)
	set(DIRS ${CMAKE_BINARY_DIR})
	set(APPS "\${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app")
	set(CPACK_BUNDLE_NAME "tpanel-\${VERSION}")
	set(CPACK_BUNDLE_ICON "images/icon.png")
#	set(CPACK_BUNDLE_PLIST "")
	set(CPACK_GENERATOR "DragNDrop")
#	add_executable(${APP_NAME} MACOSX_BUNDLE ${SOURCES})
#	install(CODE "include(BundleUtilities)
#    	fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")
endif(APPLE)

install(TARGETS tpanel RUNTIME DESTINATION bin)
include(CPack)
