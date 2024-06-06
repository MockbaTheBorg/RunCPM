# RunCPM - Z80 CP/M emulator

(It is important to read this documentation fully before attempting to build/use RunCPM)

RunCPM is an application which can execute vintage CP/M 8 bits programs on many modern platforms, like Windows, Mac OS X, Linux, FreeBSD, Arduino DUE and variants, like the Teensy or ESP32. It can be built both on 32 and 64 bits host environments and should be easily portable to other platforms.<br>
RunCPM is fully written in C and in a modular way, so porting to other platforms should be only a matter of writing an abstraction layer file for it. No modification to the main code modules should be necessary.

If you miss using powerful programs like Wordstar, dBaseII, mBasic and others, then RunCPM is for you. It is very stable and fun to use.<br>
RunCPM emulates CP/M from Digital Research as close as possible, the only difference being that it uses regular folders on the host instead of disk images.

RunCPM builds on Visual Studio 2013 or later. Posix builds use GCC/LLVM. It can also be built on the Arduino IDE. It can be built also on Cygwin (posix) and Mingw. Makefiles are provided with the distribution.

## Why RunCPM?

RunCPM was written to serve as a test environment when I was restoring the only copy of Z80 MicroMumps v4.06 which exists online (https://github.com/MockbaTheBorg/MicroMumps).<br>
Making changes, recompiling MicroMumps and loading it onto a regular CP/M emulator via a disk image every time I moved a bit forward on the restoration was becoming too time consuming. So I decided to write something that would allow me to run the executable right away after making any modifications.<br>
RunCPM then evolved as more and more CP/M applications were added to its compatibility list.

## Arduino / Teensy / ESP32 / STM32

RunCPM builds on Arduino IDE 1.8.7 or later.<br>
RunCPM so far runs on the Arduino DUE, on the Teensy 3.5 and up, on the ESP32 and on some STM32, as it requires a fair amount of RAM to run (64K used to be a lot back in those days).<br>
If using the Arduino DUE, RunCPM also needs a SD (or microSD) card shield to place the CP/M files in. The Teensy has an on-board microSD adapter. Some ESP32 and STM32 boards may need external SD card adapters.

Arduino digital and analog read/write support was added by Krzysztof Kliś via extra non-standard BDOS calls (see the bottom of cpm.h file for details).

LED blink codes: Arduino/Teensy/ESP32/STM32 user LED will blink fast when RunCPM is waiting for a serial connection and will send two repeating short blinks when RunCPM has exited (CPU halted). Other than that the user LED will indicate disk activity.

RunCPM needs A LOT of RAM and Flash memory by Arduino standards, so it will NOT run on other Arduinos than the DUE (not the Duemilanove) as they will not have enough of those.
It is theoretically possible to run it on an Arduino which has enough Flash (at least 96K) by adding external RAM to it via some shield, but this is untested, probably slow and would require an entirely different port of RunCPM code.

When using Arduino boards, the serial speed as well as other parameters, may be set by editing the RunCPM.ino sketch. The default serial speed is 9600 for compatibility with vintage terminals.<br>

If building for the Teensy, ESP32 and STM32, please read the entire document, as there is more information below.

You will also need to set the correct board definition. To do this, look at line 9 in RunCPM.ino: `#include "hardware/due.h"`

In the Hardware folder, you will find additional board definitions. If your board is not listed, copy the board that's closest to yours
and update the SDINIT macro with your SD card reader's CS pin. If your reader will not operate at 50MHz (many won't), you can change SDMHZ
to the correct speed for your card. 25 has been known to work with SD card readers on the Due, and you may need to set this slower for 
certain cards or readers.

Also, read **SdFat library change** below.


## Experimental Platforms

Atmel SAM4S Xplained Pro.<br>
Altera/Intel DE1 Cyclone II FPGA (NiosII).

## Building

RunCPM builds natively on Visual Studio.

For building on other systems run `make yyy build`, where "yyy" is:

* **macosx** - Mac OS X,
* **mingw** - when building with MinGW under Windows,
* **posix** - when building under Linux, FreeBSD etc,
* **tdm** - when building with TDM-GCC under Windows.

For Linux and FreeBSD the "ncurses.h" header file (and library) is required and needs to be installed prior to building RunCPM. The name of the package varies depending on the Linux distribution. Some are named "ncurses-dev", some "ncurses-devel", so Google is your friend for that.<br>
On Mac OS X, install it using "brew install ncurses".<br>
The "readline.h" header file is also required on Linux/FreeBSD. On some linuxes it is called "libreadline-dev".<br>

All makefile options:
* `make yyy build`: Compiles RunCPM.  Only recompiles changed files.
* `make yyy clean`: Deletes all object files.  Doesn't recompile.
* `make yyy rebuild`: Performs clean, then build.  Required when alternating compiling for different platforms.

Anyone having issues building with homebrew 'binutils' on Mac should check this issue: https://github.com/MockbaTheBorg/RunCPM/issues/136<br>

## Getting Started

**Preparing the RunCPM folder :**<br>
Create a folder containing both the RunCPM executable and the CCP binaries for the system. CCP Binaries for 64K and 60K are provided.<br>
If using a SD card, RunCPM and its CCPs need to be on the SD card's root folder.<br>
The 64K version CCPs will provide the maximum amount of memory possible to CP/M applications, but its addressing ranges are unrealistic in terms of emulating a real CP/M computer.<br>
The 60K version CCPs will provide a more realistic addressing space, by keeping the CCP entry point on the same loading address it would be on a physical CP/M computer.<br>
Other amounts of memory can be used, but this would require rebuilding the CCP binaries (sources available on disk A.ZIP).
The CCP binaries are named with their file name extensions matching the amount of memory they run on, so for example, DRI's CCP runnin on 60K memory would be named CCP-DR.60K. RunCPM looks for the file accordingly, depending on the amount of memory selected when it is built.<br>
**IMPORTANT NOTE** - Starting on version 3.4, regardless of the amount of memory allocated to the CP/M system, RunCPM will still allocate 64K of RAM on the host so the BIOS is always at the same starting position. This favors the porting of even more different CCP codes to RunCPM. This also requires that, starting on version 3.4, new copies of the master disk A.ZIP, ZCPR2 CCP and ZCPR3 CCP (all provided here) are used.

**Preparing the CP/M virtual drives :**<br>
**VERY IMPORTANT NOTE** - Starting on RunCPM version 3.7 user areas are now mandatory. The support of non-user-area disk folders has been dropped between versions 3.5 and 3.6, so if you are running on a version up to 3.5, please consider moving to 3.7 or higher but not before updating your disk folders structure to match the support of user areas.<br><br>

RunCPM emulates the CP/M disks and user areas by means of subfolders under the RunCPM executable location, to prepare a folder or SD card for running RunCPM use the following procedures:<br>
Create subfolders under where the RunCPM executable is located and name them "A", "B", "C" and so on, for each disk drive you intend to use, each one of these folders will be one disk drive, and under folder "A" create a subfolder named "0". This is the user area 0 of disk A:, extract the contents of A.ZIP package into this "0" subfolder. Switching to another user area inside CP/M will automatically create the respective user area subfolders, "1", "2", "3" ... as they are selected. Subfolders for the user areas 10 to 15 are created as letters "A" to "F".

All the letters for folders/subfolders and file names should be kept in uppercase, to avoid any issues of case-sensitive filesystems compatibility.
CP/M only supported 16 disk drives: A: to P:, so creating other letters above P won't work, same goes for user areas above 15 (F).

**Available CCPs :**<br>
RunCPM can run on its internal CCP or using binary CCPs from real CP/M computers. A few CCPs are provided:

* **CCP-DR** - Is the original CCP from Digital Research.<br>
* **CCP-CCPZ** - Is the Z80 CCP from RLC and others.<br>
* **CCP-ZCP2** - Is the original ZCPR2 CCP modification.<br>
* **CCP-ZCP3** - Is the original ZCPR3 CCP modification.<br>
* **CCP-Z80** - Is the Z80CCP CCP modification, also from RLC and others.<br>

These CCPs are provided with their source code on the A.ZIP package, and can be natively rebuilt if needed.<br>
SUBMIT (.SUB) files are provided to allow for rebuilding the CCPs and some of the RunCPM utilities.<br>
Other CCPs may be adapted to work, and if succeeding, please share it so we can add it to here.

The default choice for CCP is the internal one in RunCPM. If you want to use a different CCP, you must do two things:<br>
**1 -** Change the selected CCP in globals.h (in the RunCPM folder). Find the lines that show:
 
> /* Definition of which CCP to use (must define only one) */<br> 
> \#define CCP_INTERNAL	// If this is defined, an internal CCP will emulated<br>
> //#define CCP\_DR<br>
> //#define CCP\_CCPZ<br> 
> //#define CCP\_ZCPR2<br> 
> //#define CCP\_ZCPR3<br> 
> //#define CCP\_Z80<br>

Comment out the CCP_INTERNAL line by inserting two slashes at the line's beginning. Then remove the two slashes at the start of the line containing the name of the CCP you intend to use. Save the file.<br>

**2 -** Copy a matching CCP from the CCP folder to the folder that holds your A folder. Each CCP selection will have two external CCP's, one for 60K and another for 64K. If you have already built the executable, you will need to do it again.

Anytime you wish to change the CCP, you must repeat these steps and rebuild.

**IMPORTANT NOTE** - CCP-Z80 expects the $$$.SUB to be created on the currently logged drive/user, so when using it, use SUBMITD.COM instead of SUBMIT.COM when starting SUBMIT jobs.

**Contents of the "master" disk (A.ZIP) :**<br>
The master disk A.ZIP contains a basic initial CP/M environment, with the source code for the CCPs and also the **EXIT** program, which ends RunCPM execution.<br>
There is also a **FORMAT** program which creates a drive folder as if it was formatting a disk. It does nothing to pre-existing drive folders, so it is very safe to use.<br>
Disks created by **FORMAT** cannot be removed from inside RunCPM itself, if needed, it must be done manually by accessing the RunCPM folder or SD Card on a host machine and removing the disk drive folder.<br>
The master disk also contains **Z80ASM**, which is a very powerful Z80 assembly that generates .COM files directly.<br>
Other CP/M applications which were not part of the official DRI's distribution are also provided to improve the RunCPM experience. These applications are listed on the 1STREAD.ME file.

## Automation

If a single line text file named AUTOEXEC.TXT containing a CP/M command up to 125 characters long is placed onto the same folder as the RunCPM executable, the command on this file will be loaded onto the CCP buffer, emulating the patch of the CCP sector on a real CP/M disk.<br>
This command will then be executed every time the CCP is restarted or once when RunCPM loads, which is configurable in the globals.h header file.

## Printing

Printing to the PUN: and LST: devices is allowed and will generate files called "PUN.TXT" and "LST.TXT" under user area 0 of disk A:. These files can then be tranferred over to a host computer via XMODEM for real physical printing.
These files are created when the first printing occurs, and will be kept open throughout RunCPM usage. They can be erased inside CP/M to trigger the start of a new printing.
As of now RunCPM does not support printing to physical devices.

## Lua Scripting Support (Deprecated)

The internal CCP can be built with support for Lua scripting.<br>
Lua scripts can be written on the CP/M environment using any text editor and then executed as if they were CP/M extrinsic commands.<br>

The order of execution on the internal CCP when an extrinsic command is typed with no explicit drive is:<br>
* The command with extension .COM is searched on the current drive.<br>
* If not found it is searched on drive A: user area 0.<br>
* If not found it is searched on the current drive user area 0.<br>
* If not found then use .LUA extension instead and repeat the above search.

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
 2: BC
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
The disk A.ZIP contains an example script called LUAINFO.LUA, with the same functionality of INFO.COM, which provides information about RunCPM.

Caveat: Lua scripts must have a comment (--) on their last line, to prevent issues with the CP/M ^Z end-of-file character when the scripts are created with CP/M text editors. The comment on the last line comments out the CP/M EOF (^Z) character and prevents Lua interpreter errors.

UPDATE: Lua support has been removed, it can still be built in by manually modifying the Makefiles or renaming the RunCPM.vcxproj and RunCPM.vcxproj.filters files. However, it will eventually be gone forever. Maybe one day from now, maybe ten years from now, who knows.

## Limitations / Misbehaviors

The objective of RunCPM is **not** to emulate a Z80 CP/M computer perfectly, but to allow CP/M to be emulated as close as possible while keeping its files on the native (host) filesystem.<br>
This will obviously prevent the accurate physical emulation of disk drives, so applications like **MOVCPM** and **STAT** will not be useful.<br>
They are still provided on the master disk A.ZIP just to maintain compatibility with DRI's official CP/M distribution.

At the moment only CP/M 2.2 is fully supported. Support for CP/M 3.0 is in the works.

The IN/OUT instructions are used to implement the communication between the soft CPU BIOS and BDOS and the equivalent RunCPM functions, therefore these instructions are unusable for any other purpose.

The "video monitor" is assumed to be ANSI/VT100 emulation, as this is the standard for DOS/Windows/Linux distributions. So CP/M applications which are hardcoded for other terminals won't build their screens correctly.<br>
When using a serial terminal emulator, make sure it sends either CR or LF when you press enter, not both (CR+LF), or else it will break the DIR listing on DR's CCP. This is standard CP/M 2.2 behavior.

RunCPM does not support making files read-only or any other CP/M attributes. All the files will be visible and R/W all the time, so be careful. It supports making "disks" read-only though, but only from RunCPM's perspective. The R/O attributes of the disk's containing folder are not modified.

Lua scripting is not supported on platforms other than Windows, Linux and MacOS. There is no Lua support for Arduino based platforms yet. (see deprecation comment above)

Some applications, like hi-tech C for example, will try to access user areas higher than 15 to verify if they are running on a different CP/M flavor than 2.2. This causes the generation of user areas with letters higher than F. This is an expected behavior and won't be "fixed".

There is a hardware-design-error on some Clones and some Revisions of the original
Arduino Due which prevents the correct use of the RX0-Pin (serial TTL-Port Receive-Pin)
while not using the USB-Programming-Port for accessing RunCPM on the Arduino Due.
The solution is to use the serial port 1 = RX1 and TX1
For that you have to replace all occurrences of "Serial." with "Serial1."
https://github.com/MockbaTheBorg/RunCPM/issues/117 - @Guidol70

## CP/M Software

I have shared my entire CP/M software library [here!](https://drive.google.com/drive/folders/11WIu8rD_7pIDaET7dqTeA73CvX0jkxz2?usp=sharing)<br>
Please, if you have newer/cleaner (closer to the original distribution) versions of the software found here, please consider sending it over so I can update.<br>
Same goes for user guides and other useful documentation.<br>
My intent is to keep here clean copies of useful software which has been tested and proven to work fine with RunCPM.

## Online contact/support

https://discord.gg/WTTWVZ6 - I have created this discord channel to be able to (eventually) meet people in real time and talk about RunCPM. Feel free to join.

## References

https://weblambdazero.blogspot.com/2016/07/cpm-on-stick_16.html<br>
https://lehwalder.wordpress.com/2017/04/17/cpm-auf-dem-arduino-due-mit-runcpm-bzw-cpmdunino/ - in German<br>
https://hackaday.io/project/19560-z80-cpm-computer-using-an-arduino<br>
http://www.chstercius.cz/runcpm/<br>
https://hackaday.com/2014/12/30/z80-cpm-and-fat-file-formats/<br>
https://ubuntuforum-br.org/index.php?topic=120787.0 - in Portuguese<br>
http://mrwrightteacher.net/retrochallenge2018/<br>
https://learn.adafruit.com/z80-cpm-emulator-for-the-samd51-grand-central<br>
https://forum.armbian.com/topic/8569-runcpm-on-armbian-cpm-weekend-fun/<br>
https://blog.hackster.io/z80-cp-m-emulator-runs-on-adafruits-new-grand-central-dev-board-6a28ad73dfbc<br>
https://ht-deko.com/arduino/runcpm.html - in Japanese<br>
https://www.instructables.com/Retro-CPM-Stand-Alone-Emulator/<br>
https://www.hackster.io/news/kian-ryan-s-tiny-2040-build-could-be-the-smallest-cp-m-microcomputer-in-history-5d8eeab29d29<br>

## RunCPM compatible boards tested so far

https://store.arduino.cc/usa/arduino-due<br>
https://www.pjrc.com/store/teensy35.html<br>
https://www.pjrc.com/store/teensy36.html<br>
https://www.pjrc.com/store/teensy40.html - With TEENSY4_AUDIO board<br>
https://www.pjrc.com/store/teensy41.html<br>
https://github.com/LilyGO/ESP32-TTGO-T1 - LED on pin 22<br>
https://wiki.wemos.cc/products:lolin32:lolin32_pro - LED on pin 5 (inverted)<br>
https://docs.zerynth.com/latest/official/board.zerynth.doit_esp32/docs/index.html - LED on pin 2<br>
https://docs.espressif.com/projects/esp-idf/en/latest/get-started/get-started-pico-kit.html - No user LED on board<br>
https://www.st.com/en/evaluation-tools/stm32f4discovery.html - SD on Software SPI mode (mounted on EB-STM32F4DISCOVERY-LCD)<br>
https://www.adafruit.com/product/4064<br>
https://github.com/SmartArduino/SZDOITWiKi/wiki/ESP8266---ESPduino-32<br>
https://shop.pimoroni.com/products/tiny-2040?variant=39560012234835<br>

I have seen reports of people being able to run it on the Nucleo F401re, Nucleo F411 and a japanese board called GR-SAKURA.
It was also successfully built and ran on the Kindle Keyboard 3G.

## Building dependencies

For the Arduino DUE, support for it needs to be added through the board manager.<br>
For the Teensy follow the instructions from here: https://www.pjrc.com/teensy/td_download.html<br>
For the ESP32 follow the instructions from here: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/<br>
For the STM32 follow the instructions from here: https://github.com/stm32duino/Arduino_Core_STM32<br>
All boards now use the SdFat 2.x library, from here: https://github.com/greiman/SdFat/<br>
All Arduino libraries can be found here: https://www.arduinolibraries.info/

## SdFat library change

If you get a <b>'File' has no member named 'dirEntry'</b> error, then a modification is needed on the SdFat Library SdFatConfig.h file (line 78 as of version 2.0.2) changing:<br>
```#define SDFAT_FILE_TYPE 3```<br>
to<br>
```#define SDFAT_FILE_TYPE 1```<br>
As file type 1 is required for most of the RunCPM ports.

To find your libraries folder, open the Preferences in Arduino IDE and look at the Sketchbook location field. 

On Windows systems, SdFatConfig.h will be in Documents\Arduino\libraries\SdFat\src


## ESP32 Limitations

The ESP32 build doesn't yet support the analogWrite BDOS call.<br>
The ESP32 build may require additional changes to the code to support different ESP32 boards.<br>
The ESP32 build uses SPI mode for accessing the SD card.<br>

## STM32 Limitations

The STM32 build uses Soft SPI mode for accessing the SD card.<br>

## Dedications

I dedicate this software to:<br>
* *Krzysztof Kliś* -  For writing the Arduino hardware interface and helping with debugging/testing.<br>
* *Yeti* - For being an awesome agitator since RunCPM went on GitHub.<br>
* *Wayne Hortensius* - For going deep into the code and bringing even more CP/M 2.2 compatibility to disk routines.<br>
* All the other folks who are actively helping with finding/fixing bugs and issues on Github - thanks guys.<br>

I dedicate it also to the memory of some awesome people who unfortunately are not among us anymore:<br>
* *Mr. Gary Kildall* - For devising an operating system which definitely played a role on changing the world.<br>
* *Mr. Jon Saxton* - For finding the very first RunCPM bug back in 2014.<br>
* *Dr. Richard Walters* - For writing the Z80 version of mumps, one of the most useful and fun languages I had the joy to use.<br>
* *Mr. Tom L. Burnett* - For helping me with testing/debugging many different CP/M 2.2 applications on RunCPM.<br>

May the computers in heaven be all 8-bit.<br>

## Donations

Last but not least, if you feel like make a donation to help the project moving forward, feel free to do so by clicking [here!](http://paypal.me/MockbaTheBorg)<br>
These will help me source more hardware and other things that assist on the RunCPM development.
<hr>
###### The original copy of this readme file was written on WordStar 3.3 under RunCPM
Tor  6 Jun 2024 23:34:14 CEST
