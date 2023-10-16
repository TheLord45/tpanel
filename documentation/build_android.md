# Build Android APK

There are 2 possibilities to create an Android APK file. You can use the ready script on the command line to build it, or use _QtCreator_. Both methods are based on _cmake_. The following description shows both methods and explain what you need and how to put the things together.

Developing for another platform means to _cross compile_ code. This is necessary because there is no way (not that I know) to create the program directly on the target platform. And even if it would be a nightmere to develop a complex program like **TPanel** directly on an Android phone.

> This documentation assumes that you're creating **TPanel** on a Linux system.

## Prerequisites

We need the [Android SDK](https://developer.android.com/). In case you've not installed it, do it now. I recommend to install the SDK into your home directory. Usually this is installed in the directory `<home directory>/Android/Sdk`. Additionally you need the following libraries compiled for Android:

- [Skia](https://skia.org)
- [Qt 6.5.x](https://doc.qt.io/qt-6/) or newer
- openssl (can easily be installed out of QtCreator)
- [pjsip](https://www.pjsip.org)

> _Hint_: For Qt version less then 6.5.0 you need openssl 1.1, while for newer versions of Qt you need openssl 3!

I made an archive file containing _Skia_ and _PjSIP_ as precompiled versions for all 4 possible Android architectures (arm64-v8a, armeabi-v7a, x86_64, x86) You can download it from my [server](https://www.theosys.at/download/android_dist.tar.bz2) (~ 1Gb!). After you've downloaded this hugh file, unpack it into any directory. The file details are:

- [`android_dist.tar.bz2`](https://www.theosys.at/download/android_dist.tar.bz2)
- SHA256: `a79b313d03aa630f9574e5d9652fa1c28ef8d766a7cbb270d406838260c8c2b2`

> The package does **not** include the Qt framework!

## Build with script `build_android.sh`

This is a shell script to create an Android APK file. Such files can be deployed to any Android device running Android 11 or newer. This is API level 30.

First copy the file `build_android.sh` to another name e.g.:

    cp build_android.sh buildAndroid.sh

Now open the copy with a text editor. The first lines defining some paths. Adapt the settings to the paths of your installation. Then simply start the script on the command line. If everything was installed and all paths were set correct, it should compile the code and produce an APK file. The last line will give you the path and name of the produced file.

The script contains a help function (`build_android.sh --help`). It shows the possible parameters and their meaning:
```
build_android.sh [clean] [debug] [deploy] [sign] [list-avds] [help|--help|-h]
   clean     Delete old build, if there is one, and start a new clean build.
   debug     Create a binary with debugging enabled.
   deploy    Deploy the binary to an Android emulator.
   sign      Sign the resulting APK file. This requires a file named
             "$HOME/.keypass" containing the password and has read
             permissions for the owner only (0400 or 0600). If this file
             is missing, the script asks for the password.
   verbose | --verbose | -v
             Prints the commands used to create the target file.
   list-avds List all available AVDs and exit.

   help | --help | -h   Displays this help screen and exit.

Without parameters the source is compiled, if there were changes, and then
an Android package is created. This package will be an unsigned release APK.
```

## Build with _QtCreator_

There are some reasons why it is better to use [QtCreator](https://www.qt.io/product/development-tools) then using the command line. The most important reason is if you want to be able to flexible deploy the application to different Android devices. And if you want to participate in developing you should use this IDE also.

After starting _QtCreator_ click on `Open project` and open the file `CMakeLists.txt` in the source folder of **TPanel**. When the project is open, click on `Projects` on left panel. Select `Android Qt 6.x.x CLang x86_64` from the list and click on `Build` (hammer symbol). Now you see the _Build Settings_. Under `CMake` set in the _Initial Configuration_ the value of `ANDROID_PLATFORM` to `android-30`. Make sure the NDK version `25.2.9519653` is set. If not, hit `Manage Kits...` and make this version the default or install it and make it then the default. I will not explain how to install the Android SDK or an NDK, which is part of the Android SDK. This is subject to the Google documentation of Android Studio.

Click now on `Re-configure with Initial Parameters` (on the bottom of the `Cmake` box.). If everything went well, the tab should jump to `Current Configuration`. If you want to build Android for all ABIs you must set `QT_ANDROID_BUILD_ALL_ABIS` to `ON`. Leave the rest as it is.

> _Hint_: If you want to compile sources in parallel (much faster) you can add the parameter `-j<#cpus>` (replace `<#cpus>` with the number of CPUs your system offers) to `Additional CMake options:`.

At the block called `Build Android APK` set under `Application` the `Android build platform SDK:` to `android-30`. This very important because with an SDK version less it may not even compile. You can, if you like, use any newer version.

Expand the block `Build Environment` and set the variable `ANDROID_NDK_PLATFORM` to `android-30`. _Do not check the `Clear system environment` check box!_

Before you can start compiling, click on the Android symbol in the left bottom corner and select `tpanel` under _Run_. Now you can compile the program by clicking on the hammer symbol.

# Problems

With Qt 6.5.2 and older versions it is very likely that you can't compile the code. You will see errors like this:
```
Changing into build directory "tpanel-6-build" ...
Compiling the source ...
[1/72] Performing tpanel_build step for 'qt_internal_android_x86'
FAILED: qt_internal_android_x86-prefix/src/qt_internal_android_x86-stamp/qt_internal_android_x86-tpanel_build /home/<user>/tpanel/tpanel-6-build/qt_internal_android_x86-prefix/src/qt_internal_android_x86-stamp/qt_internal_android_x86-tpanel_build
cd /home/<user>/tpanel/tpanel-6-build && /opt/Qt/Tools/CMake/bin/cmake --build /home/<user>/tpanel/tpanel-6-build/android_abi_builds/x86 --config Release --target tpanel
[1/58] Building CXX object CMakeFiles/tpanel.dir/tnameformat.cpp.o
FAILED: CMakeFiles/tpanel.dir/tnameformat.cpp.o
/home/<user>/Android/ndk/25.1.8937393/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ --target=i686-none-linux-android23 --sysroot=/home/<user>/Android/ndk/25.1.8937393/toolchains/llvm/prebuilt/linux-x86_64/sysroot -DPJ_AUTOCONF -DQT_CORE_LIB -DQT_GUI_LIB -DQT_MULTIMEDIAWIDGETS_LIB -DQT_MULTIMEDIA_LIB -DQT_NETWORK_LIB -DQT_NO_DEBUG -DQT_POSITIONING_LIB -DQT_WIDGETS_LIB -D_GNU_SOURCE -D_OPAQUE_SKIA_ -D_REENTRANT -Dtpanel_EXPORTS -I/home/<user>/tpanel/tpanel-6-build/android_abi_builds/x86/tpanel_autogen/include -I/home/<user>/Android/extras/expat/include -I/home/<user>/Android/distribution/skia/include -I/home/<user>/Android/distribution/pjsip/include -I/home/<user>/Android/android_openssl/ssl_3/include -isystem /opt/Qt/6.5.2/android_x86/include/QtCore -isystem /opt/Qt/6.5.2/android_x86/include -isystem /opt/Qt/6.5.2/android_x86/mkspecs/android-clang -isystem /opt/Qt/6.5.2/android_x86/include/QtWidgets -isystem /opt/Qt/6.5.2/android_x86/include/QtGui -isystem /opt/Qt/6.5.2/android_x86/include/QtMultimedia -isystem /opt/Qt/6.5.2/android_x86/include/QtNetwork -isystem /opt/Qt/6.5.2/android_x86/include/QtMultimediaWidgets -isystem /opt/Qt/6.5.2/android_x86/include/QtPositioning -g -DANDROID -fdata-sections -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -mstackrealign -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security   -O3 -DNDEBUG  -fPIC -fvisibility=default   -pedantic -fexceptions -Wextra -Wno-attributes -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-gnu-zero-variadic-macro-arguments -fPIC -pthread -std=gnu++17 -MD -MT CMakeFiles/tpanel.dir/tnameformat.cpp.o -MF CMakeFiles/tpanel.dir/tnameformat.cpp.o.d -o CMakeFiles/tpanel.dir/tnameformat.cpp.o -c /home/<user>/tpanel/tnameformat.cpp
/home/<user>/tpanel/tnameformat.cpp:29:2: error: "This module needs Android API level 28 or newer!"
#error "This module needs Android API level 28 or newer!"
 ^
/home/<user>/tpanel/tnameformat.cpp:223:18: error: use of undeclared identifier 'iconv_open'
    iconv_t cd = iconv_open(from.c_str(), to.c_str());
                 ^
/home/<user>/tpanel/tnameformat.cpp:240:7: error: use of undeclared identifier 'iconv'; did you mean 'lconv'?
                if (iconv(cd, &in_buf, &in_left, &out_buf, &out_left) == (size_t) -1)
                    ^
...
```
You can see the line `#error "This module needs Android API level 28 or newer!"`. This happens when you try to make a multi architecture APK file. There is a bug in the cmake macros used for Android. Here is how you can fix this.

- Go to the directory where Qt is installed (e.g. `/opt/Qt/6.5.2`)
- Make sure you have the right to write all files and directories.
- With a text editor open the file `android_x86_64/lib/cmake/Qt6Core/Qt6AndroidMacros.cmake`.
- Search for a function called `_qt_internal_configure_android_multiabi_target`.
- Scroll down to (about line 1180)
```
...
    if(DEFINED QT_HOST_PATH_CMAKE_DIR)
        list(APPEND extra_cmake_args "-DQT_HOST_PATH_CMAKE_DIR=${QT_HOST_PATH_CMAKE_DIR}")
    endif()

    if(CMAKE_MAKE_PROGRAM)
        list(APPEND extra_cmake_args "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")
    endif()
...
```
- Add the following lines:
```
    if(ANDROID_PLATFORM)
        list(APPEND extra_cmake_args "-DANDROID_PLATFORM=${ANDROID_PLATFORM}")
    endif()

```
- Save the file and exit the editor.
