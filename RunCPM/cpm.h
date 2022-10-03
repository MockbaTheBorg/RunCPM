#ifndef CPM_H
#define CPM_H

enum eBDOSFunc {
	F_BOOT = 0,
	C_READ = 1,
	C_WRITE = 2,
	READER_IN = 3,
	PUNCH_OUT = 4,
	PRINT_OUT = 5,
	DIRECT_IO = 6,
	GET_IOBYTE = 7,
	SET_IOBYTE = 8,
	OUT_STRING = 9,
	C_READSTR = 10,
	C_STAT = 11,
	GET_VERSION = 12,
	DRV_ALLRESET = 13,
	DRV_SET = 14,
	F_OPEN = 15,
	F_CLOSE = 16,
	F_SEARCH_FIRST = 17,
	F_SEARCH_NEXT = 18,
	F_DELETE = 19,
	F_READ = 20,
	F_WRITE = 21,
	F_MAKE = 22,
	F_RENAME = 23,
	DRV_LOGINVECTOR = 24,
	DRV_GET = 25,
	F_DMAOFF = 26,
	DRV_GETADDRALLOC = 27,
	DRV_WRITEPROTECT = 28,
	DRV_GETROVECTOR = 29,
	F_SETATTRIBUTES = 30,
	DRV_GETDPB = 31,
	F_USERNUM = 32,
	F_READRANDOM = 33,
	F_WRITERANDOM = 34,
	F_COMPUTESIZE = 35,
	F_SETRANDOM = 36,
	DRV_RESET = 37,
	F_WRITERANDOMZERO = 40,
	F_RUNLUA = 254
};

/* see main.c for definition */

#define JP 0xc3
#define CALL 0xcd
#define RET 0xc9
#define INa 0xdb    // Triggers a BIOS call
#define OUTa 0xd3   // Triggers a BDOS call

/* set up full PUN and LST filenames to be on drive A: user 0 */
#ifdef USE_PUN
char pun_file[17] = {'A', FOLDERCHAR, '0', FOLDERCHAR, 'P', 'U', 'N', '.', 'T', 'X', 'T', 0};

#endif // ifdef USE_PUN
#ifdef USE_LST
char lst_file[17] = {'A', FOLDERCHAR, '0', FOLDERCHAR, 'L', 'S', 'T', '.', 'T', 'X', 'T', 0};

#endif // ifdef USE_LST

#ifdef PROFILE
unsigned long time_start = 0;
unsigned long time_now = 0;

#endif // ifdef PROFILE

void _PatchCPM(void) {
	uint16 i;

	// **********  Patch CP/M page zero into the memory  **********

	/* BIOS entry point */
	_RamWrite(0x0000, JP);  /* JP BIOS+3 (warm boot) */
	_RamWrite16(0x0001, BIOSjmppage + 3);
	if (Status != 2) {
		/* IOBYTE - Points to Console */
		_RamWrite(	IOByte,		0x3D);

		/* Current drive/user - A:/0 */
		_RamWrite(	DSKByte,	0x00);
	}
	/* BDOS entry point (0x0005) */
	_RamWrite(0x0005, JP);
	_RamWrite16(0x0006,				BDOSjmppage + 0x06);

	// **********  Patch CP/M Version into the memory so the CCP can see it
	_RamWrite16(BDOSjmppage,		0x1600);
	_RamWrite16(BDOSjmppage + 2,	0x0000);
	_RamWrite16(BDOSjmppage + 4,	0x0000);

	// Patches in the BDOS jump destination
	_RamWrite(BDOSjmppage + 6, JP);
	_RamWrite16(BDOSjmppage + 7, BDOSpage);

	// Patches in the BDOS page content
	_RamWrite(	BDOSpage,		INa);
	_RamWrite(	BDOSpage + 1,	0xFF);
	_RamWrite(	BDOSpage + 2,	RET);

	// Patches in the BIOS jump destinations
	for (i = 0; i < 0x36; i = i + 3) {
		_RamWrite(BIOSjmppage + i, JP);
		_RamWrite16(BIOSjmppage + i + 1, BIOSpage + i);
	}

	// Patches in the BIOS page content
	for (i = 0; i < 0x36; i = i + 3) {
		_RamWrite(	BIOSpage + i,		OUTa);
		_RamWrite(	BIOSpage + i + 1,	0xFF);
		_RamWrite(	BIOSpage + i + 2,	RET);
	}
	// **********  Patch CP/M (fake) Disk Paramater Block after the BDOS call entry  **********
	i = DPBaddr;
	_RamWrite(	i++,	64);    // spt - Sectors Per Track
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	5);     // bsh - Data allocation "Block Shift Factor"
	_RamWrite(	i++,	0x1F);  // blm - Data allocation Block Mask
	_RamWrite(	i++,	1);     // exm - Extent Mask
	_RamWrite(	i++,	0xFF);  // dsm - Total storage capacity of the disk drive
	_RamWrite(	i++,	0x07);
	_RamWrite(	i++,	255);   // drm - Number of the last directory entry
	_RamWrite(	i++,	3);
	_RamWrite(	i++,	0xFF);  // al0
	_RamWrite(	i++,	0x00);  // al1
	_RamWrite(	i++,	0);     // cks - Check area Size
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0x02);  // off - Number of system reserved tracks at the beginning of the ( logical ) disk
	_RamWrite(	i++,	0x00);
	blockShift = _RamRead(DPBaddr + 2);
	blockMask = _RamRead(DPBaddr + 3);
	extentMask = _RamRead(DPBaddr + 4);
	numAllocBlocks = _RamRead16((DPBaddr + 5)) + 1;
	extentsPerDirEntry = extentMask + 1;

	// **********  Patch CP/M (fake) Disk Parameter Header after the Disk Parameter Block  **********
	_RamWrite(	i++,	0); // Addr of the sector translation table
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0); // Workspace
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0x80);                  // Addr of the Sector Buffer
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	LOW_REGISTER(DPBaddr)); // Addr of the DPB Disk Parameter Block
	_RamWrite(	i++,	HIGH_REGISTER(DPBaddr));
	_RamWrite(	i++,	0);                     // Addr of the Directory Checksum Vector
	_RamWrite(	i++,	0);
	_RamWrite(	i++,	0);                     // Addr of the Allocation Vector
	_RamWrite(	i++,	0);

	//

	// figure out the number of the first allocation block
	// after the directory for the phoney allocation block
	// list in _findnext()
	firstBlockAfterDir = 0;
	i = 0x80;

	while (_RamRead(DPBaddr + 9) & i) {
		firstBlockAfterDir++;
		i >>= 1;
	}
	if (_RamRead(DPBaddr + 9) == 0xFF) {
		i = 0x80;

		while (_RamRead(DPBaddr + 10) & i) {
			firstBlockAfterDir++;
			i >>= 1;
		}
	}
	physicalExtentBytes = logicalExtentBytes * (extentMask + 1);
} // _PatchCPM

#ifdef DEBUGLOG
uint8 LogBuffer[128];

void _logRegs(void) {
	uint8 J, I;
	uint8 Flags[9] = {'S', 'Z', '5', 'H', '3', 'P', 'N', 'C'};
	uint8 c = HIGH_REGISTER(AF);

	if ((c < 32) || (c > 126)) {
		c = 46;
	}

	for (J = 0, I = LOW_REGISTER(AF); J < 8; ++J, I <<= 1) {
		Flags[J] = I & 0x80 ? Flags[J] : '.';
	}
	sprintf((char *)LogBuffer, "  BC:%04x DE:%04x HL:%04x AF:%02x(%c)|%s| IX:%04x IY:%04x SP:%04x PC:%04x\n",
			WORD16(BC), WORD16(DE), WORD16(HL), HIGH_REGISTER(AF), c, Flags, WORD16(IX), WORD16(IY), WORD16(SP), WORD16(PC));
	_sys_logbuffer(LogBuffer);
} // _logRegs

void _logMem(uint16 address, uint8 amount) {    // Amount = number of 16 bytes lines, so 1 CP/M block = 8, not 128
	uint8 i, m, c, pos;
	uint8 head = 8;
	uint8 hexa[] = "0123456789ABCDEF";

	for (i = 0; i < amount; ++i) {
		pos = 0;

		for (m = 0; m < head; ++m) {
			LogBuffer[pos++] = ' ';
		}
		sprintf((char *)LogBuffer, "  %04x: ", address);

		for (m = 0; m < 16; ++m) {
			c = _RamRead(address++);
			LogBuffer[pos++] = hexa[c >> 4];
			LogBuffer[pos++] = hexa[c & 0x0f];
			LogBuffer[pos++] = ' ';
			LogBuffer[m + head + 48] = c > 31 && c < 127 ? c : '.';
		}
		pos += 16;
		LogBuffer[pos++] = 0x0a;
		LogBuffer[pos++] = 0x00;
		_sys_logbuffer(LogBuffer);
	}
} // _logMem

void _logChar(char *txt, uint8 c) {
	uint8 asc[2];

	asc[0] = c > 31 && c < 127 ? c : '.';
	asc[1] = 0;
	sprintf((char *)LogBuffer, "        %s = %02xh:%3d (%s)\n", txt, c, c, asc);
	_sys_logbuffer(LogBuffer);
} // _logChar

void _logBiosIn(uint8 ch) {
#ifdef LOGBIOS_NOT
	if (ch == LOGBIOS_NOT) {
		return;
	}
#endif // ifdef LOGBIOS_NOT
#ifdef LOGBIOS_ONLY
	if (ch != LOGBIOS_ONLY) {
		return;
	}
#endif // ifdef LOGBIOS_ONLY
	static const char *BIOSCalls[18] =
	{
		"boot", "wboot", "const", "conin", "conout", "list", "punch/aux", "reader", "home", "seldsk", "settrk", "setsec", "setdma",
		"read", "write", "listst", "sectran", "altwboot"
	};
	int index = ch / 3;

	if (index < 18) {
		sprintf((char *)LogBuffer, "\nBios call: %3d/%02xh (%s) IN:\n", ch, ch, BIOSCalls[index]);
		_sys_logbuffer(LogBuffer);
	} else {
		sprintf((char *)LogBuffer, "\nBios call: %3d/%02xh IN:\n", ch, ch);
		_sys_logbuffer(LogBuffer);
	}
	_logRegs();
} // _logBiosIn

void _logBiosOut(uint8 ch) {
#ifdef LOGBIOS_NOT
	if (ch == LOGBIOS_NOT) {
		return;
	}
#endif // ifdef LOGBIOS_NOT
#ifdef LOGBIOS_ONLY
	if (ch != LOGBIOS_ONLY) {
		return;
	}
#endif // ifdef LOGBIOS_ONLY
	sprintf((char *)LogBuffer, "               OUT:\n");
	_sys_logbuffer(LogBuffer);
	_logRegs();
} // _logBiosOut

void _logBdosIn(uint8 ch) {
#ifdef LOGBDOS_NOT
	if (ch == LOGBDOS_NOT) {
		return;
	}
#endif // ifdef LOGBDOS_NOT
#ifdef LOGBDOS_ONLY
	if (ch != LOGBDOS_ONLY) {
		return;
	}
#endif // ifdef LOGBDOS_ONLY
	uint16 address = 0;
	uint8 size = 0;

	static const char *CPMCalls[41] =
	{
		"System Reset", "Console Input", "Console Output", "Reader Input", "Punch Output", "List Output", "Direct I/O",
		"Get IOByte",
		"Set IOByte", "Print String", "Read Buffered", "Console Status", "Get Version", "Reset Disk", "Select Disk", "Open File",
		"Close File", "Search First", "Search Next", "Delete File", "Read Sequential", "Write Sequential", "Make File",
		"Rename File",
		"Get Login Vector", "Get Current Disk", "Set DMA Address", "Get Alloc", "Write Protect Disk", "Get R/O Vector",
		"Set File Attr", "Get Disk Params",
		"Get/Set User", "Read Random", "Write Random", "Get File Size", "Set Random Record", "Reset Drive", "N/A", "N/A",
		"Write Random 0 fill"
	};

	if (ch < 41) {
		sprintf((char *)LogBuffer, "\nBdos call: %3d/%02xh (%s) IN from 0x%04x:\n", ch, ch, CPMCalls[ch], _RamRead16(SP) - 3);
		_sys_logbuffer(LogBuffer);
	} else {
		sprintf((char *)LogBuffer, "\nBdos call: %3d/%02xh IN from 0x%04x:\n", ch, ch, _RamRead16(SP) - 3);
		_sys_logbuffer(LogBuffer);
	}
	_logRegs();

	switch (ch) {
		case 2:
		case 4:
		case 5:
		case 6: {
			_logChar("E", LOW_REGISTER(DE));
			break;
		}

		case 9:
		case 10: {
			address = DE;
			size = 8;
			break;
		}

		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 22:
		case 23:
		case 30:
		case 35:
		case 36: {
			address = DE;
			size = 3;
			break;
		}

		case 20:
		case 21:
		case 33:
		case 34:
		case 40: {
			address = DE;
			size = 3;
			_logMem(address, size);
			sprintf((char *)LogBuffer, "\n");
			_sys_logbuffer(LogBuffer);
			address = dmaAddr;
			size = 8;
			break;
		}

		default: {
			break;
		}
	} // switch
	if (size) {
		_logMem(address, size);
	}
} // _logBdosIn

void _logBdosOut(uint8 ch) {
#ifdef LOGBDOS_NOT
	if (ch == LOGBDOS_NOT) {
		return;
	}
#endif // ifdef LOGBDOS_NOT
#ifdef LOGBDOS_ONLY
	if (ch != LOGBDOS_ONLY) {
		return;
	}
#endif // ifdef LOGBDOS_ONLY
	uint16 address = 0;
	uint8 size = 0;

	sprintf((char *)LogBuffer, "              OUT:\n");
	_sys_logbuffer(LogBuffer);
	_logRegs();

	switch (ch) {
		case 1:
		case 3:
		case 6: {
			_logChar("A", HIGH_REGISTER(AF));
			break;
		}

		case 10: {
			address = DE;
			size = 8;
			break;
		}

		case 20:
		case 21:
		case 33:
		case 34:
		case 40: {
			address = DE;
			size = 3;
			_logMem(address, size);
			sprintf((char *)LogBuffer, "\n");
			_sys_logbuffer(LogBuffer);
			address = dmaAddr;
			size = 8;
			break;
		}

		case 26: {
			address = dmaAddr;
			size = 8;
			break;
		}

		case 35:
		case 36: {
			address = DE;
			size = 3;
			break;
		}

		default: {
			break;
		}
	} // switch
	if (size) {
		_logMem(address, size);
	}
} // _logBdosOut
#endif // ifdef DEBUGLOG

void _Bios(void) {
	uint8 ch = LOW_REGISTER(PCX);
	uint8 disk[2] = {'A', 0};

#ifdef DEBUGLOG
	_logBiosIn(ch);
#endif

	switch (ch) {
		case 0x00: {
			Status = 1; // 0 - BOOT - Ends RunCPM
			break;
		}

		case 0x03: {
			Status = 2; // 1 - WBOOT - Back to CCP
			break;
		}

		case 0x06: {    // 2 - CONST - Console status
			SET_HIGH_REGISTER(AF, _chready());
			break;
		}

		case 0x09: {    // 3 - CONIN - Console input
			SET_HIGH_REGISTER(AF, _getch());
#ifdef DEBUG
			if (HIGH_REGISTER(AF) == 4) {
				Debug = 1;
			}
#endif // ifdef DEBUG
			break;
		}

		case 0x0C: {    // 4 - CONOUT - Console output
			_putcon(LOW_REGISTER(BC));
			break;
		}

		case 0x0F: {    // 5 - LIST - List output
			break;
		}

		case 0x12: {    // 6 - PUNCH/AUXOUT - Punch output
			break;
		}

		case 0x15: {    // 7 - READER - Reader input (0x1a = device not implemented)
			SET_HIGH_REGISTER(AF, 0x1a);
			break;
		}

		case 0x18: {    // 8 - HOME - Home disk head
			break;
		}

		case 0x1B: {    // 9 - SELDSK - Select disk drive
			disk[0] += LOW_REGISTER(BC);
			if (_sys_select(&disk[0])) {
				HL = DPHaddr;
			} else {
				HL = 0x0000;
			}
			break;
		}

		case 0x1E: {    // 10 - SETTRK - Set track number
			break;
		}

		case 0x21: {    // 11 - SETSEC - Set sector number
			break;
		}

		case 0x24: {    // 12 - SETDMA - Set DMA address
			HL = BC;
			dmaAddr = BC;
			break;
		}

		case 0x27: {    // 13 - READ - Read selected sector
			SET_HIGH_REGISTER(AF, 0x00);
			break;
		}

		case 0x2A: {    // 14 - WRITE - Write selected sector
			SET_HIGH_REGISTER(AF, 0x00);
			break;
		}

		case 0x2D: {    // 15 - LISTST - Get list device status
			SET_HIGH_REGISTER(AF, 0x0ff);
			break;
		}

		case 0x30: {    // 16 - SECTRAN - Sector translate
			HL = BC;    // HL=BC=No translation (1:1)
			break;
		}

		case 0x33: {    // 17 - RETTOCCP - This allows programs ending in RET return to internal CCP
			Status = 3;
			break;
		}

		default: {
#ifdef DEBUG    // Show unimplemented BIOS calls only when debugging
			_puts(	"\r\nUnimplemented BIOS call.\r\n");
			_puts(	"C = 0x");
			_puthex8(ch);
			_puts("\r\n");
#endif // ifdef DEBUG
			break;
		}
	} // switch
#ifdef DEBUGLOG
	_logBiosOut(ch);
#endif
} // _Bios

void _Bdos(void) {
	uint16 i;
	uint8 j, chr, ch = LOW_REGISTER(BC);

#ifdef DEBUGLOG
	_logBdosIn(ch);
#endif

	HL = 0x0000;                            // HL is reset by the BDOS
	SET_LOW_REGISTER(BC, LOW_REGISTER(DE)); // C ends up equal to E

	switch (ch) {
		/*
		   C = 0 : System reset
		   Doesn't return. Reloads CP/M
		 */
		case F_BOOT: {
			Status = 2; // Same as call to "BOOT"
			break;
		}

		/*
		   C = 1 : Console input
		   Gets a char from the console
		   Returns: A=Char
		 */
		case C_READ: {
			HL = _getche();
#ifdef DEBUG
			if (HL == 4) {
				Debug = 1;
			}
#endif // ifdef DEBUG
			break;
		}

		/*
		   C = 2 : Console output
		   E = Char
		   Sends the char in E to the console
		 */
		case C_WRITE: {
			_putcon(LOW_REGISTER(DE));
			break;
		}

		/*
		   C = 3 : Auxiliary (Reader) input
		   Returns: A=Char
		 */
		case READER_IN: {
			HL = 0x1a;
			break;
		}

		/*
		   C = 4 : Auxiliary (Punch) output
		 */
		case PUNCH_OUT: {
#ifdef USE_PUN
			if (!pun_open) {
				pun_dev = _sys_fopen_w((uint8 *)pun_file);
				pun_open = TRUE;
			}
			if (pun_dev) {
				_sys_fputc(LOW_REGISTER(DE), pun_dev);
			}
#endif // ifdef USE_PUN
			break;
		}

		/*
		   C = 5 : Printer output
		 */
		case PRINT_OUT: {
#ifdef USE_LST
			if (!lst_open) {
				lst_dev = _sys_fopen_w((uint8 *)lst_file);
				lst_open = TRUE;
			}
			if (lst_dev) {
				_sys_fputc(LOW_REGISTER(DE), lst_dev);
			}
#endif // ifdef USE_LST
			break;
		}

		/*
		   C = 6 : Direct console IO
		   E = 0xFF : Checks for char available and returns it, or 0x00 if none (read)
		   E = char : Outputs char (write)
		   Returns: A=Char or 0x00 (on read)
		 */
		case DIRECT_IO: {
			if (LOW_REGISTER(DE) == 0xff) {
				HL = _getchNB();
#ifdef DEBUG
				if (HL == 4) {
					Debug = 1;
				}
#endif // ifdef DEBUG
			} else {
				_putcon(LOW_REGISTER(DE));
			}
			break;
		}

		/*
		   C = 7 : Get IOBYTE
		   Gets the system IOBYTE
		   Returns: A = IOBYTE
		 */
		case GET_IOBYTE: {
			HL = _RamRead(0x0003);
			break;
		}

		/*
		   C = 8 : Set IOBYTE
		   E = IOBYTE
		   Sets the system IOBYTE to E
		 */
		case SET_IOBYTE: {
			_RamWrite(0x0003, LOW_REGISTER(DE));
			break;
		}

		/*
		   C = 9 : Output string
		   DE = Address of string
		   Sends the $ terminated string pointed by (DE) to the screen
		 */
		case OUT_STRING: {
			while ((ch = _RamRead(DE++)) != '$') {
				_putcon(ch);
			}
			break;
		}

		/*
		   C = 10 (0Ah) : Buffered input
		   DE = Address of buffer
		   Reads (DE) bytes from the console
		   Returns: A = Number os chars read
		   DE) = First char
		 */
		case C_READSTR: {
            uint16 chrsMaxIdx = WORD16(DE);                 //index to max number of characters
            uint16 chrsCntIdx = (chrsMaxIdx + 1) & 0xFFFF;  //index to number of characters read
            uint16 chrsIdx = (chrsCntIdx + 1) & 0xFFFF;     //index to characters
            //printf("\n\r chrsMaxIdx: %0X, chrsCntIdx: %0X", chrsMaxIdx, chrsCntIdx);

            static uint8 *last = 0;
            if (!last) {
                last = calloc(1, 256);    //allocate one (for now!)
            }

#ifdef PROFILE
			if (time_start != 0) {
				time_now = millis();
				printf(": %ld\n", time_now - time_start);
				time_start = 0;
			}
#endif // ifdef PROFILE
            uint8 chrsMax = _RamRead(chrsMaxIdx);   // Gets the max number of characters that can be read
            uint8 chrsCnt = 0;                      // this is the number of characters read
            uint8 curCol = 0;                       //this is the cursor column (relative to where it started)

            while (chrsMax) {
                // pre-backspace, retype & post backspace counts
                uint8 preBS = 0, reType = 0, postBS = 0;

                chr = _getch(); //input a character

                if (chr == 1) {                             // ^A - Move cursor one character to the left
                    if (curCol > 0) {
                        preBS++;            //backspace one
                    } else {
                        _putcon('\007');    //ring the bell
                    }
                }

                if (chr == 2) {                             // ^B - Toggle between beginning & end of line
                    if (curCol) {
                        preBS = curCol;             //move to beginning
                    } else {
                        reType = chrsCnt - curCol;  //move to EOL
                    }
                }

                if ((chr == 3) && (chrsCnt == 0)) {         // ^C - Abort string input
                    _puts("^C");
                    Status = 2;
                    break;
                }

#ifdef DEBUG
                if (chr == 4) {                             // ^D - DEBUG
                    Debug = 1

                    printf("\r\n curCol: %u, chrsCnt: %u, chrsMax: %u", curCol, chrsCnt, chrsMax);
                    _puts("#\r\n  ");
                    reType = chrsCnt;
                    postBS = chrsCnt - curCol;
                }
#endif // ifdef DEBUG

                if (chr == 5) {                             // ^E - goto beginning of next line
                    _puts("\n");
                    preBS = curCol;
                    reType = postBS = chrsCnt;
                }

                if (chr == 6) {                             // ^F - Move the cursor one character forward
                    if (curCol < chrsCnt) {
                        reType++;
                    } else {
                        _putcon('\007');  //ring the bell
                    }
                }

                if (chr == 7) {                             // ^G - Delete character at cursor
                    if (curCol < chrsCnt) {
                        //delete this character from buffer
                        for (i = curCol, j = i + 1; j < chrsCnt; i++, j++) {
                            ch = _RamRead(((chrsIdx + j) & 0xFFFF));
                            _RamWrite((chrsIdx + i) & 0xFFFF, ch);
                        }
                        reType = postBS = chrsCnt - curCol;
                        chrsCnt--;
                    } else {
                        _putcon('\007');  //ring the bell
                    }
                }

                if (((chr == 0x08) || (chr == 0x7F))) {     // ^H and DEL - Delete one character to left of cursor
                    if (curCol > 0) {   //not at BOL
                        if (curCol < chrsCnt) { //not at EOL
                            //delete previous character from buffer
                            for (i = curCol, j = i - 1; i < chrsCnt; i++, j++) {
                                ch = _RamRead(((chrsIdx + i) & 0xFFFF));
                                _RamWrite((chrsIdx + j) & 0xFFFF, ch);
                            }
                            preBS++;    //pre-backspace one
                            //note: does one extra to erase EOL
                            reType = postBS = chrsCnt - curCol + 1;
                        } else {
                            preBS = reType = postBS = 1;
                        }
                        chrsCnt--;
                    } else {
                        _putcon('\007');  //ring the bell
                    }
                }

                if ((chr == 0x0A) || (chr == 0x0D)) {   // ^J and ^M - Ends editing
#ifdef PROFILE
                    time_start = millis();
#endif
                    break;
                }

                if (chr == 0x0B) {                      // ^K - Delete to EOL from cursor
                    if (curCol < chrsCnt) {
                        reType = postBS = chrsCnt - curCol;
                        chrsCnt = curCol;   //truncate buffer to here
                    } else {
                        _putcon('\007');  //ring the bell
                    }
                }

                if (chr == 18) {                        // ^R - Retype the command line
                    _puts("#\b\n");
                    preBS = curCol;             //backspace to BOL
                    reType = chrsCnt;           //retype everything
                    postBS = chrsCnt - curCol;  //backspace to cursor column
                }

                if (chr == 21) {                        // ^U - delete all characters
                    _puts("#\b\n");
                    preBS = curCol; //backspace to BOL
                    chrsCnt = 0;
                }

                if (chr == 23) {                        // ^W - recall last command
                    if (!curCol) {      //if at beginning of command line
                        if (last[0]) {  //and there's a last command
                            //restore last command
                            for (j = 0; j <= chrsCnt; j++) {
                                _RamWrite((chrsCntIdx + j) & 0xFFFF, last[j]);
                            }
                            //retype to greater of chrsCnt & last[0]
                            reType = (chrsCnt > last[0]) ? chrsCnt : last[0];
                            chrsCnt = last[0];  //this is the restored length
                            //backspace to end of restored command
                            postBS = reType - chrsCnt;
                        } else {
                            _putcon('\007');  //ring the bell
                        }
                    } else if (curCol < chrsCnt) {  //if not at EOL
                        reType = chrsCnt - curCol;  //move to EOL
                    }
                }

                if (chr == 24) {                        // ^X - delete all character left of the cursor
                    if (curCol > 0) {
                        //move rest of line to beginning of line
                        for (i = 0, j = curCol; j < chrsCnt;i++, j++) {
                            ch = _RamRead(((chrsIdx + j) & 0xFFFF));
                            _RamWrite((chrsIdx +i) & 0xFFFF, ch);
                        }
                        preBS = curCol;
                        reType = chrsCnt;
                        postBS = chrsCnt;
                        chrsCnt -= curCol;
                    } else {
                        _putcon('\007');  //ring the bell
                    }
                }

                if ((chr >= 0x20) && (chr <= 0x7E)) { //valid character
                    if (curCol < chrsCnt) {
                        //move rest of buffer one character right
                        for (i = chrsCnt, j = i - 1; i > curCol; i--, j--) {
                            ch = _RamRead(((chrsIdx + j) & 0xFFFF));
                            _RamWrite((chrsIdx + i) & 0xFFFF, ch);
                        }
                    }
                    //put the new character in the buffer
                    _RamWrite((chrsIdx + curCol) & 0xffff, chr);

                    chrsCnt++;
                    reType = chrsCnt - curCol;
                    postBS = reType - 1;
                }

                //pre-backspace
                for (i = 0; i < preBS; i++) {
                    _putcon('\b');
                    curCol--;
                }

                //retype
                for (i = 0; i < reType; i++) {
                    if (curCol < chrsCnt) {
                        ch = _RamRead(((chrsIdx + curCol) & 0xFFFF));
                    } else {
                        ch = ' ';
                    }
                    _putcon(ch);
                    curCol++;
                }

                //post-backspace
                for (i = 0; i < postBS; i++) {
                    _putcon('\b');
                    curCol--;
                }

                if (chrsCnt == chrsMax) {   // Reached the maximum count
                    break;
                }
            }   // while (chrsMax)

            // Save the number of characters read
            _RamWrite(chrsCntIdx, chrsCnt);

            //if there are characters...
            if (chrsCnt) {
                //... then save this as last command
                for (j = 0; j <= chrsCnt; j++) {
                    last[j] = _RamRead((chrsCntIdx + j) & 0xFFFF);
                }
            }
#if 0
            printf("\n\r chrsMaxIdx: %0X, chrsMax: %u, chrsCnt: %u", chrsMaxIdx, chrsMax, chrsCnt);
            for (j = 0; j < chrsCnt + 2; j++) {
                printf("\n\r chrsMaxIdx[%u]: %0.2x", j, last[j]);
            }
#endif
            _putcon('\r');          // Gives a visual feedback that read ended
			break;
		}

		/*
		   C = 11 (0Bh) : Get console status
		   Returns: A=0x00 or 0xFF
		 */
		case C_STAT: {
			HL = _chready();
			break;
		}

		/*
		   C = 12 (0Ch) : Get version number
		   Returns: B=H=system type, A=L=version number
		 */
		case GET_VERSION: {
			HL = 0x22;
			break;
		}

		/*
		   C = 13 (0Dh) : Reset disk system
		 */
		case DRV_ALLRESET: {
			roVector = 0;       // Make all drives R/W
			loginVector = 0;
			dmaAddr = 0x0080;
			cDrive = 0;         // userCode remains unchanged
			HL = _CheckSUB();   // Checks if there's a $$$.SUB on the boot disk
			break;
		}

		/*
		   C = 14 (0Eh) : Select Disk
		   Returns: A=0x00 or 0xFF
		 */
		case DRV_SET: {
			oDrive = cDrive;
			cDrive = LOW_REGISTER(DE);
			HL = _SelectDisk(LOW_REGISTER(DE) + 1); // +1 here is to allow SelectDisk to be used directly by disk.h as well
			if (!HL) {
				oDrive = cDrive;
			} else {
				if ((_RamRead(DSKByte) & 0x0f) == cDrive) {
					cDrive = oDrive = 0;
					_RamWrite(DSKByte, _RamRead(DSKByte) & 0xf0);
				} else {
					cDrive = oDrive;
				}
			}
			break;
		}

		/*
		   C = 15 (0Fh) : Open file
		   Returns: A=0x00 or 0xFF
		 */
		case F_OPEN: {
			HL = _OpenFile(DE);
			break;
		}

		/*
		   C = 16 (10h) : Close file
		 */
		case F_CLOSE: {
			HL = _CloseFile(DE);
			break;
		}

		/*
		   C = 17 (11h) : Search for first
		 */
		case F_SEARCH_FIRST: {
			HL = _SearchFirst(DE, TRUE);    // TRUE = Creates a fake dir entry when finding the file
			break;
		}

		/*
		   C = 18 (12h) : Search for next
		 */
		case F_SEARCH_NEXT: {
			HL = _SearchNext(DE, TRUE); // TRUE = Creates a fake dir entry when finding the file
			break;
		}

		/*
		   C = 19 (13h) : Delete file
		 */
		case F_DELETE: {
			HL = _DeleteFile(DE);
			break;
		}

		/*
		   C = 20 (14h) : Read sequential
		 */
		case F_READ: {
			HL = _ReadSeq(DE);
			break;
		}

		/*
		   C = 21 (15h) : Write sequential
		 */
		case F_WRITE: {
			HL = _WriteSeq(DE);
			break;
		}

		/*
		   C = 22 (16h) : Make file
		 */
		case F_MAKE: {
			HL = _MakeFile(DE);
			break;
		}

		/*
		   C = 23 (17h) : Rename file
		 */
		case F_RENAME: {
			HL = _RenameFile(DE);
			break;
		}

		/*
		   C = 24 (18h) : Return log-in vector (active drive map)
		 */
		case DRV_LOGINVECTOR: {
			HL = loginVector;   // (todo) improve this
			break;
		}

		/*
		   C = 25 (19h) : Return current disk
		 */
		case DRV_GET: {
			HL = cDrive;
			break;
		}

		/*
		   C = 26 (1Ah) : Set DMA address
		 */
		case F_DMAOFF: {
			dmaAddr = DE;
			break;
		}

		/*
		   C = 27 (1Bh) : Get ADDR(Alloc)
		 */
		case DRV_GETADDRALLOC: {
			HL = SCBaddr;
			break;
		}

		/*
		   C = 28 (1Ch) : Write protect current disk
		 */
		case DRV_WRITEPROTECT: {
			roVector = roVector | (1 << cDrive);
			break;
		}

		/*
		   C = 29 (1Dh) : Get R/O vector
		 */
		case DRV_GETROVECTOR: {
			HL = roVector;
			break;
		}

		/*
		   C = 30 (1Eh) : Set file attributes (does nothing)
		 */
		case F_SETATTRIBUTES: {
			HL = 0;
			break;
		}

		/*
		   C = 31 (1Fh) : Get ADDR(Disk Parms)
		 */
		case DRV_GETDPB: {
			HL = DPBaddr;
			break;
		}

		/*
		   C = 32 (20h) : Get/Set user code
		 */
		case F_USERNUM: {
			if (LOW_REGISTER(DE) == 0xFF) {
				HL = userCode;
			} else {
				_SetUser(DE);
			}
			break;
		}

		/*
		   C = 33 (21h) : Read random
		 */
		case F_READRANDOM: {
			HL = _ReadRand(DE);
			break;
		}

		/*
		   C = 34 (22h) : Write random
		 */
		case F_WRITERANDOM: {
			HL = _WriteRand(DE);
			break;
		}

		/*
		   C = 35 (23h) : Compute file size
		 */
		case F_COMPUTESIZE: {
			HL = _GetFileSize(DE);
			break;
		}

		/*
		   C = 36 (24h) : Set random record
		 */
		case F_SETRANDOM: {
			HL = _SetRandom(DE);
			break;
		}

		/*
		   C = 37 (25h) : Reset drive
		 */
		case DRV_RESET: {
			roVector = roVector & ~DE;
			break;
		}

		/* ********* Function 38: Not supported by CP/M 2.2 *********
		  ********* Function 39: Not supported by CP/M 2.2 *********
		  ********* (todo) Function 40: Write random with zero fill *********
		 */

		/*
		   C = 40 (28h) : Write random with zero fill (we have no disk blocks, so just write random)
		 */
		case F_WRITERANDOMZERO: {
			HL = _WriteRand(DE);
			break;
		}

#if defined board_digital_io

		/*
		   C = 220 (DCh) : PinMode
		 */
		case 220: {
			pinMode(HIGH_REGISTER(DE), LOW_REGISTER(DE));
			break;
		}

		/*
		   C = 221 (DDh) : DigitalRead
		 */
		case 221: {
			HL = digitalRead(HIGH_REGISTER(DE));
			break;
		}

		/*
		   C = 222 (DEh) : DigitalWrite
		 */
		case 222: {
			digitalWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
			break;
		}

		/*
		   C = 223 (DFh) : AnalogRead
		 */
		case 223: {
			HL = analogRead(HIGH_REGISTER(DE));
			break;
		}

#endif // if defined board_digital_io
#if defined board_analog_io

		/*
		   C = 224 (E0h) : AnalogWrite
		 */
		case 224: {
			analogWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
			break;
		}

#endif // if defined board_analog_io

		/*
		   C = 230 (E6h) : Set 8 bit masking
		 */
		case 230: {
			mask8bit = LOW_REGISTER(DE);
			break;
		}

		/*
		   C = 231 (E7h) : Host specific BDOS call
		 */
		case 231: {
			HL = hostbdos(DE);
			break;
		}

			/*
			   C = 232 (E8h) : ESP32 specific BDOS call
			 */
#if defined board_esp32
		case 232: {
			HL = esp32bdos(DE);
			break;
		}

#endif // if defined board_esp32
#if defined board_stm32
		case 232: {
			HL = stm32bdos(DE);
			break;
		}

#endif // if defined board_stm32

		/*
		   C = 249 (F9h) : MakeDisk
		   Makes a disk directory if not existent.
		 */
		case 249: {
			HL = _MakeDisk(DE);
			break;
		}

		/*
		   C = 250 (FAh) : HostOS
		   Returns: A = 0x00 - Windows / 0x01 - Arduino / 0x02 - Posix / 0x03 - Dos / 0x04 - Teensy / 0x05 - ESP32 / 0x06 - STM32
		 */
		case 250: {
			HL = HostOS;
			break;
		}

		/*
		   C = 251 (FBh) : Version
		   Returns: A = 0xVv - Version in BCD representation: V.v
		 */
		case 251: {
			HL = VersionBCD;
			break;
		}

		/*
		   C = 252 (FCh) : CCP version
		   Returns: A = 0x00-0x04 = DRI|CCPZ|ZCPR2|ZCPR3|Z80CCP / 0xVv = Internal version in BCD: V.v
		 */
		case 252: {
			HL = VersionCCP;
			break;
		}

		/*
		   C = 253 (FDh) : CCP address
		 */
		case 253: {
			HL = CCPaddr;
			break;
		}

#ifdef HASLUA

		/*
		   C = 254 (FEh) : Run Lua file
		 */
		case 254: {
			HL = _RunLua(DE);
			break;
		}

#endif // ifdef HASLUA

		/*
		   Unimplemented calls get listed
		 */
		default: {
#ifdef DEBUG    // Show unimplemented BDOS calls only when debugging
			_puts(	"\r\nUnimplemented BDOS call.\r\n");
			_puts(	"C = 0x");
			_puthex8(ch);
			_puts("\r\n");
#endif // ifdef DEBUG
			break;
		}
	} // switch

	// CP/M BDOS does this before returning
	SET_HIGH_REGISTER(	BC, HIGH_REGISTER(HL));
	SET_HIGH_REGISTER(	AF, LOW_REGISTER(HL));

#ifdef DEBUGLOG
	_logBdosOut(ch);
#endif
} // _Bdos

#endif // ifndef CPM_H

