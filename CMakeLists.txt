cmake_minimum_required(VERSION 3.1.0)
project(tpanel VERSION 1.3.1)

find_package(PkgConfig)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Multimedia)
find_package(Qt5MultimediaWidgets)
find_package(Qt5Qml)
find_package(Qt5QuickWidgets)

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

find_package(Linphone REQUIRED)
find_library(LINPHONE_LIBRARY NAMES linphone)
set(LIBS ${LIBS} ${LINPHONE_LIBRARIES})

find_library(UUID_LIBRARY NAMES uuid)
set(LIBS ${LIBS} ${UUID_LIBRARY})

set(CMAKE_INSTALL_PREFIX /usr)
set(QT_USE_QTMULTIMEDIA TRUE)
set(QT_USE_QTMULTIMEDIAWIDGETS TRUE)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Multimedia REQUIRED)
find_package(Qt5 COMPONENTS MultimediaWidgets REQUIRED)
find_package(Qt5 COMPONENTS Qml QuickWidgets REQUIRED)

if (APPLE)
	set(OSX_INCLUDE -I/opt/local/include -I/usr/local/include)
	set(OSX_LIB -L/opt/local/lib -L/usr/local/lib)
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
			tqemitqueue.cpp
			tamxnet.cpp
			tamxcommands.cpp
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
			base64.cpp
			texpat++.cpp
			tqtbusy.cpp
			tpanel.qrc)

add_executable(tpanel ${SOURCES})

add_definitions(-D_REENTRANT)
add_definitions(-D_GNU_SOURCE)
add_definitions(-DQT5_LINUX)
add_definitions(${Qt5Widgets_DEFINITIONS} ${QtQml_DEFINITIONS} ${${Qt5Quick_DEFINITIONS}})
include_directories(/usr/local/include/skia)

if (DEBUG)
   add_definitions(-g)
endif(DEBUG)

add_definitions(-pedantic -fexceptions -Wextra)

find_library(LIBSKIA_LIBRARY NAMES skia HINTS /usr/local/lib/skia)

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
	target_link_libraries(tpanel stdc++fs)
endif()

target_link_libraries(tpanel m pthread ssl crypto ftp ${LIBS} ${LIBSKIA_LIBRARY} Qt5::Widgets Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Qml Qt5::QuickWidgets)

install(TARGETS tpanel RUNTIME DESTINATION bin)
