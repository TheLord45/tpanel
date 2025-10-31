# Build Android APK

There are 2 possibilities to create an Android APK file. You can use the ready
script on the command line to build it, or use _QtCreator_. Both methods are
based on _cmake_. The following description shows both methods and explain what
you need and how to put the things together.

Developing for another platform means to _cross compile_ code. This is necessary
because there is no way (not that I know) to create the program directly on the
target platform. And even if it would be a nightmere to develop a complex
program like **TPanel** directly on an Android phone.

> This documentation assumes that you're creating **TPanel** on a Linux or Mac system.

## Prerequisites

We need the [Android SDK](https://developer.android.com/). In case you've not
installed it, do it now. I recommend to install the SDK into your home
directory. Usually this is installed in the directory `<home directory>/Android/Sdk`.
Additionally you need the following libraries compiled for Android:

- [Skia](https://skia.org)
- [Qt 6.10.0](https://doc.qt.io/qt-6/) (newer libs may work but may need different configuration!)
- - Older Qt versions are still supported, but you must adapt the file `src/android/build.cradle`
    and the URL in `src/android/gradle/wrapper/gradle-wrapper.properties`!
- openssl (can easily be installed out of QtCreator)
- [pjsip](https://www.pjsip.org)

> _Hint_: For Qt version less then 6.5.x you need openssl 1.1, while for newer
versions of Qt you need openssl 3!

I made an archive file containing _Skia_ and _PjSIP_ as precompiled versions for
all 4 possible Android architectures (arm64-v8a, armeabi-v7a, x86_64, x86) You
can download it from my [server](https://www.theosys.at/download/android_dist.tar.gz) (~ 1Gb!).
After you've downloaded this hugh file, unpack it into any directory. The file
details are:

- [`android_dist.tar.gz`](https://www.theosys.at/download/android_dist.tar.gz)
- SHA256: `3f4deeeb7db60b0864c57e068b4e8917d614ee1fa7e7694189cf7fdbfb0379b8`

> The package does **not** include the Qt framework!

## Build with script `build_android.sh`

This is a shell script to create an Android APK file. Such files can be deployed
to any Android device running Android 11 or newer. This is API level 30.

First copy the file `build_android.sh` to another name e.g.:

    cp build_android.sh buildAndroid.sh

Now open the copy with a text editor. The first lines defining some paths.
Adapt the settings to the paths of your installation. Then simply start the
script on the command line. If everything was installed and all paths were set
correct, it should compile the code and produce an APK file. The last line will
give you the path and name of the produced file.

The script contains a help function (`build_android.sh --help`). It shows the
possible parameters and their meaning:

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

There are some reasons why it is better to use [QtCreator](https://www.qt.io/product/development-tools)
then using the command line. The most important reason is if you want to be able
to flexible deploy the application to different Android devices. And if you want
to participate in developing you should use this IDE also.

After starting _QtCreator_ click on `Open project` and open the file
`CMakeLists.txt` in the source folder of **TPanel**. When the project is open,
click on `Projects` on left panel. Select `Android Qt 6.10.0 CLang x86_64` 
(or a newer Qt version) on Intel based machines and `arm64-v8a` on ARM based
machines, from the list and click on `Build` (hammer symbol). Now you see the
_Build Settings_. Under `CMake` set in the _Initial Configuration_ the value
of `ANDROID_PLATFORM` to `android-30`. Make sure the NDK version `27.2.12479018`
is set (`ANDROID_NDK` and `CMAKE_C_COMPILER`). If not, hit `Manage Kits...`
and make this version the default or install it and make it then the default.
I will not explain how to install the Android SDK or an NDK, which is part of
the Android SDK. This is subject to the Google documentation of Android Studio.

> **Note**: *QtCreator* supports the installation of the Android SDK out of the box
> and this has the advantage that everything you need will be there, but nothing
> else.

Click now on `Re-configure with Initial Parameters` (on the bottom of the
`Cmake` box.). If everything went well, the tab should jump to 
`Current Configuration`. If you want to build a version running on a real device
you must select the architecture `arm64-v8a` and plug a phone or tablet to one
of your USB plugs.

If you want to compile for an Android emulator it depends on the CPU of your
developer machine. On all Intel based machines select the architecture `x86_64`
and on the ARM base the architecture `arm64-v8a`. Set this to the variable
`QT_ANDROID_ABIS`. 

> _Hint_: If you want to compile sources in parallel (much faster) you can add
the parameter `-j<#cpus>` (replace `<#cpus>` with the number of CPUs your
system offers) to `Additional CMake options:`.

Before you can start compiling, click on the phone symbol in the left bottom
corner and select `tpanel` under _Run_. Now you can compile the program by
clicking on the hammer symbol.

> _Hint_: You must not create a multi architecture file any more. Since it it
very unlikely that there are real devices with an architecture other then
`arm64-v8a` it is not necessary. This is also true for any emulator. You just
create an APK file with the architecture the emulator supports.

### QtCreator settings for Qt 6.5.3

The following table shows the settings I used to create an APK file:

|Section|Description|
|-------|-----------|
|Distribution|Android 8.0 (API 26) to 15 (Api 35)|
|Architecture|arm64-v8, x86_64, x86, and armeabi-v7a|
|Compiler|Clang 14.0.6, Clang 17.0.2 and Clang 18.0.1 (NDK r25b, r26b and r27c or **25.1.8937393**, 26.1.10909125 and 27.2.12479018)<br/><pre><b><i>Note</b>:</i> It's recommended that Qt apps use the same NDK version used for building the official Qt for Android libraries to avoid missing symbol errors. In releases supporting multiple NDKs, the newest supported NDK is used for building Qt.</pre>|
|JDK|JDK 17|
|Gradle|Gradle 8.10 and AGP 8.5.2 --> Version: 7.4.1<br/><b>Hint:</b> Set the Gradle version in file `build.gradle`.|
|Package|Multi-ABI APKs and AABs|
|Configuration|In `QtCreator`: ANDROID_PLATFORM: android-30<br/>Android build-tools version: 34.0.0<br/>Android build platform SDK: android-34|

### QtCreator settings for Qt 6.8.3

The following table shows the settings I used to create an APK file:

|Section|Description|
|-------|-----------|
|Distribution|Android 9.0 (API 28) to 15 (Api 35)|
|Architecture|arm64-v8, x86_64, x86, and armeabi-v7a|
|Compiler|Clang 17.0.2 (NDK r26b and r27c or 26.1.10909125 and 27.2.12479018)<br/><pre><b><i>Note</b>:</i> It's recommended that Qt apps use the same NDK version used for building the official Qt for Android libraries to avoid missing symbol errors. In releases supporting multiple NDKs, the newest supported NDK is used for building Qt.</pre>|
|JDK|JDK 17|
|Gradle|Gradle 8.10 and AGP 8.6.0 --> Version: 8.6.0<br/><b>Hint:</b> Set the Gradle version in file `build.gradle`.|
|Package|Multi-ABI APKs and AABs|
|Configuration|In `QtCreator`: ANDROID_PLATFORM: android-30<br/>Android build-tools version: 34.0.0<br/>Android build platform SDK: android-34|

### QtCreator settings for Qt 6.9.3

The following table shows the settings I used to create an APK file:

|Section|Description|
|-------|-----------|
|Distribution|Android 9.0 (API 28) to 15 (Api 35)|
|Architecture|arm64-v8, x86_64, x86, and armeabi-v7a|
|Compiler|Clang 17.0.2 (NDK r26b and r27c or 26.1.10909125 and 27.2.12479018)<br/><pre><b><i>Note</b>:</i> It's recommended that Qt apps use the same NDK version used for building the official Qt for Android libraries to avoid missing symbol errors. In releases supporting multiple NDKs, the newest supported NDK is used for building Qt.</pre>|
|JDK|JDK 17|
|Gradle|Gradle 8.10 and AGP 8.6.0 --> Version: 8.6.0<br/><b>Hint:</b> Set the Gradle version in file `build.gradle`.|
|Package|Multi-ABI APKs and AABs|
|Configuration|In `QtCreator`: ANDROID_PLATFORM: android-30<br/>Android build-tools version: 34.0.0<br/>Android build platform SDK: android-34|

### QtCreator settings for Qt 6.10.x

The following table shows the settings I used to create an APK file:

|Section|Description|
|-------|-----------|
|Distribution|Android 9.0 (API 28) to 15 (Api 35)|
|Architecture|arm64-v8, x86_64, x86, and armeabi-v7a|
|Compiler|Clang 17.0.2 (NDK r26b and r27c or 26.1.10909125 and 27.2.12479018)<br/><pre><b><i>Note</b>:</i> It's recommended that Qt apps use the same NDK version used for building the official Qt for Android libraries to avoid missing symbol errors. In releases supporting multiple NDKs, the newest supported NDK is used for building Qt.</pre>|
|JDK|JDK 17|
|Gradle|Gradle 8.14.2 and AGP 8.10.1 --> Version: 8.8.0<br/><b>Hint:</b> Set the Gradle version in file `build.gradle`.|
|Package|Multi-ABI APKs and AABs|
|Configuration|In `QtCreator`: ANDROID_PLATFORM: android-30<br/>Android build-tools version: 35.0.1<br/>Android build platform SDK: android-36|
