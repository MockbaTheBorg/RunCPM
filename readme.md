# RunCPM - Z80 CP/M 2.2 emulator

RunCPM is an application which allows you to execute old CP/M 8 bits programs on Windows, Mac OS X, Linux, FreeBSD, MS-DOS and Arduino DUE. It can be built both on 32 and 64 bits host environments and should be easily portable to other platforms.<br>
RunCPM is fully written in C and in a modular way, so porting to other platforms should be only a matter of writing an abstraction layer file for it. No modification of the main code modules should be necessary.

If you miss powerful programs like Wordstar, dBaseII, MBASIC and others, then RunCPM is for you. It is very stable and fun to use.<br>
RunCPM emulates CP/M 2.2 from Digital Research as close as possible.

RunCPM builds on Visual Studio 2013 or later. Posix builds use GCC/LLVM. It can also be built on the Arduino IDE and even on DJGPP for DOS. It can be built also on Cygwin (posix) and Mingw. Makefiles are provided with the distribution.

## Arduino

RunCPM builds on Arduino IDE 1.6.6 or later.<br>
RunCPM so far runs on the Arduino DUE and on the Teensy 3.5 and 3.6, as it requires a fair amount of RAM to run (64K used to be a lot back in those days).<br>
If using the Arduino DUE, RunCPM also needs a SD (or microSD) card shield to place the CP/M files in. The Teensy has on on-board SD adapter.

Arduino digital and analog read/write support was added by Krzysztof Kliś via BDOS calls (see the bottom of cpm.h file for details).

LED blink codes: Arduino LED (pin 13) will blink fast when RunCPM is waiting for a serial connection and will send two repeating short blinks when RunCPM has exited (CPU halted). 

## Building

RunCPM builds natively on Visual Studio

Run "make yyy", where "yyy" is:

* **dos** - when building with DJGPP under MS-DOS,
* **mingw** - when building with MinGW under Windows,
* **posix** - when building under Linux, FreeBSD, Mac OS X, etc,
* **tdm** - when building with TDM-GCC under Windows.

For Linux and FreeBSD the ncurses-dev package is required. On Mac OS X, install it using "brew install ncurses".

## Getting Started

Create a folder containing both the RunCPM executable and the CCP binaries for the system. CCP Binaries for 64K and 60K are provided.<br>
The 64K version provides the maximum amount of memory possible to CP/M application, but its addressing ranges are unrealistic in terms of emulating a real CP/M computer.<br>
The 60K version provides a more realistic addressing space, keeping the CCP on the same loading address it would be on a similar CP/M computer.<br>
Other amounts of memory can be used, but this would require rebuilding the CCP binaries (available on disk A.ZIP).
The CCP binaries are named with their extensions being the amount of memory they run on, so for example, DRI's CCP runnin on 60K memory would be named CCP-DR.60K. RunCPM looks for the file accordingly depending on the amount of memory selected when it is built.

RunCPM can emulate the user areas as well (this is the default), so to create the disk drives use the following procedures:

* **when using user areas** - Create subfolders under the executable folder named "A", "B", "C" and so on, for each disk drive you intend to use, each one of these folders will be one disk drive, and under folder "A" create a subfolder named "0". This is the user area 0 of disk A:, extract the contents of A.ZIP package into this "0" subfolder. The "user" command will automatically create other user area subfolders, "1", "2", "3" ... as they are selected. Subfolders for the users 10 to 15 are created as letters "A" to "F".
* **when not using user areas** - Create subfolders under the executable folder named "A", "B", "C" and so on, for each disk drive you intend to use, each one of these folders will be one disk drive, and only user 0 is available. The "user" command to select user areas will be ignored. Extract the contents of the A.ZIP package into the "A" subfolder.

All the letters for folders/subfolders and file names should be kept in uppercase, to avoid any issues of case-sensitive filesystems compatibility.
CP/M only supported 16 disk drives: A: to P:, so creating other letters above P won't work, same goes for user areas above 15 (F).

RunCPM can run on its internal CCP (beta) or using binary CCPs from real CP/M computers. A few CCPs are provided:

* **CCP-DR** - Is the original CCP from Digital Research.<br>
* **CCP-CCPZ** - Is the Z80 CCP from RLC and others.<br>
* **CCP-ZCP2** - Is the original ZCPR2 CCP modification.<br>
* **CCP-ZCP3** - Is the original ZCPR3 CCP modification.<br>
* **CCP-Z80** - Is the Z80CCP CCP modification, also from RLC and others.<br>

These CCPs are provided with their source code on the A.ZIP package, and can be natively rebuilt if needed.<br>
Other CCPs may be adapted to work, and if succeeding, please share it so we can add it to here.

The disk A.ZIP contains a basic initial CP/M environment, with the source code for the CCPs and also the **EXIT** program, which ends RunCPM execution.<br>
This disk also contains **Z80ASM**, which is a very powerful Z80 assembly that generates .COM files directly.
Other CP/M applications which were not part of the official DRI's distribution are also provided to improve the RunCPM experience.

## Lua Scripting Support

The internal CCP can be built with support for Lua scripting.<br>
Lua scripts can be written on the CP/M environment using any text editor and then executed as if they were CP/M extrinsic commands.<br>

The order of execution when an extrinsic command is typed with no explicit drive is:<br>
* The command with extension .COM is searched on the current drive.<br>
* If not found it is searched on drive A: user area 0.<br>
* If not found it is searched on the current drive user area 0.<br>
* If not found then use .LUA extension and repeat the search.

Lua scripts have access to these functions:
* **BdosCall(C, DE)** - **C** is the number of the function to call and **DE** the parameter to pass.<br>
  The C and DE CPU registers are loaded accordingly and the BDOS function if called.<br>
  The function returns the contents of the HL register upon returning from the BDOS call.
* **RamRead(addr)** - **addr** is the memory address to read from, the function returns a byte.
* **RamWrite(addr, v)** - **addr** is the memory address to write **v** to. **v** must be a byte.
* **RamRead16(addr)** - **addr** is the memory address to read from, the function returns a 16 bit word.
* **RamWrite16(addr, v)** - **addr** is the memory address to write **v** to. **v** must be a 16 bit word.
* **ReadReg(reg)** - **reg** is the 16 bit CPU register to read from, the function returns a 16 bit word.
* **WriteReg(reg, v)** - **reg** is the CPU register to write to. **v** must be a 16 bit word.
  Extra care must be taken when willing to replace only part of the 16 bit register.<br>

The **ReadReg** and **WriteReg** functions refer to the CPU registers as index values.
The possible values for **reg** on those functions are:<br>
```
 0: PCX - External view of PC
 1: AF
 2: CC
 3: DE
 4: HL
 5: IX
 6: IY
 7: PC
 8: SP
 9: AF'
10: BC'
11: DE'
12: HL'
13: IFF - Interrupt Flip Flop
14: IR - Interrupt (upper) / Refresh (lower) register
```
The disk A.ZIP contains a script called LUAINFO.LUA, which replicates the functionality of INFO.Z80, which provides information about RunCPM.

Caveat: Lua scripts must have a comment (--) on their last line, to prevent issues with the CP/M ^Z end-of-file character. The comment on the last line comments out the CP/M EOF (^Z) character and prevents Lua errors.

## Limitations

The objective of RunCPM is **not** to emulate a Z80 CP/M computer perfectly, but to allow CP/M to be emulated as close as possible while keeping its files on the native (host) filesystem.<br>
This will obviously prevent the accurate physical emulation of disk drives, so applications like **MOVCPM** and **STAT** will not be useful.<br>
They are provided on disk A.ZIP just to maintain compatibility with DRI's official distribution.

Other CP/M flavors like CP/M 3 or CP/M Plus are not supported, as the emulated BDOS is specific for CP/M 2.2.

The "video monitor" is assumed to be ANSI/VT100, as this is the standard for DOS/Windows/Linux distributions. So CP/M applications which are hardcoded for other terminals won't build their screens correctly.

RunCPM does not support making files read-only or any other CP/M attributes. All the files will be visible and R/W all the time, so be careful. It supports making "disks" read-only though, but only during RunCPM's execution. The R/O attributes of the containing folder are not modified.

RunCPM does not support any CP/M device other than CON: and will silently fail to access them.

<hr>
###### The original copy of this readme file was written on WordStar 3.3 under RunCPM
