add_library(ftp SHARED ftplib.cpp)

set_target_properties(ftp PROPERTIES VERSION 1.0.0)
set_target_properties(ftp PROPERTIES SOVERSION 1)
set_target_properties(ftp PROPERTIES PUBLIC_HEADER ftplib.h)

if (APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(ftp OpenSSL::SSL)
endif(APPLE)

if(ANDROID)
    set(OPENSSL_INCLUDE_DIRS $ENV{HOME}/Android/android_openssl/ssl_3/include)
    set(OPENSSL_SSL_LIBRARY $ENV{HOME}/Android/android_openssl/ssl_3/${ANDROID_ABI})
    include_directories(${OPENSSL_INCLUDE_DIRS})
    target_link_libraries(ftp -L${OPENSSL_SSL_LIBRARY} ssl_3 crypto_3)
elseif(NOT APPLE)
    target_link_libraries(ftp ssl crypto)
endif()

include(GNUInstallDirs)
install(TARGETS ftp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
