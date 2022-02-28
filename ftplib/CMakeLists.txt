add_library(ftp SHARED ftplib.cpp)

set_target_properties(ftp PROPERTIES VERSION 1.0.0)
set_target_properties(ftp PROPERTIES SOVERSION 1)
set_target_properties(ftp PROPERTIES PUBLIC_HEADER ftplib.h)

include(GNUInstallDirs)
install(TARGETS ftp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
