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
#define F_DMAOFF		26

#define CLENGTH			124	// Maximum size of a command line

#define CmdFCB BatchFCB + 36	// FCB for use by internal commands
#define ParFCB	0x005C			// FCB for use by line parameters

#define NCMDS	6			// Maximum number of internal cmds

// CCP global variables
uint8 curDrive;	// 0 -> 15 = A -> P	.. Current drive for the CCP (same as RAM[0x0004]
uint8 cmdDrive;	// 0 -> 15 = A -> P .. Drive to read the (external) command from
uint8 parDrive;	// 0 -> 15 = A -> P .. Drive for the first file parameter
uint8 prompt[5] = "\r\n >";
uint8 *pbuf, *pcmd, *ppar;	// Pointers for extracting commands and parameters from command line buffer
uint8 blen;					// Actual size of the typed command line (size of the buffer)
uint8 clen;					// Actual size of the command in characters
uint8 plen;					// Actual size ot the first file parameter in characters
uint8 command[15] = "";		// Buffer for the external command filename
uint8 parm[15] = "";		// Buffer for the first file name parameter

static const char *Commands[NCMDS] =
{
	"DIR",
	"ERA",
	"TYPE",
	"SAVE",
	"REN",
	"USER"
};

uint8 _ccp_bdos(uint8 function, uint16 de, uint8 a) {
	SET_LOW_REGISTER(BC, function);
	DE = de;
	SET_HIGH_REGISTER(AF, a);
	_Bdos();
	return(HIGH_REGISTER(AF));
}

uint8 _ccp_cnum(void) {
	uint8 i = 0;
	uint8 result = 255;

	for (i = 0; i < NCMDS; i++) {
		if (strcmp((char*)command, Commands[i]) == 0) {
			result = i; break;
		}
	}
	return(result);
}

uint8 _ccp_separator(uint8 ch) {
	return(ch == ' ' || ch == '=');
}

void _ccp_NameToFCB(uint16 addr, uint8 *name) {
	uint8 i = 0;
	uint8 fill;
	uint8 psize;	// Part size

	// drive part
	if (*name) {
		name++;
		if (*name == ':') {
			parDrive = *(name - 1) - 'A';
			name++;
		} else {
			name--;
		}
	}
	_RamWrite(addr++, parDrive + 1);

	// name part
	psize = 8;
	fill = '?';
	while (psize) {
		switch (*name) {
		case 0:
			_RamWrite(addr++, fill); break;
		case '*':
			_RamWrite(addr++, '?'); break;
		case '.':
			fill = ' ';
			_RamWrite(addr++, ' '); break;
		default:
			fill = ' ';
			_RamWrite(addr++, *(name++)); break;
		}
		psize--;
	}
	while (*name == '*' || *name == '.')
		name++;
	// extension part
	psize = 3;
	fill = '?';
	while (psize) {
		switch (*name) {
		case 0:
			_RamWrite(addr++, fill); break;
		case '*':
			_RamWrite(addr++, '?'); break;
		default:
			fill = ' ';
			_RamWrite(addr++, *(name++)); break;
		}
		psize--;
	}
}

void _ccp_ffcb(uint16 address) {	// Fills the FCB with the next available parameter
	ppar = &parm[0];
	plen = 14;					// Set the maximum extraction length for a command (D:NNNNNNNN.EEE)
	while (!_ccp_separator(*pbuf) && blen && plen) {
		*ppar = toupper(*pbuf);
		pbuf++; ppar++; blen--; plen--;
	}
	*ppar = 0;	// Closes the command string

	_ccp_NameToFCB(address, parm);
}

void _ccp_zeroFCB(uint16 address) {
	uint8 i;

	for (i = 0; i < 36; i++)
		_RamWrite(address + i, 0x00);
	for (i = 0; i < 11; i++) {
		_RamWrite(address + 1 + i, 0x20);
		_RamWrite(address + 17 + i, 0x20);
	}
}

void _ccp_dir(void) {
	uint8 i;
	uint8 dirHead[6] = "\r\nA: ";
	uint8 dirSep[4] = " : ";
	uint32 fcount = 0;	// Number of files printed
	uint32 ccount = 0;	// Number of columns printed

	_ccp_zeroFCB(CmdFCB);	// Clears the FCB area (36 bytes)
	while (*pbuf == ' ' && blen) {		// Skips any leading spaces
		pbuf++; blen--;
	}

	dirHead[2] = parDrive + 'A';

	_ccp_ffcb(CmdFCB);
	if (!_SearchFirst(CmdFCB, TRUE)) {
		_puts((char*)dirHead);
		for (i = 0; i < 11; i++) {
			if (i == 8)
				_ccp_bdos(C_WRITE, ' ', 0x00);
			_ccp_bdos(C_WRITE, _RamRead(tmpFCB + i + 1), 0x00);
		}
		fcount++; ccount++;
		while (!_SearchNext(CmdFCB, TRUE)) {
			if (!ccount) {
				_puts((char*)dirHead);
			} else {
				_puts((char*)dirSep);
			}
			for (i = 0; i < 11; i++) {
				if (i == 8)
					_ccp_bdos(C_WRITE, ' ', 0x00);
				_ccp_bdos(C_WRITE, _RamRead(tmpFCB + i + 1), 0x00);
			}
			fcount++; ccount++;
			if (ccount > 4)
				ccount = 0;
		}
	} else {
		_puts("\r\nNo file");
	}
}

void _ccp_era(void) {
	_ccp_zeroFCB(CmdFCB);	// Clears the FCB area (36 bytes)
	while (*pbuf == ' ' && blen) {		// Skips any leading spaces
		pbuf++; blen--;
	}

	_ccp_ffcb(CmdFCB);
	if (_ccp_bdos(F_DELETE, CmdFCB, 0x00))
		_puts("\r\nNo file");
}

void _ccp_type(void) {
	uint8 i, c;
	uint16 a;

	_ccp_zeroFCB(CmdFCB);	// Clears the FCB area (36 bytes)
	while (*pbuf == ' ' && blen) {		// Skips any leading spaces
		pbuf++; blen--;
	}

	_puts("\r\n");
	_ccp_ffcb(CmdFCB);
	if (!_ccp_bdos(F_OPEN, CmdFCB, 0x00)) {
		while (!_ccp_bdos(F_READ, CmdFCB, 0x00)) {
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
		_puts((char*)parm);
		_puts("?");
	}
}

void _ccp_save(void) {

}

void _ccp_ren(void) {

}

void _ccp_user(void) {

}

uint8 _ccp_external(void) {
	uint8 result = FALSE;
	uint16 loadAddr = 0x0100;

	_ccp_NameToFCB(CmdFCB, command);
	_RamWrite(CmdFCB + 9, 'C');
	_RamWrite(CmdFCB + 10, 'O');
	_RamWrite(CmdFCB + 11, 'M');

	if (!_ccp_bdos(F_OPEN, CmdFCB, 0x00)) {
		_ccp_zeroFCB(ParFCB);	// Clears the FCB area (36 bytes)
		while (*pbuf == ' ' && blen) {		// Skips any leading spaces
			pbuf++; blen--;
		}
		//		_ccp_ffcb(ParFCB);
		_RamWrite(0x0080, 0x00);

		_ccp_bdos(F_DMAOFF, loadAddr, 0x00);
		while (!_ccp_bdos(F_READ, CmdFCB, 0x00)) {
			loadAddr += 128;
			_ccp_bdos(F_DMAOFF, loadAddr, 0x00);
		}
		_ccp_bdos(F_CLOSE, CmdFCB, 0x00);

		_puts("\r\n");
		Z80reset();			// Resets the Z80 CPU
		SET_LOW_REGISTER(BC, _RamRead(0x0004));	// Sets C to the current drive/user
		PC = 0x0100;		// Sets CP/M application jump point
		Z80run();			// Starts simulation

		result = TRUE;
	}

	return(result);
}

void _ccp(void) {

	_puts(CCPHEAD);

	while (TRUE) {
		command[0] = 0;
		parm[0] = 0; ppar = &parm[0];
		curDrive = _RamRead(0x0004) & 0x0f;
		_ccp_bdos(DRV_ALLRESET, 0x0000, 0x00);
		_ccp_bdos(DRV_SET, curDrive, 0x00);
		parDrive = cmdDrive = curDrive;	// Initially the parameter and command drives are the same as the current drive

		prompt[2] = 'A' + curDrive;
		_puts((char*)prompt);

		_RamWrite(dmaAddr, CLENGTH);
		_ccp_bdos(C_READSTR, dmaAddr, 0x00);
		blen = _RamRead(dmaAddr + 1);
		if (blen) {
			pbuf = _RamSysAddr(dmaAddr + 2);	// Points to the first command character

			while (*pbuf == ' ' && blen) {		// Skips any leading spaces
				pbuf++; blen--;
			}
			if (!blen)					// There were only spaces
				continue;
			if (*pbuf == ';')			// Found a comment
				continue;

			// Extracts the command from the buffer (and makes it toupper)
			pcmd = &command[0];
			clen = 14;					// Set the maximum extraction length for a command (D:NNNNNNNN.EEE)
			while (*pbuf != ' ' && blen && clen) {
				*pcmd = toupper(*pbuf);
				pbuf++; pcmd++; blen--; clen--;
			}
			*pcmd = 0;	// Closes the command string

			if (command[1] == ':') {	// If the command has a drive, maybe do a drive select
				curDrive = command[0] - 'A';
				if (!command[2]) {		// Command was a drive select itself
					_ccp_bdos(DRV_SET, curDrive, 0x00);
					continue;
				}
			}

			// Checks if the command is valid
			switch (_ccp_cnum()) {
			case 0:		// DIR
				_ccp_dir(); break;
			case 1:		// ERA
				_ccp_era(); break;
			case 2:		// TYPE
				_ccp_type(); break;
			case 3:		// SAVE
				_ccp_save(); break;
			case 4:		// REN
				_ccp_ren(); break;
			case 5:		// USER
				_ccp_user();  break;
			case 255:	// It is an external command
				if (_ccp_external())	// Will fall down to default is doesn't exist
					break;
			default:
				_puts("\r\n");
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
