/*
		RunCPM - Execute 8bit CP/M applications on modern computers
		Copyright (c) 2014 - Marcelo Dantas
		*/

#include "globals.h"
/*
		abstraction.h - Adds all system dependent calls and definitions needed by RunCPM
		This should be the only file modified for portability. Any other file
		shoud be kept the same.
		Defines:
		------------------------------------------------ CPU abstraction
		Includes the soft-cpu module
		------------------------------------------------ Memory abstraction
		RAMSIZE - Always 64k
		RAM[] - Ram memory array
		_RamRead - Reads a byte from the RAM
		_RamWrite - Writes a byte to the RAM
		------------------------------------------------ Filesystem (disk) abstraction
		_fopen_r
		_fopen_w
		_fopen_rw
		_fseek
		_ftell
		_fread
		_fwrite
		_feof
		_fclose
		_remove
		_rename
		------------------------------------------------ Console abstraction
		console includes - Things like conio.h
		_console_init - Initializes console
		_console_reset - Resets console (prior to ending execution)
		_kbhit - Checks if keyboard hit
		_getch - Gets character (blocking - no echo)
		_getche - Gets character (blocking - echo)
		_putch - Puts character (native)
		_clrscr - Clears the console screen

		*/
#include "abstraction_vstudio.h"

/*
		cpu.h - Implements the emulated CPU
		Defines:
		CPU - The emulated CPU
		*/
#include "cpu.h"
/*
		ram.h - Implements the RAM
		Defines:
		_RamFill - Fills the memory with a value
		_RamCopy - Copies memory from one addr to another
		_RamLoad - Loads a file to the RAM memory
		*/
#include "ram.h"

/*
		console.h - Defines all the console abstraction functions
		Defines:
		_chready - Checks if keyboard hit (CP/M mode)
		_getchNB - Gets character (non blocking - no echo)
		_putcon - Puts character with VT100 translation (if needed)
		_puts - Puts a string (0x00 terminated)
		_puthex8 - Puts a XX hex character
		_puthex16 - Puts a XXXX hex word
		*/
#include "console.h"

/*
		disk.h - Defines all the disk access abstraction functions
		Defines:
		CPM_FCB - Structure defining a FCB
		CPM_DIR - Structure defining a directory entry
		_error - Prints disk access errors (FCB errors)
		_FCBtoDIR - Creates a mock DIR antry onto the current dmaAddr
		_GetFile - Gets file name from the FCB
		_SetFile - Sets filename on the FCB
		_OpenFile - Opens CP/M file
		_CloseFile - Closes CP/M file
		_MakeFile - Creates CP/M file
		_DeleteFile - Deletes CP/M file
		_RenameFile - Renames CP/M file
		_SearchFirst - Searches first file matching FCB
		_SearchNext - Searches next file matching FCB
		_ReadSeq - Reads file sequentially
		_WriteSeq - Writes file sequentially
		_ReadRand - Reads file randomly
		_WriteRand - Writes file randomly
		*/
#include "disk.h"

/*
		cpm.h - Defines the CPM structures and calls
		Defines:
		_PatchCPM - Patches CP/M entry points into the system memory
		_PatchCMD - Patches the command line parameters into 0x0080 (Initial dmaAddr)
		_Bios - Executes a CPM BDOS call
		_Bdos - Executes a CPM BDOS call
		*/
#include "cpm.h"

int main(int argc, char* argv[])
{
	FILE* file;

	_console_init();
	_clrscr();
	_puts("CP/M 2.2 Emulator v1.9 by Marcelo Dantas.\n");
	_puts("CP/M debugging/testing by Tom L. Burnett.\n");
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
			_RamLoad(file, CCPaddr);	// Loads the binary file into memory
			_fclose(file);

			_PatchCPM();	// Patches the CP/M entry points in

			Z80reset();	// Resets the CPU
			SET_LOW_REGISTER(BC, drive[0] - 'A');

			PC = CCPaddr;	// Sets CP/M application jump point
			Z80run();	// Start simulation
			if (Status == 1)	// Makes a call to BIOS 0 end RunCPM
				break;
		}
	}

	_console_reset();

	return(0);
}