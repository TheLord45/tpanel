# TPanel
**TPanel** is an emulation of some AMX G4 touch panels. The panels used to verify the communication protocol and the behavior were an *AMX MVP-5200i* and an *AMX NXD-700Vi*.

**TPanel** was designed for *NIX desktops (Linux, BSD, …) as well as Android operating systems version 10 or newer. Currently there exists no Windows version and there probably never will.

The software uses internally the [Skia](https://skia.org) library for drawing all objects and the [Qt 5.15](https://doc.qt.io/qt-5.15/) library to display the objects. **TPanel** is written in C++. This makes it especially on mobile platforms fast and reliable. It has the advantage to not drain the battery of any mobile device while running as fast as possible. Compared to commercial products the battery lasts up to 10 times longer.

# Full documentation
Look at the full documentation in this repository. You'll will find the [reference manual](https://github.com/TheLord45/tpanel/tree/main/documentation) in three different formats:
* [PDF](https://github.com/TheLord45/tpanel/blob/main/documentation/ReferenceGuide.pdf)
* [ODT](https://github.com/TheLord45/tpanel/blob/main/documentation/ReferenceGuide.odt)
* [HTML](https://github.com/TheLord45/tpanel/blob/main/documentation/ReferenceGuide.html)

# How to compile
## Compile for Linux desktop
First download the source into a directory. Then enter the directory and type the following commands.

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install

To be able to compile the source you need the following credentials installed. Make sure you've installed the developper files of them.
- Skia
  - Download and compile the source from [Skia](https://skia.org)
- Qt 5.15 or Qt 6.3.x (only for desktop!)
- pjsip
  - Download and compile the source from [pjsip](https://www.pjsip.org)
- Expat
- Freetype

Apart from the fact that Skia and maybe pjsip is not part of your Linux distribution, the other dependencies are (for most distributions). But for Android you must cross compile them. Look at the home pages of this projects to find out how.

If everything compiled successfull and installed, you should create a configuration file. Look at the [reference guide](https://github.com/TheLord45/tpanel/tree/main/documentation) of how to do this.

## Compile for Android
For Android we need the [Android SDK](https://developer.android.com/). In case you've not intstalled it, do it now. Additionaly you need the following libraries compiled for Android:

- [Skia](https://skia.org)
- [Qt 5.15](https://doc.qt.io/qt-5/)
- openssl (is part of NDK)
- [pjsip](https://www.pjsip.org)

You can download a ready package with all the necessary libraries and header files from my [server](https://www.theosys.at/download/android_dist.tar.bz2) (~ 1Gb!). After you've downloaded this hugh file, unpack it into any directory. The file details are:

- [`android_dist.tar.bz2`](https://www.theosys.at/download/android_dist.tar.bz2)
- SHA256: `eb7f474aec318bec4af6052ac23232296fa9a9c43f0bf2640f771781d6afb1fc`

I developped **TPanel** with [QTCreator](https://www.qt.io/product/development-tools). Therefor I would recoment to use this tool to compile **TPanel**. All the following descriptions are for this tool. The easiest way to install it is by using the [Qt maintenance tool](https://www.qt.io/download).

Start `QTCreator` and load the project file `tpanel.pro`. You must set some things to have the correct paths. Go to the build settings of the project. Define the path where you like the binary for Android, the APK-file, to be. Expand the **Build steps** and enter into the line **Additional arguments** the following definitions:

- `SDK_PATH=</path/to/your/android/sdk>`
- `EXT_LIB_PATH=</path/to/the/distribution>` (The self compiled libraries or the distribution downloaded from my [server](https://www.theosys.at/download/android_dist.tar.bz2) )
- `EXTRA_PATH=</path/to/android/sdk/extras>`

To make clear what this means I give you an example. We assume that the Android SDK was installed into the directory `/usr/share/android-sdk-linux`. The distribution from my server (the file containing all necessary binaries for Android) was unpacked into `/home/user/distribution`. Then the line **Additional arguments** must look like:

    SDK_PATH=/usr/share/android-sdk-linux EXT_LIB_PATH=/home/user/distribution EXTRA_PATH=/usr/share/android-sdk-linux/extras

For **Build environment** check **Clear system environment**. Then click on **Details** to expand this section. With the example above in mind make sure that the following elements are there.

    ANDROID_HOME           /usr/share/android-sdk-linux
    ANDROID_NDK_HOST       linux-x86_64
    ANDROID_NDK_PLATTFORM  android-29
    ANDROID_NDK_ROOT       /usr/share/android-sdk-linux/ndk/23.1.7779620
    ANDROID_SDK_ROOT       /usr/share/android-sdk-linux
    JAVA_HOME              /usr/lib/jvm/java-11-openjdk-amd64
    PATH                   /usr/lib/jvm/java-11-openjdk-amd64/bin:/home/user/distribution/qt/bin:/usr/share/android-sdk-linux/ndk/23.1.7779620/toolchains/llvm/prebuilt/linux-x86_64/bin:/bin:/usr/bin
    QTDIR                  /home/user/distribution/qt

Your Java distribution may be in a different location. Set the variable `JAVA_HOME` to the location on your system. Don't forget to set this path also for the variable `PATH`.

It is possible that you want to use another ndk version then *23.1.7779620*. At the time of writing this documentation, this was the latest one available supported by Qt 5.15. If you want to use a newer version make sure your Qt version supports it.

I wrote the Java part for Android to work with Android 10. I use some API function available with Android 10 and above for getting the battery state and the network state. This is the reason why I set `ANDROID_NDK_PLATTFORM` to `android-29`. It is up to you to use a newer API.

With this parameters set you should be able to start the compiler. When everything finished you should find the Android binary in the **Build directory** at `android-build/build/outputs/apk/debug`.
