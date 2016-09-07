#ifndef CCP_H
#define CCP_H

#include <string.h>

// CP/M BDOS calls
#define C_WRITE			2
#define C_READSTR		10
#define DRV_ALLRESET	13
#define DRV_SET			14
#define F_OPEN			15
#define F_CLOSE			16
#define F_DELETE		19
#define F_READ			20
#define DRV_GET			25
#define F_DMAOFF		26

#define CmdFCB	BatchFCB + 36		// FCB for use by internal commands
#define ParFCB	0x005C				// FCB for use by line parameters

#define inBuf	BDOSjmppage - 256	// Input buffer location
#define cLen	0x7f				// Maximum size of a command line

#define defDMA	0x0080				// Default DMA address
#define defLoad	0x0100				// Default load address

// CCP global variables
uint8 curDrive;	// 0 -> 15 = A -> P	.. Current drive for the CCP (same as RAM[0x0004]
uint8 cmdDrive;	// 0 -> 15 = A -> P .. Drive to read the (external) command from
uint8 parDrive;	// 0 -> 15 = A -> P .. Drive for the first file parameter
uint8 prompt[5] = "\r\n >";
uint8 *pbuf, *pcmd, *ppar;			// Pointers for extracting commands and parameters from command line buffer
uint8 blen;							// Actual size of the typed command line (size of the buffer)
uint8 clen;							// Actual size of the command in characters
uint8 plen;							// Actual size ot the first file parameter in characters
uint8 command[9] = "";				// Buffer for the external command filename

static const char *Commands[] =
{
	// Standard CP/M commands
	"DIR",
	"ERA",
	"TYPE",
	"SAVE",
	"REN",
	"USER",
	// Extra CCP commands
	"DEL",
	"INFO",
	"EXIT",
	NULL
};

// Used to call BDOS from inside the CCP
uint8 _ccp_bdos(uint8 function, uint16 de, uint8 a) {
	SET_LOW_REGISTER(BC, function);
	DE = de;
	SET_HIGH_REGISTER(AF, a);
	_Bdos();
	return(HIGH_REGISTER(AF));
}

// Gets the command ID number
uint8 _ccp_cnum(void) {
	uint8 i = 0;
	uint8 result = 255;

	if (!RAM[CmdFCB]) {	// If a drive is set, then the command is external
		while (Commands[i]) {
			if (strcmp((char*)command, Commands[i]) == 0)
				result = i; break;
			i++;
		}
	}
	return(result);
}

// Returns true if character is a separator
uint8 _ccp_delim(uint8 ch) {
	return(ch == ' ' || ch == '=' || ch == '.' || ch == ';');
}

// Prints the FCB filename
void _ccp_printfcb(uint16 address, uint8 compact) {
	uint8 i, ch;

	for (i = 1; i < 12; i++) {
		ch = RAM[address + i];
		if (ch == ' ' && compact)
			continue;
		if (i == 9)
			_ccp_bdos(C_WRITE, compact ? '.' : ' ', 0x00);
		_ccp_bdos(C_WRITE, ch, 0x00);
	}
}

// Initializes the FCB
void _ccp_initFCB(uint16 address) {
	uint8 i;

	for (i = 0; i < 36; i++)
		_RamWrite(address + i, 0x00);
	for (i = 0; i < 11; i++) {
		_RamWrite(address + 1 + i, 0x20);
		_RamWrite(address + 17 + i, 0x20);
	}
}

// DIR command
void _ccp_dir(void) {
	uint8 i;
	uint8 dirHead[6] = "A: ";
	uint8 dirSep[4] = " : ";
	uint32 fcount = 0;	// Number of files printed
	uint32 ccount = 0;	// Number of columns printed

	if (!plen) {
		for (i = 1; i < 12; i++)
			RAM[ParFCB + i] = '?';
	}

	dirHead[0] = parDrive + 'A';
	if (!_SearchFirst(ParFCB, TRUE)) {
		_puts((char*)dirHead);
		_ccp_printfcb(tmpFCB, FALSE);
		fcount++; ccount++;
		while (!_SearchNext(ParFCB, TRUE)) {
			if (!ccount) {
				_puts("\r\n");
				_puts((char*)dirHead);
			} else {
				_puts((char*)dirSep);
			}
			_ccp_printfcb(tmpFCB, FALSE);
			fcount++; ccount++;
			if (ccount > 3)
				ccount = 0;
		}
	} else {
		_puts("No file");
	}
}

// ERA command
void _ccp_era(void) {
	if (_ccp_bdos(F_DELETE, ParFCB, 0x00))
		_puts("No file");
}

// TYPE command
void _ccp_type(void) {
	uint8 i, c;
	uint16 a;

	if (!_ccp_bdos(F_OPEN, ParFCB, 0x00)) {
		while (!_ccp_bdos(F_READ, ParFCB, 0x00)) {
			i = 128;
			a = dmaAddr;
			while (i) {
				c = _RamRead(a);
				if (c == 0x1A)
					break;
				_ccp_bdos(C_WRITE, c, 0x00);
				i--; a++;
			}
		}
	} else {
		_ccp_printfcb(ParFCB, TRUE);
		_puts("?\r\n");
	}
}

// SAVE command
void _ccp_save(void) {

}

// REN command
void _ccp_ren(void) {

}

// USER command
void _ccp_user(void) {

}

// INFO command
void _ccp_info(void) {
	_puts("RunCPM version " VERSION "\r\n");
	_puts("BDOS Page set to "); _puthex16(BDOSjmppage); _puts("\r\n");
	_puts("BIOS Page set to "); _puthex16(BIOSjmppage);
}

// External (.COM) command
uint8 _ccp_external(void) {
	uint8 result = FALSE;
	uint16 loadAddr = defLoad;

	_RamWrite(CmdFCB + 9, 'C');
	_RamWrite(CmdFCB + 10, 'O');
	_RamWrite(CmdFCB + 11, 'M');

	if (!_ccp_bdos(F_OPEN, CmdFCB, 0x00)) {
		_ccp_bdos(F_DMAOFF, loadAddr, 0x00);
		while (!_ccp_bdos(F_READ, CmdFCB, 0x00)) {
			loadAddr += 128;
			_ccp_bdos(F_DMAOFF, loadAddr, 0x00);
		}
		_ccp_bdos(F_CLOSE, CmdFCB, 0x00);

		Z80reset();			// Resets the Z80 CPU
		SET_LOW_REGISTER(BC, _RamRead(0x0004));	// Sets C to the current drive/user
		PC = 0x0100;		// Sets CP/M application jump point
		Z80run();			// Starts simulation

		result = TRUE;
	}

	return(result);
}

// Main CCP code
void _ccp(void) {
	uint8 ch;
	uint8 drvSel;	// Flag if a drive was explicitly selected for the command

	_puts(CCPHEAD);

	_ccp_bdos(DRV_ALLRESET, 0x0000, 0x00);
	_ccp_bdos(DRV_SET, curDrive, 0x00);

	while (TRUE) {
		command[0] = 0;
		_ccp_bdos(F_DMAOFF, defDMA, 0x00);
		curDrive = _ccp_bdos(DRV_GET, 0x0000, 0x00);
		RAM[0x0004] = curDrive;
		parDrive = curDrive;				// Initially the parameter and command drives are the same as the current drive

		prompt[2] = 'A' + curDrive;			// Shows the prompt
		_puts((char*)prompt);

		_RamWrite(inBuf, cLen);				// Sets the buffer size to read the command line
		_ccp_bdos(C_READSTR, inBuf, 0x00);

		blen = _RamRead(inBuf + 1);
		if (blen) {
			pbuf = _RamSysAddr(inBuf + 2);	// Points to the first command character

			while (*pbuf == ' ' && blen) {	// Skips any leading spaces
				pbuf++; blen--;
			}
			if (!blen)						// There were only spaces
				continue;
			if (*pbuf == ';')				// Found a comment
				continue;

			_ccp_initFCB(CmdFCB); // Initializes the command FCB

			// Checks for a drive and places it on the Command FCB
			drvSel = FALSE;
			if (blen > 1 && *(pbuf + 1) == ':') {
				cmdDrive = toupper(*pbuf++);
				cmdDrive -= '@';				// Makes the cmdDrive 1-10
				RAM[CmdFCB] = cmdDrive--;		// Adjusts the cmdDrive to 0-f
				pbuf++; blen -= 2;
				drvSel = TRUE;
			}

			// Extracts the command from the buffer (and makes it toupper)
			pcmd = &RAM[CmdFCB] + 1;
			clen = 8;					// Set the maximum extraction length for a command name (8)
			while (!_ccp_delim(*pbuf) && blen && clen) {
				*pcmd = toupper(*pbuf);
				command[8 - clen] = *pcmd;
				pbuf++; pcmd++; blen--; clen--;
			}
			command[8 - clen] = 0;

			if (RAM[CmdFCB + 1] == ' ') {	// Command was a drive select
				_ccp_bdos(DRV_SET, cmdDrive, 0x00);
				continue;
			}

			RAM[defDMA] = blen;	// Move the command line at this point to 0x0080
			for (ch = 0; ch < blen; ch++) {
				RAM[defDMA + ch + 1] = *(pbuf + ch);
			}
			RAM[defDMA + ch + 1] = 0;

			while (*pbuf == ' ' && blen) {		// Skips any leading spaces
				pbuf++; blen--;
			}

			_ccp_initFCB(ParFCB); // Initializes the parameter FCB

			// Loads the next file parameter onto the parameter FCB
			// Checks for a drive and places it on the parameter FCB
			if (blen > 1 && *(pbuf + 1) == ':') {
				parDrive = toupper(*pbuf++);
				parDrive -= '@';				// Makes the parDrive 1-10
				RAM[ParFCB] = parDrive--;		// Adjusts the parDrive to 0-f
				pbuf++; blen -= 2;
			}

			plen = 0;
			if (blen) {
				while (blen && plen < 8) {
					ch = toupper(*pbuf++);
					blen--;
					if (ch == '*') {
						ch = '?';
						break;
					}
					if (_ccp_delim(ch)) {
						ch = ' ';
						break;
					}
					plen++;
					RAM[ParFCB + plen] = ch;
					ch = ' ';
				}
				while (plen++ < 8)
					RAM[ParFCB + plen] = ch;

				if (*pbuf == '*' || *pbuf == '.') {
					pbuf++; blen--;
				}
				ch = ' ';
				while (blen && plen < 12) {
					ch = toupper(*pbuf++);
					blen--;
					if (ch == '*') {
						ch = '?';
						break;
					}
					if (_ccp_delim(ch)) {
						ch = ' ';
						break;
					}
					RAM[ParFCB + plen] = ch;
					plen++;
					ch = ' ';
				}
				while (plen < 12)
					RAM[ParFCB + plen++] = ch;
			}

			// Checks if the command is valid and executes
			_puts("\r\n");
			switch (_ccp_cnum()) {
			case 0:		// DIR
				_ccp_dir();		break;
			case 1:		// ERA
				_ccp_era();		break;
			case 2:		// TYPE
				_ccp_type();	break;
			case 3:		// SAVE
				_ccp_save();	break;
			case 4:		// REN
				_ccp_ren();		break;
			case 5:		// USER
				_ccp_user();	break;
				// Extra commands
			case 6:		// DEL is an alias to ERA
				_ccp_era();		break;
			case 7:		// INFO
				_ccp_info();	break;
			case 8:		// EXIT
				Status = 1;		break;
			case 255:	// It is an external command
				if (_ccp_external())	// Will fall down to default if it doesn't exist
					break;
			default:
				if (drvSel) {
					_putch(drvSel + 'A');
					_putch(':');
				}
				_puts((char*)command);
				_puts("?\r\n");
				blen = 0;
				break;
			}
		}
		if (Status == 1)	// This is set by a call to BIOS 0 - ends CP/M
			break;
	}
}

#endif
