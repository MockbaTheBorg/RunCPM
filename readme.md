# RunCPM

RunCPM is a 32 bits application which allows you to execute old CP/M 8 bits programs on Windows, Mac OS X, Linux, FreeBSD and Arduino DUE.
If you miss powerful programs like Wordstar, dBaseII, Mbasic and others, then RunCPM is for you.

RunCPM builds on Visual Studio 2013 or later. Posix builds use gcc/llvm.

## Arduino

RunCPM builds on Arduino 1.6.6 or later.
RunCPM so far runs only on the Arduino DUE. It requires a fair amount of RAM to run (64K used to be a lot in those days).
RunCPM needs a SDFat library. I am using the one from https://github.com/greiman/SdFat
RunCPM needs a SD (or microSD) card shield.
The file SdFatConfig.h of SdFat must be changed so: #define USE_LONG_FILE_NAMES 0 (if it is 1, CPMDuino will fail if the folder contains files with long names)

Arduino digital and analog read/write added by Krzysztof Klis via BDOS calls (see cpm.h file for details).

## Building

Just run "./configure" and then "make". Your system will be automatically detected by the build scripts.
For Posix systems (Linux, FreeBSD, Mac OS X) the ncurses library is required.

## Getting Started

Just drop the RunCPM executable on a folder and create subfolders on that one called "A", "B", "C" ... these will be you "disk drives".
In Arduino just format a SD card and create subfolders on it called "A", "B", "C" ... these will be your "disk drives".
There's no need to create clumsy disk images. Just put your CP/M programs and files inside these subfolders, which are seen as drive letters "A:", "B:", until "P:".

RunCPM needs a binary copy of the CP/M CCP to be on the same folder as RunCPM.exe. The one provided is CPM22.bin, which is CPM 2.2. Others may work as well, your mileage may vary.

