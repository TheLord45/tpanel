# Build for IOS

**TPanel** can be build also for IOS (iPad, iPhone). There exists a file called `tpanel.pro`. This file contains the instructions for the `qmake` tool. It is meant to be used out of _QtCreator_. Start the _QtCreator_ and open this file as a project.

## Prerequisits

For IOS you need a Mac with *XCode* installed. You need also a developer account and a signature of a developer team if you want to compile the code for a real iPhone or iPad. For details look at [Apple developer](https://developer.apple.com/tutorials/app-dev-training).

To be able to build **TPanel** for IOS you need the following libraries compiled for IOS and MacOS. For MacOS you may need them for the architectures ARM as well as Intel. For IOS you may need versions for 64 bit and 32 bit. It depends on what devices you have.

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

Start `QtCreator` and load the project file `tpanel.pro`. You must set some things to have the correct paths. Go to the build settings of the project. Expand the **Build steps** and enter into the line **Additional arguments** the following definitions:

- `OS=[IOS|IOSsim|macos]`
- `EXT_LIB_PATH=<path/to/precompiled/files>`

To make clear what this means I give you an example. We assume that the precompiled libraries (Skia, PjSIP) are installed in your home directory (`/Users/<user>/SDKs`). We want to create an executable for the IOS simulator. Then we must set the _Additional arguments_ to:

`OS=IOS EXT_LIB_PATH=/Users/<user>/SDKs`

Now check the _ABIs_ you want to compile for (only IOS). With this settings you should be able to compile **TPanel** for IOS.
In case you're building for a real iPhone or iPad you must select in the section **iOS settings** the **development team**.
In the section **Build environment** check **Clear system environment**. Then append to line **PATH**: `:/usr/bin`

Now you should be able to compile **TPanel** for IOS.
