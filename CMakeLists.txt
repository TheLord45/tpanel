cmake_minimum_required(VERSION 3.16)

project(tpanel VERSION 2.0.0 LANGUAGES CXX)

if(ANDROID)
    message("** Current platform (${ANDROID_ABI}): ${ANDROID_PLATFORM}")

    if(NOT "$ENV{ANDROID_PLATFORM}" STREQUAL "")
        message("** Environment (${ANDROID_ABI}): $ENV{ANDROID_PLATFORM}")

        if(NOT (DEFINED ANDROID_PLATFORM) OR NOT (${ANDROID_PLATFORM} STREQUAL $ENV{ANDROID_PLATFORM}))
            set(ANDROID_PLATFORM $ENV{ANDROID_PLATFORM})
            message("++ Set ANDROID_PLATFORM to ${ANDROID_PLATFORM} for ${ANDROID_ABI}")
        endif()
    else()
        if(NOT (DEFINED ANDROID_PLATFORM) OR NOT (${ANDROID_PLATFORM} STREQUAL "android-30"))
            set(ANDROID_PLATFORM "android-30")
            message("++ Set ANDROID_PLATFORM to ${ANDROID_PLATFORM} for ${ANDROID_ABI}")
        endif()
    endif()
endif(ANDROID)

add_subdirectory(ftplib)
add_subdirectory(src)
