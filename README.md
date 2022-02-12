# TPanel
TPanel is an emulation of some AMX G4 touch panels. The panels used to reverse engineer the communication protocol and the behavior were a *AMX MVP-5200i* and a *AMX NXD-700Vi*.

TPanel was designed for *NIX desktops (Linux, BSD, …) as well as Android operating systems version 10 or newer. Currently there exists no Windows version and there probably never will be one.

The software uses internally the [Skia](https://skia.org) library for drawing all objects and the [Qt 5.15](https://doc.qt.io/qt-5.15/) library to display the objects. TPanel is written in C++. This makes it especially on mobile platforms fast and reliable. It has the advantage to not drain the accumulator of any mobile device while running as fast as possible. Compared to commercial products the accumulator lasts up to 10 times as longer.

# Full documentation
Look at the full documentation in this repository. You'll will find the [reference manual](https://github.com/TheLord45/tpanel/tree/main/documentation) in three different formats:
* [PDF](https://github.com/TheLord45/tpanel/blob/main/documentation/ReferenceGuide.pdf)
* [ODT](https://github.com/TheLord45/tpanel/blob/main/documentation/ReferenceGuide.odt)
* [HTML](https://github.com/TheLord45/tpanel/blob/main/documentation/ReferenceGuide.html)
