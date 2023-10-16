# Build for IOS

**TPanel** can be build also for IOS (iPad, iPhone). The _cmake_ configuration contains the necessary options. The easiest way to do this is to use _QtCreator_. Start the _QtCreator_ and open the _cmake_ file `CMakeLists.txt` as a project.

On a Mac with Apple silicon, set _arm64_ as the base architecture (x86_64 is default).

## Prerequisits

For IOS you need a Mac with *XCode* installed. You need also a developer account and a signature of a developer team if you want to compile the code for a real iPhone or iPad. For details look at [Apple developer](https://developer.apple.com/tutorials/app-dev-training).

To be able to build **TPanel** for IOS you need the following libraries compiled for IOS and MacOS. For MacOS you may need them for the architectures ARM as well as Intel. For IOS you may need versions for 64 bit and 32 bit (from iOS version 17.0 on the 32Bit is no longer supported). It depends on what devices you have.

- Skia
- PjSIP
- pcre
- openssl (version 1.1 for Qt prior to 6.5.0 and version 3 for newer Qt)

You can (cross) compile them yourself or download the [SDK](https://www.theosys.at/download/SDKs.tar.bz2) (~3.5Gb!) from my machine. I recommend you to download the SDK. This saves you a lot of time. The following description assumes you've installed the _SDK_ in the following directory structure:
```
SDKs
 +-openssl
 | +-include
 | +-IOS
 | +-IOSsim
 | +-lib
 +-pcre
 | +-include
 | +-lib
 +-pjsip
 | +-include
 | +-lib
 +-skia
   +-IOS32
   +-IOS64
   +-IOSsim
   +-IOSsim-apple
   +-IOSsim-intel
```

**File: SDKs.tar.bz2**

- Download: [SDKs.tar.bz2](https://www.theosys.at/download/SDKs.tar.bz2)
- SHA256 hash: `333a0bad139705c2b48c6e8ce37da959e9ce8e3284638f83d605464505463f98`

## Build with QtCreator

Start `QtCreator` and load the project file `CMakeLists.txt`. If you're on a Mac with Apple silicon (arm64 M1, M2 or newer) then you must set `CMAKE_OSX_ARCHITECTURES` to `arm64` (default: `x86_64`).

In case you have unzipped the file `SDKs.tar.bz2` in another directory then the _HOME_-directory, you must specify the path. Add the variable `EXT_LIB_PATH` to the _Current configurations_ of _cmake_. Set it to the path where you've unzipped it.

Now you should be able to compile **TPanel** for IOS.

## Using the script `build_ios.sh`

The repository comes with a shell script called `build_ios.sh`. With it you can compile **TPanel** on the command line. The script offers some parameters:
```
build_ios.sh [clean] [debug] [sign] [id <ID>] [help|--help|-h]
   clean     Delete old build, if there is one, and start a new clean build.
   debug     Create a binary with debugging enabled.
   sign      Sign the resulting app.
   id <ID>   The signing identity (team ID)
```

For example, start the script as follows:

`./build_ios.sh clean sign`

to build **TPanel** for an iPhone or iPad. If you've problems and get errors, open the script with an editor and adapt the paths and settings according to your needs.
