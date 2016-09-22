# RunCPM - Z80 CP/M 2.2 emulator

RunCPM is an application which allows you to execute old CP/M 8 bits programs on Windows, Mac OS X, Linux, FreeBSD, MS-DOS and Arduino DUE. It can be built both on 32 and 64 bits host environments and should be easily portable to other platforms.<br>
RunCPM is fully written in C and in a modular way, so porting to other platforms should be only a matter of writing an abstraction layer file for it. No modification of the main code modules would be necessary.

If you miss powerful programs like Wordstar, dBaseII, Mbasic and others, then RunCPM is for you. It is very stable and fun to use.<br>
RunCPM emulates CP/M 2.2 from Digital Research as close as possible.

RunCPM builds on Visual Studio 2013 or later. Posix builds use gcc/llvm. It can also be built on the Arduino IDE and even on DJGPP for DOS.

## Arduino

RunCPM builds on Arduino 1.6.6 or later.<br>
RunCPM so far runs only on the Arduino DUE, as it requires a fair amount of RAM to run (64K used to be a lot back in those days).<br>
RunCPM needs a SDFat library. I am using the one from [Bill Greiman](https://github.com/greiman/SdFat/).<br>
The file SdFatConfig.h of SdFat must be changed to: *#define USE_LONG_FILE_NAMES 0* (if left at 1, RunCPM will fail if the folder contains files with long names).<br>
RunCPM also needs a SD (or microSD) card shield to place the CP/M files in.

Arduino digital and analog read/write support was added by Krzysztof Kliś via BDOS calls (see the bottom of cpm.h file for details).

## Building

RunCPM builds natively on Visual Studio

Run "make -f Makefile.yyy", where "yyy" is:

* *dos* - when building with DJGPP under MS-DOS,
* *mingw* - when building with MinGW under Windows,
* *tdm* - when building with TDM-GCC under Windows,
* *posix* - when building under Linux, FreeBSD, Mac OS X, etc.

For Linux and FreeBSD the ncurses-dev package is required. On Mac OS X, install it using "brew install ncurses".<br>
Cygwin can be used to build the posix implementation on Windows machines.

## Getting Started

Create a folder to contain the RunCPM executable. On that folder goes both the executable and the CCP binaries for the system. CCP Binaries for 64K and 60K are provided.<br>
The 64K version provides the maximum amount of memory possible to CP/M application, but its addressing ranges are unrealistic in terms of emulating a real CP/M computer.<br>
The 60K version provides a more realistic addressing space, keeping the CCP on the same loading address it would be on a similar CP/M computer.<br>
Other amounts of memory can be used, but this would require rebuilding the CCP binaries (available on Disk A.ZIP).

RunCPM can emulate the user areas as well (this is the default), so to create the disk drives use the following procedures:

* *when not using user areas* - Create subfolders under the executable folder named "A", "B", "C" and so on, for each disk drive you intend to use, each one of these folders will be one disk drive, and only user 0 is available. The "user" command to select user areas will be ignored. Extract the contents of the A.ZIP package into the "A" subfolder.
* *when using user areas* - Create subfolders under the executable folder named "A", "B", "C" and so on, for each disk drive you intend to use, each one of these folders will be one disk drive, and under folder "A" create a subfolder named "0". This is the user area 0 of disk A:, extract the contents of A.ZIP package into this "0" subfolder. The "user" command will automatically create other user area subfolders, "1", "2", "3" ... as they are selected. Subfolders for the users 10 to 15 are created as letters "A" to "F".

All the letters for folders/subfolders and file names should be kept in uppercase, to avoid any issues of case-sensitive filesystems compatibility.
CP/M only supported 16 disk drives: A: to P:, so creating other letters above P won't work, same goes for user areas above 15 (F).

RunCPM can run on its internal CCP (beta) or using binary CCPs from real CP/M computers. Two CCPs are provided:

* *CCP-DR.bin* - Is the original CCP from Digital Research.<br>
* *CCP-ZCPR.bin* - Is the original ZCPR2 ccp modification.

Both CCPs are provided as source code on the A.ZIP package, and can be natively rebuilt if needed.<br>
Other CCPs may be adapted to work, and if succeeding, please share it so we can add it to here.

The disk A.ZIP contains a basic initial CP/M environment, with the source code for the CCPs and also the EXIT.COM program, which ends RunCPM execution.<br>
This disk also contains Z80ASM, which is a very powerful Z80 assembly that generates .COM files directly.
Other CP/M applications which were not part of the official DRI's distribution are also provided to improve the RunCPM experience.

## Limitations

The objective of RunCPM is *not* to emulate a Z80 CP/M computer perfectly, but to allow CP/M to be emulated as close as possible while keeping the files on the native (host) filesystem.<br>
This will obviusly prevent the accurate physical emulation of disk drives, so applications like *MOVCPM* and *STAT* will not be completely useful.<br>
They are provided on Disk A.ZIP just to keep compatibility with DRI's official distribution.

Other CP/M flavors like CP/M 3 or CP/M Plus are not supported, as the emulated BDOS is specific for CP/M 2.2.

The "video monitor" is assumed to be VT100, as this is the standard for DOS/Windows/Linux distributions. So CP/M applications which are hardcoded for other terminals won't build their screens correctly.

<hr>
###### The original copy of this readme file was written on WordStar 3.3 under RunCPM
