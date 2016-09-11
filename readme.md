# RunCPM

RunCPM is a 32 bits application which allows you to execute old CP/M 8 bits programs on Windows, Mac OS X, Linux, FreeBSD, MS-DOS and Arduino DUE.
If you miss powerful programs like Wordstar, dBaseII, Mbasic and others, then RunCPM is for you.
RunCPM emulates CP/M 2.2 from Digital Research as close as possible.

RunCPM builds on Visual Studio 2013 or later. Posix builds use gcc/llvm.

## Arduino

RunCPM builds on Arduino 1.6.6 or later.
RunCPM so far runs only on the Arduino DUE, as it requires a fair amount of RAM to run (64K used to be a lot in those days).
RunCPM needs a SDFat library. I am using the one from https://github.com/greiman/SdFat
The file SdFatConfig.h of SdFat must be changed so: #define USE_LONG_FILE_NAMES 0 (if it is 1, RunCPM will fail if the folder contains files with long names)
RunCPM also needs a SD (or microSD) card shield to put the CP/M files in.

Arduino digital and analog read/write support was added by Krzysztof Klis via BDOS calls (see cpm.h file for details).

## Building

Run "make -f Makfile.yyy", where "yyy" is:

* *dos* - when building with DJGPP under MS-DOS,
* *mingw* - when building with MinGW (or TDM-GCC) under Windows,
* *posix* - when building under Linux, FreeBSD, Mac OS X, etc.

For Linux and FreeBSD the ncurses-dev package is required. In Mac OS X install it using "brew install ncurses".

Windows version also builds with Visual Studio.

## Getting Started

Just drop the RunCPM executable onto a folder and create subfolders under that one called "A", "B", "C" ... these will be your "disk drives".
In Arduino just format a SD card and create subfolders on it called "A", "B", "C" ... these will be your "disk drives".
There's no need to create clumsy disk images. Just put your CP/M programs and files inside these subfolders, which are seen as drive letters "A:", "B:", until "P:". Other letter above P won't work as CP/M only supported a maximum of 16 disk drives.

RunCPM needs a binary copy of the CP/M CCP to be on the same folder as RunCPM.exe. The one provided is CCP-DR.bin, copyright 1979 from Digital Research. Other CCPs may work as well, your mileage may vary. ZCPR2, for example, is also included.

The disk A.ZIP contains a basic initial CP/M environment, with the source code for the CCP-DR.BIN and also the EXIT.COM, which ends RunCPM execution. To use its contents just extract the .ZIP archive into an "A" subfolder under RunCPM.

This disk also contains Z80ASM, which is a very powerful Z80 assembly that generates .COM files directly.
