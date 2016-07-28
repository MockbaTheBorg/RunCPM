/*
		RunCPM - Execute 8bit CP/M applications on modern computers
		Copyright (c) 2016 - Marcelo Dantas

		Extensive debugging/testing by Tom L. Burnett
		Debugging/testing and new features by Krzysztof Klis
*/

#include "globals.h"

/*
		abstraction.h - Adds all system dependent calls and definitions needed by RunCPM
		This should be the only file modified for portability. Any other file
		shoud be kept the same.
*/
#include "abstraction_vstudio.h"

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
	_puts("CP/M 2.2 Emulator v2.0 by Marcelo Dantas.\n");
	_puts("-----------------------------------------\n");

	_RamFill(0, 0x10000, 0);	// Clears the memory

	while (TRUE) {
		file = _fopen_r((uint8*)CCPname);
		if (file == NULL) {
			_puts("\nCan't open CCP!\n");
			break;
		} else {
			//**********  Boot code  **********//
			_puts("\n64k CP/M Vers 2.2\n");
			_RamLoad(file, CCPaddr);	// Loads the CCP binary file into memory
			_fclose(file);

			if(!Status)
				_PatchCPM();	// Patches the CP/M entry points and other things in

			Z80reset();			// Resets the Z80 CPU
			SET_LOW_REGISTER(BC, _RamRead(0x0004));

			PC = CCPaddr;		// Sets CP/M application jump point
			Z80run();			// Start simulation
			if (Status == 1)	// This is set by a call to BIOS 0 - ends CP/M
				break;
		}
	}

	_console_reset();

	return(0);
}