add_library(ftp SHARED ftplib.cpp)

set_target_properties(ftp PROPERTIES VERSION 1.0.0)
set_target_properties(ftp PROPERTIES SOVERSION 1)
set_target_properties(ftp PROPERTIES PUBLIC_HEADER ftplib.h)

if (APPLE)
    set(OSX_LIB -L/opt/homebrew/lib -L/usr/local/lib)
    set(LIBS ${LIBS} ${OSX_LIB})
    target_link_libraries(ftp ssl crypto)
endif(APPLE)

include(GNUInstallDirs)
install(TARGETS ftp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
