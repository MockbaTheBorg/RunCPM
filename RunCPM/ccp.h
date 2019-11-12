#ifndef CCP_H
#define CCP_H

// CP/M BDOS calls
#define C_READ			1
#define C_WRITE			2
#define C_READSTR		10
#define DRV_ALLRESET	13
#define DRV_SET			14
#define F_OPEN			15
#define F_CLOSE			16
#define F_DELETE		19
#define F_READ			20
#define F_WRITE			21
#define F_MAKE			22
#define F_RENAME		23
#define DRV_GET			25
#define F_DMAOFF		26
#define F_USERNUM		32
#define F_RUNLUA		254

#define CmdFCB	(BatchFCB + 36)		// FCB for use by internal commands
#define ParFCB	0x005C				// FCB for use by line parameters
#define SecFCB	0x006C				// Secondary part of FCB for renaming files
#define Trampoline (CmdFCB + 36)	// Trampoline for running external commands

#define inBuf	(BDOSjmppage - 256)	// Input buffer location
#define cmdLen	125					// Maximum size of a command line (sz+rd+cmd+\0)

#define defDMA	0x0080				// Default DMA address
#define defLoad	0x0100				// Default load address

#define pgSize	24					// for TYPE

// CCP global variables
uint8 curDrive;	// 0 -> 15 = A -> P	.. Current drive for the CCP (same as RAM[0x0004]
uint8 parDrive;	// 0 -> 15 = A -> P .. Drive for the first file parameter
uint8 curUser;	// 0 -> 15			.. Current user area to access
uint8 sFlag;	//					.. Submit Flag
uint8 prompt[6] = "\r\n  >";
uint16 pbuf, perr;
uint8 blen;							// Actual size of the typed command line (size of the buffer)

static const char* Commands[] =
{
	// Standard CP/M commands
	"DIR",
	"ERA",
	"TYPE",
	"SAVE",
	"REN",
	"USER",
	// Extra CCP commands
	"CLS",
	"DEL",
	"EXIT",
	NULL
};

// Used to call BDOS from inside the CCP
uint16 _ccp_bdos(uint8 function, uint16 de) {
	SET_LOW_REGISTER(BC, function);
	DE = de;
	_Bdos();
	return(HL & 0xffff);
}

// Compares two strings (Atmel doesn't like strcmp)
uint8 _ccp_strcmp(char* stra, char* strb) {
	while (*stra && *strb && (*stra == *strb)) {
		++stra; ++strb;
	}
	return(*stra == *strb);
}

// Gets the command ID number
uint8 _ccp_cnum(void) {
	uint8 result = 255;
	uint8 command[9];
	uint8 i = 0;

	if (!_RamRead(CmdFCB)) {	// If a drive was set, then the command is external
		while (i < 8 && _RamRead(CmdFCB + i + 1) != ' ') {
			command[i] = _RamRead(CmdFCB + i + 1);
			++i;
		}
		command[i] = 0;

		i = 0;
		while (Commands[i]) {
			if (_ccp_strcmp((char*)command, (char*)Commands[i])) {
				result = i;
				perr = defDMA + 2;
				break;
			}
			++i;
		}
	}
	return(result);
}

// Returns true if character is a separator
uint8 _ccp_delim(uint8 ch) {
	return(ch == 0 || ch == ' ' || ch == '=' || ch == '.' || ch == ':' || ch == ';' || ch == '<' || ch == '>');
}

// Prints the FCB filename
void _ccp_printfcb(uint16 fcb, uint8 compact) {
	uint8 i, ch;

	ch = _RamRead(fcb);
	if (ch && compact) {
		_ccp_bdos(C_WRITE, ch + '@');
		_ccp_bdos(C_WRITE, ':');
	}

	for (i = 1; i < 12; ++i) {
		ch = _RamRead(fcb + i);
		if (ch == ' ' && compact)
			continue;
		if (i == 9)
			_ccp_bdos(C_WRITE, compact ? '.' : ' ');
		_ccp_bdos(C_WRITE, ch);
	}
}

// Initializes the FCB
void _ccp_initFCB(uint16 address) {
	uint8 i;

	for (i = 0; i < 36; ++i)
		_RamWrite(address + i, 0x00);
	for (i = 0; i < 11; ++i) {
		_RamWrite(address + 1 + i, 0x20);
		_RamWrite(address + 17 + i, 0x20);
	}
}

// Name to FCB
uint8 _ccp_nameToFCB(uint16 fcb) {
	uint8 pad, plen, ch, n = 0;

	// Checks for a drive and places it on the Command FCB
	if (_RamRead(pbuf + 1) == ':') {
		ch = toupper(_RamRead(pbuf++));
		_RamWrite(fcb, ch - '@');		// Makes the drive 0x1-0xF for A-P
		++pbuf;							// Points pbuf past the :
		blen -= 2;
	}

	if (blen) {
		++fcb;

		plen = 8;
		pad = ' ';
		ch = toupper(_RamRead(pbuf));
		while (blen && plen) {
			if (_ccp_delim(ch)) {
				break;
			}
			++pbuf; --blen;
			if (ch == '*')
				pad = '?';
			if (pad == '?' || ch == '?') {
				ch = pad;
				n = n | 0x80;	// Name is not unique
			}
			--plen; ++n;
			_RamWrite(fcb++, ch);
			ch = toupper(_RamRead(pbuf));
		}

		while (plen--)
			_RamWrite(fcb++, pad);

		plen = 3;
		pad = ' ';
		if (ch == '.') {
			++pbuf; --blen;
		}
		while (blen && plen) {
			ch = toupper(_RamRead(pbuf));
			if (_ccp_delim(ch)) {
				break;
			}
			++pbuf; --blen;
			if (ch == '*')
				pad = '?';
			if (pad == '?' || ch == '?') {
				ch = pad;
				n = n | 0x80;	// Name is not unique
			}
			--plen; ++n;
			_RamWrite(fcb++, ch);
		}

		while (plen--)
			_RamWrite(fcb++, pad);
	}

	return(n);
}

// Converts the ParFCB name to a number
uint16 _ccp_fcbtonum() {
	uint8 ch;
	uint16 n = 0;
	uint8 pos = ParFCB + 1;
	while (TRUE) {
		ch = _RamRead(pos++);
		if (ch < '0' || ch > '9')
			break;
		n = (n * 10) + (ch - 0x30);
	}
	return(n);
}

// DIR command
void _ccp_dir(void) {
	uint8 i;
	uint8 dirHead[6] = "A: ";
	uint8 dirSep[6] = "  |  ";
	uint32 fcount = 0;	// Number of files printed
	uint32 ccount = 0;	// Number of columns printed

	if (_RamRead(ParFCB + 1) == ' ') {
		for (i = 1; i < 12; ++i)
			_RamWrite(ParFCB + i, '?');
	}

	dirHead[0] = _RamRead(ParFCB) ? _RamRead(ParFCB) + '@' : prompt[2];

	_puts("\r\n");
	if (!_SearchFirst(ParFCB, TRUE)) {
		_puts((char*)dirHead);
		_ccp_printfcb(tmpFCB, FALSE);
		++fcount; ++ccount;
		while (!_SearchNext(ParFCB, TRUE)) {
			if (!ccount) {
				_puts("\r\n");
				_puts((char*)dirHead);
			} else {
				_puts((char*)dirSep);
			}
			_ccp_printfcb(tmpFCB, FALSE);
			++fcount; ++ccount;
			if (ccount > 3)
				ccount = 0;
		}
	} else {
		_puts("No file");
	}
}

// ERA command
void _ccp_era(void) {
	if (_ccp_bdos(F_DELETE, ParFCB))
		_puts("\r\nNo file");
}

// TYPE command
uint8 _ccp_type(void) {
	uint8 i, c, l = 0, error = TRUE;
	uint16 a, p = 0;

	if (!_ccp_bdos(F_OPEN, ParFCB)) {
		_puts("\r\n");
		while (!_ccp_bdos(F_READ, ParFCB)) {
			i = 128;
			a = dmaAddr;
			while (i) {
				c = _RamRead(a);
				if (c == 0x1a)
					break;
				_ccp_bdos(C_WRITE, c);
				if (c == 0x0a) {
					++l;
					if (l == pgSize) {
						l = 0;
						p = _ccp_bdos(C_READ, 0x0000);
						if (p == 3)
							break;
					}
				}
				--i; ++a;
			}
			if (p == 3)
				break;
		}
		error = FALSE;
	}
	return(error);
}

// SAVE command
uint8 _ccp_save(void) {
	uint8 error = TRUE;
	uint16 pages = _ccp_fcbtonum();
	uint16 i, dma;

	if (pages < 256) {
		error = FALSE;
		while (_RamRead(pbuf) == ' ' && blen) {		// Skips any leading spaces
			++pbuf; --blen;
		}
		_ccp_nameToFCB(ParFCB);						// Loads file name onto the ParFCB
		if (_ccp_bdos(F_MAKE, ParFCB)) {
			_puts("Err: create");
		} else {
			if (_ccp_bdos(F_OPEN, ParFCB)) {
				_puts("Err: open");
			} else {
				pages *= 2;							// Calculates the number of CP/M blocks to write
				dma = defLoad;
				_puts("\r\n");
				for (i = 0; i < pages; i++) {
					_ccp_bdos(F_DMAOFF, dma);
					_ccp_bdos(F_WRITE, ParFCB);
					dma += 128;
					_ccp_bdos(C_WRITE, '.');
				}
				_ccp_bdos(F_CLOSE, ParFCB);
			}
		}
	}
	return(error);
}

// REN command
void _ccp_ren(void) {
	uint8 ch, i;
	++pbuf;

	_ccp_nameToFCB(SecFCB);
	for (i = 0; i < 12; ++i) {	// Swap the filenames on the fcb
		ch = _RamRead(ParFCB + i);
		_RamWrite(ParFCB + i, _RamRead(SecFCB + i));
		_RamWrite(SecFCB + i, ch);
	}
	if (_ccp_bdos(F_RENAME, ParFCB)) {
		_puts("\r\nNo file");
	}
}

// USER command
uint8 _ccp_user(void) {
	uint8 error = TRUE;

	curUser = (uint8)_ccp_fcbtonum();
	if (curUser < 16) {
		_ccp_bdos(F_USERNUM, curUser);
		error = FALSE;
	}
	return(error);
}

#ifdef HASLUA
// External (.LUA) command
uint8 _ccp_lua(void) {
	uint8 error = TRUE;
	uint8 found, drive, user = 0;
	uint16 loadAddr = defLoad;

	_RamWrite(CmdFCB + 9, 'L');
	_RamWrite(CmdFCB + 10, 'U');
	_RamWrite(CmdFCB + 11, 'A');

	drive = _RamRead(CmdFCB);
	found = !_ccp_bdos(F_OPEN, CmdFCB);					// Look for the program on the FCB drive, current or specified
	if (!found) {										// If not found
		if (!drive) {									// and the search was on the default drive
			_RamWrite(CmdFCB, 0x01);					// Then look on drive A: user 0
			if (curUser) {
				user = curUser;							// Save the current user
				_ccp_bdos(F_USERNUM, 0x0000);			// then set it to 0
			}
			found = !_ccp_bdos(F_OPEN, CmdFCB);
			if (!found) {								// If still not found then
				if (curUser) {							// If current user not = 0
					_RamWrite(CmdFCB, 0x00);			// look on current drive user 0
					found = !_ccp_bdos(F_OPEN, CmdFCB);	// and try again
				}
			}
		}
	}
	if (found) {
		_puts("\r\n");

		_ccp_bdos(F_RUNLUA, CmdFCB);

		if (user) {								// If a user was selected
			user = 0;
			_ccp_bdos(F_USERNUM, curUser);		// Set it back
			_RamWrite(CmdFCB, 0x00);
		}
		error = FALSE;
	}

	if (user) {									// If a user was selected
		_ccp_bdos(F_USERNUM, curUser);			// Set it back
		_RamWrite(CmdFCB, 0x00);
	}

	return(error);
}
#endif

// External (.COM) command
uint8 _ccp_ext(void) {
	uint8 error = TRUE;
	uint8 found, drive, user = 0;
	uint16 loadAddr = defLoad;

	_RamWrite(CmdFCB + 9, 'C');
	_RamWrite(CmdFCB + 10, 'O');
	_RamWrite(CmdFCB + 11, 'M');

	drive = _RamRead(CmdFCB);							// Get the drive from the command FCB
	found = !_ccp_bdos(F_OPEN, CmdFCB);					// Look for the program on the FCB drive, current or specified
	if (!found) {										// If not found
		if (!drive) {									// and the search was on the default drive
			_RamWrite(CmdFCB, 0x01);					// Then look on drive A: user 0
			if (curUser) {
				user = curUser;							// Save the current user
				_ccp_bdos(F_USERNUM, 0x0000);			// then set it to 0
			}
			found = !_ccp_bdos(F_OPEN, CmdFCB);
			if (!found) {								// If still not found then
				if (curUser) {							// If current user not = 0
					_RamWrite(CmdFCB, 0x00);			// look on current drive user 0
					found = !_ccp_bdos(F_OPEN, CmdFCB);	// and try again
				}
			}
		}
	}
	if (found) {									// Program was found somewhere
		_puts("\r\n");
		_ccp_bdos(F_DMAOFF, loadAddr);				// Sets the DMA address for the loading
		while (!_ccp_bdos(F_READ, CmdFCB)) {		// Loads the program into memory
			loadAddr += 128;
			if (loadAddr == BDOSjmppage) {			// Breaks if it reaches the end of TPA
				_puts("\r\nNo Memory");
				break;
			}
			_ccp_bdos(F_DMAOFF, loadAddr);			// Points the DMA offset to the next loadAddr
		}
		_ccp_bdos(F_DMAOFF, defDMA);				// Points the DMA offset back to the default

		if (user) {									// If a user was selected
			user = 0;
			_ccp_bdos(F_USERNUM, curUser);			// Set it back
		}
		_RamWrite(CmdFCB, drive);

		// Place a trampoline to call the external command
		// as it may return using RET instead of JP 0000h
		loadAddr = Trampoline;
		_RamWrite(loadAddr, CALL);					// CALL 0100h
		_RamWrite16(loadAddr + 1, defLoad);
		_RamWrite(loadAddr + 3, JP);				// JP RETTOCCP
		_RamWrite16(loadAddr + 4, BIOSjmppage + 0x33);

		Z80reset();									// Resets the Z80 CPU
		SET_LOW_REGISTER(BC, _RamRead(0x0004));		// Sets C to the current drive/user
		PC = loadAddr;								// Sets CP/M application jump point
		SP = BDOSjmppage;							// Sets the stack to the top of the TPA

		Z80run();									// Starts Z80 simulation

		error = FALSE;
	}

	if (user) {										// If a user was selected
		_ccp_bdos(F_USERNUM, curUser);				// Set it back
	}
	_RamWrite(CmdFCB, drive);						// Set the command FCB drive back to what it was

	return(error);
}

// Prints a command error
void _ccp_cmdError() {
	uint8 ch;

	_puts("\r\n");
	while ((ch = _RamRead(perr++))) {
		if (ch == ' ')
			break;
		_ccp_bdos(C_WRITE, toupper(ch));
	}
	_puts("?\r\n");
}

// Reads input, either from the $$$.SUB or console
void _ccp_readInput(void) {
	uint8 i;
	uint8 recs = 0;
	uint8 chars;

	if (sFlag) {									// Are we running a submit?
		if (!_ccp_bdos(F_OPEN, BatchFCB)) {			// Open batch file
			recs = _RamRead(BatchFCB + 15);			// Gets its record count
			if (recs) {
				--recs;								// Counts one less
				_RamWrite(BatchFCB + 32, recs);		// And sets to be the next read
				_ccp_bdos(F_DMAOFF, defDMA);		// Reset current DMA
				_ccp_bdos(F_READ, BatchFCB);		// And reads the last sector
				chars = _RamRead(defDMA);			// Then moves it to the input buffer
				for (i = 0; i <= chars; ++i)
					_RamWrite(inBuf + i + 1, _RamRead(defDMA + i));
				_RamWrite(inBuf + i + 1, 0);
				_puts((char*)_RamSysAddr(inBuf + 2));
				_RamWrite(BatchFCB + 15, recs);		// Prepare the file to be truncated
				_ccp_bdos(F_CLOSE, BatchFCB);		// And truncates it
			}
		}
		if (!recs) {
			_ccp_bdos(F_DELETE, BatchFCB);			// Or else just deletes it
			sFlag = 0;								// and clears the submit flag
		}
	} else {
		_ccp_bdos(C_READSTR, inBuf);				// Reads the command line from console
	}
}

// Main CCP code
void _ccp(void) {

	uint8 i;

	sFlag = (uint8)_ccp_bdos(DRV_ALLRESET, 0x0000);
	_ccp_bdos(DRV_SET, curDrive);

	for (i = 0; i < 36; ++i)
		_RamWrite(BatchFCB + i, _RamRead(tmpFCB + i));

	while (TRUE) {
		curDrive = (uint8)_ccp_bdos(DRV_GET, 0x0000);	// Get current drive
		curUser = (uint8)_ccp_bdos(F_USERNUM, 0x00FF);	// Get current user
		_RamWrite(0x0004, (curUser << 4) + curDrive);	// Set user/drive on addr 0x0004

		parDrive = curDrive;							// Initially the parameter drive is the same as the current drive

		prompt[2] = 'A' + curDrive;						// Shows the prompt
		prompt[3] = (curUser < 10) ? '0' + curUser : 'W' + curUser;
		_puts((char*)prompt);

		_RamWrite(inBuf, cmdLen);						// Sets the buffer size to read the command line
		_ccp_readInput();

		blen = _RamRead(inBuf + 1);						// Obtains the number of bytes read

		_ccp_bdos(F_DMAOFF, defDMA);					// Reset current DMA

		if (blen) {
			_RamWrite(inBuf + 2 + blen, 0);				// "Closes" the read buffer with a \0
			pbuf = inBuf + 2;							// Points pbuf to the first command character

			while (_RamRead(pbuf) == ' ' && blen) {		// Skips any leading spaces
				++pbuf; --blen;
			}
			if (!blen)									// There were only spaces
				continue;
			if (_RamRead(pbuf) == ';')					// Found a comment line
				continue;

			_ccp_initFCB(CmdFCB);						// Initializes the command FCB

			perr = pbuf;								// Saves the pointer in case there's an error
			if (_ccp_nameToFCB(CmdFCB) > 8) {			// Extracts the command from the buffer
				_ccp_cmdError();						// Command name cannot be non-unique or have an extension
				continue;
			}

			if (_RamRead(CmdFCB) && _RamRead(CmdFCB + 1) == ' ') {	// Command was a simple drive select
				_ccp_bdos(DRV_SET, _RamRead(CmdFCB) - 1);
				continue;
			}

			_RamWrite(defDMA, blen);					// Move the command line at this point to 0x0080
			for (i = 0; i < blen; ++i) {
				_RamWrite(defDMA + i + 1, _RamRead(pbuf + i));
			}
			_RamWrite(defDMA + i + 1, 0);

			while (_RamRead(pbuf) == ' ' && blen) {		// Skips any leading spaces
				++pbuf; --blen;
			}

			_ccp_initFCB(ParFCB);						// Initializes the parameter FCB
			_ccp_nameToFCB(ParFCB);						// Loads the next file parameter onto the parameter FCB

			i = FALSE;									// Checks if the command is valid and executes
			switch (_ccp_cnum()) {
				// Standard CP/M commands
			case 0:		// DIR
				_ccp_dir();			break;
			case 1:		// ERA
				_ccp_era();			break;
			case 2:		// TYPE
				i = _ccp_type();	break;
			case 3:		// SAVE
				i = _ccp_save();	break;
			case 4:		// REN
				_ccp_ren();			break;
			case 5:		// USER
				i = _ccp_user();	break;
				// Extra CCP commands
			case 6:		// CLS
				_clrscr();			break;
			case 7:		// DEL is an alias to ERA
				_ccp_era();			break;
			case 8:		// EXIT
				_puts("Terminating RunCPM.\r\n");
				_puts("CPU Halted.\r\n");
				Status = 1;			break;
			case 255:	// It is an external command
				i = _ccp_ext();
#ifdef HASLUA
				if (i)
					i = _ccp_lua();
#endif
				break;
			default:
				i = TRUE;			break;
			}
			if (i)
				_ccp_cmdError();
		}
		if (Status == 1 || Status == 2)
			break;
	}
	_puts("\r\n");
}

#endif
