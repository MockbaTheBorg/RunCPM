/*
		RunCPM - Execute 8bit CP/M applications on modern computers
		Copyright (c) 2016 - Marcelo Dantas

		Extensive debugging/testing by Tom L. Burnett
		Debugging/testing and new features by Krzysztof Klis
		DOS and Posix ports added by Krzysztof Klis

		Best operating system ever by Gary Kildall, 40 years ago
		I was 9 at that time and had no idea what a computer was
*/
#ifndef ARDUINO

/* globals.h must be the first file included - it defines the bare essentials */
#include "globals.h"

/* Any system specific includes should go here - this will define system functions used by the abstraction */

/* all the RunCPM includes must go after the system specific includes */

/*
abstraction.h - Adds all system dependent calls and definitions needed by RunCPM
This should be the only file modified for portability. Any other file
shoud be kept the same.
*/

#ifdef _WIN32
#include "abstraction_vstudio.h"
#else
  #ifdef ARDUINO
    #include "abstraction_arduino.h"
  #else
    #ifdef __DJGPP
      #include "abstract.h"	// DOS (DJGPP) needs 8.3 naming convention
    #else
      #include "abstraction_posix.h"
    #endif
  #endif
#endif

#include "ram.h"		// ram.h - Implements the RAM
#include "console.h"	// console.h - Defines all the console abstraction functions
#include "cpu.h"		// cpu.h - Implements the emulated CPU
#include "disk.h"		// disk.h - Defines all the disk access abstraction functions
#include "cpm.h"		// cpm.h - Defines the CPM structures and calls
#ifdef CCP_INTERNAL
#include "ccp.h"		// ccp.h - Defines a simple internal CCP
#endif

int main(int argc, char *argv[]) {

#ifdef DEBUGLOG
	_sys_deletefile((uint8*)LogName);
#endif

	_console_init();
	_clrscr();
	_puts("CP/M 2.2 Emulator v" VERSION " by Marcelo Dantas\r\n");
	_puts("      Build " __DATE__ " - " __TIME__ "\r\n");
#ifdef HASLUA
	_puts("       with Lua scripting support\r\n");
#endif
	_puts("-----------------------------------------\r\n");
	_puts("CCP: " CCPname "  CCP Address: 0x");
	_puthex16(CCPaddr);
	_puts("\r\n");

	while (TRUE) {
#ifdef CCP_INTERNAL
		_PatchCPM();
		Status = 0;
		_ccp();
		if (Status == 1)	// This is set by a call to BIOS 0 - ends CP/M
			break;
#else
		if(! _sys_exists((uint8*)CCPname)) {
			_puts("\r\nCan't open CCP!\r\n");
			break;
		} else {
			//**********  Boot code  **********//
			_puts(CCPHEAD);
			_RamLoad((uint8*)CCPname, CCPaddr);	// Loads the CCP binary file into memory
			_PatchCPM();	// Patches the CP/M entry points and other things in


			Z80reset();			// Resets the Z80 CPU
			SET_LOW_REGISTER(BC, _RamRead(0x0004));	// Sets C to the current drive/user
			PC = CCPaddr;		// Sets CP/M application jump point
			Z80run();			// Starts simulation
			if (Status == 1)	// This is set by a call to BIOS 0 - ends CP/M
				break;
		}
#endif
	}

	_console_reset();
	_puts("\r\n");
	return(0);
}

#endif
