/*
		RunCPM - Execute 8bit CP/M applications on modern computers
		Copyright (c) 2016 - Marcelo Dantas

		Extensive debugging/testing by Tom L. Burnett
		Debugging/testing and new features by Krzysztof Klis
		DOS and Posix ports added by Krzysztof Klis

		Best operating system ever by Gary Kildall, 40 years ago
		I was 9 at that time and had no idea what a computer was
*/

// Only build this code if not on Arduino
#ifndef ARDUINO

/* globals.h must be the first file included - it defines the bare essentials */
#include "globals.h"

/* Any system specific includes should go here - this will define system functions used by the abstraction */

/* all the RunCPM includes must go after the system specific includes */

/*
abstraction.h - Adds all system dependent calls and definitions needed by RunCPM
This should be the only file modified for portability. Any other file
should be kept the same.
*/

#ifdef _WIN32
#include "abstraction_vstudio.h"
#else
#include "abstraction_posix.h"
#endif

// AUX: device configuration
#ifdef USE_PUN
FILE* pun_dev;
int pun_open = FALSE;
#endif

// PRT: device configuration
#ifdef USE_LST
FILE* lst_dev;
int lst_open = FALSE;
#endif

#include "ram.h"		// ram.h - Implements the RAM
#include "console.h"	// console.h - Defines all the console abstraction functions
#include "cpu.h"		// cpu.h - Implements the emulated CPU
#include "disk.h"		// disk.h - Defines all the disk access abstraction functions
#include "host.h"		// host.h - Custom host-specific BDOS call
#include "cpm.h"		// cpm.h - Defines the CPM structures and calls
#ifdef CCP_INTERNAL
#include "ccp.h"		// ccp.h - Defines a simple internal CCP
#endif

int main(int argc, char* argv[]) {

#ifdef DEBUGLOG
	_sys_deletefile((uint8*)LogName);
#endif

	_host_init(argc, &argv[0]);
#ifdef STREAMIO
	_streamioInit();
#endif
	_console_init();
	_clrscr();
	_puts("  CP/M Emulator v" VERSION " by Marcelo Dantas\r\n");
	_puts("      Built " __DATE__ " - " __TIME__ "\r\n");
#ifdef HASLUA
	_puts("       with Lua scripting support\r\n");
#endif
	_puts("----------------------------------------\r\n");
	_puts("BIOS at 0x");
	_puthex16(BIOSjmppage);
	_puts(" - ");
	_puts("BDOS at 0x");
	_puthex16(BDOSjmppage);
	_puts("\r\n");
	_puts("CCP " CCPname " at 0x");
	_puthex16(CCPaddr);
	_puts("\r\n");
#if BANKS > 1
	_puts("Banked Memory: ");
	_puthex8(BANKS);
	_puts(" banks\r\n");
#endif

	while (TRUE) {
		_puts(CCPHEAD);
		_PatchCPM();		// Patches the CP/M entry points and other things in
		Status = 0;
#ifdef CCP_INTERNAL
		_ccp();
#else
		if (!_sys_exists((uint8*)CCPname)) {
			_puts("Unable to load CP/M CCP.\r\nCPU halted.\r\n");
			break;
		}
		_RamLoad((uint8*)CCPname, CCPaddr);	// Loads the CCP binary file into memory

		// Loads an autoexec file if it exists and this is the first boot
		// The file contents are loaded at ccpAddr+8 up to 126 bytes then the size loaded is stored at ccpAddr+7
		if (firstBoot) {
			if (_sys_exists((uint8*)AUTOEXEC)) {
				uint16 cmd = CCPaddr + 8;
				uint8 bytesread = (uint8)_RamLoadSz((uint8*)AUTOEXEC, cmd, 125);
				uint8 blen = 0;
				while (blen < bytesread && _RamRead(cmd + blen) > 31)
					blen++;
				_RamWrite(cmd + blen, 0x00);
				_RamWrite(--cmd, blen);
			}
			if (BOOTONLY)
				firstBoot = FALSE;
		}

		Z80reset();			// Resets the Z80 CPU
		SET_LOW_REGISTER(BC, _RamRead(DSKByte));	// Sets C to the current drive/user
		PC = CCPaddr;		// Sets CP/M application jump point
		Z80run();			// Starts simulation
#endif
		if (Status == 1)	// This is set by a call to BIOS 0 - ends CP/M
			break;
#ifdef USE_PUN
		if (pun_dev)
			_sys_fflush(pun_dev);
#endif
#ifdef USE_LST
		if (lst_dev)
			_sys_fflush(lst_dev);
#endif
	}

	_puts("\r\n");
	_console_reset();
#ifdef STREAMIO
	_streamioReset();
#endif
	return(0);
}

#endif
