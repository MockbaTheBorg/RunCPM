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

#include "globals.h"

/*
		abstraction.h - Adds all system dependent calls and definitions needed by RunCPM
		This should be the only file modified for portability. Any other file
		shoud be kept the same.
*/
#ifdef __DJGPP
	#include "abstract.h"	// DOS (DJGPP) needs 8.3 naming convention
#else
#ifdef _WIN32
	#include "abstraction_vstudio.h"
#else
	#include "abstraction_posix.h"
#endif
#endif

/*
		cpu.h - Implements the emulated CPU
*/
#include "cpu.h"

/*
		ram.h - Implements the RAM
*/
#include "ram.h"

/*
		console.h - Defines all the console abstraction functions
*/
#include "console.h"

/*
		disk.h - Defines all the disk access abstraction functions
*/
#include "disk.h"

/*
		cpm.h - Defines the CPM structures and calls
*/
#include "cpm.h"

int main(int argc, char* argv[])
{
	FILE* file;

	_console_init();
	_clrscr();
	_puts("CP/M 2.2 Emulator v" VERSION " by Marcelo Dantas.\r\n");
	_puts("      Build " __DATE__ " - " __TIME__ "\r\n");
	_puts("-----------------------------------------\r\n");

	_RamFill(0, 0x10000, 0);	// Clears the memory

	while (TRUE) {
		file = _sys_fopen_r((uint8*)CCPname);
		if (file == NULL) {
			_puts("\r\nCan't open CCP!\r\n");
			break;
		} else {
			//**********  Boot code  **********//
			_puts("\r\nRunCPM Version " VERSION " (CP/M 2.2 64K)\r\n");
			_RamLoad(file, CCPaddr);	// Loads the CCP binary file into memory
			_sys_fclose(file);

			_PatchCPM();	// Patches the CP/M entry points and other things in

			Z80reset();			// Resets the Z80 CPU
			SET_LOW_REGISTER(BC, _RamRead(0x0004));	// Sets C to the current drive/user

			PC = CCPaddr;		// Sets CP/M application jump point

			Z80run();			// Start simulation
			if (Status == 1)	// This is set by a call to BIOS 0 - ends CP/M
				break;
		}
	}

	_console_reset();
	_puts("\r\n");
	return(0);
}

#endif
