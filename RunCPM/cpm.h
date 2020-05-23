#ifndef CPM_H
#define CPM_H

/* see main.c for definition */

#define JP		0xc3
#define CALL	0xcd
#define RET		0xc9
#define INa		0xdb	// Triggers a BIOS call
#define OUTa	0xd3	// Triggers a BDOS call

/* set up full PUN and LST filenames to be on drive A: user 0 */
#ifdef USE_PUN
char pun_file[17] = { 'A',FOLDERCHAR,'0',FOLDERCHAR,'P','U','N','.','T','X','T',0 };
#endif
#ifdef USE_LST
char lst_file[17] = { 'A',FOLDERCHAR,'0',FOLDERCHAR,'L','S','T','.','T','X','T',0 };
#endif

#ifdef PROFILE
unsigned long time_start = 0;
unsigned long time_now = 0;
#endif

void _PatchCPM(void) {
	uint16 i;

	//**********  Patch CP/M page zero into the memory  **********

	/* BIOS entry point */
	_RamWrite(0x0000, JP);		/* JP BIOS+3 (warm boot) */
	_RamWrite16(0x0001, BIOSjmppage + 3);

	if (Status != 2) {
		/* IOBYTE - Points to Console */
		_RamWrite(0x0003, 0x3D);
		/* Current drive/user - A:/0 */
		_RamWrite(0x0004, 0x00);
	}

	/* BDOS entry point (0x0005) */
	_RamWrite(0x0005, JP);
	_RamWrite16(0x0006, BDOSjmppage + 0x06);

	//**********  Patch CP/M Version into the memory so the CCP can see it
	_RamWrite16(BDOSjmppage, 0x1600);
	_RamWrite16(BDOSjmppage + 2, 0x0000);
	_RamWrite16(BDOSjmppage + 4, 0x0000);

	// Patches in the BDOS jump destination
	_RamWrite(BDOSjmppage + 6, JP);
	_RamWrite16(BDOSjmppage + 7, BDOSpage);

	// Patches in the BDOS page content
	_RamWrite(BDOSpage, INa);
	_RamWrite(BDOSpage + 1, 0x00);
	_RamWrite(BDOSpage + 2, RET);

	// Patches in the BIOS jump destinations
	for (i = 0; i < 0x36; i = i + 3) {
		_RamWrite(BIOSjmppage + i, JP);
		_RamWrite16(BIOSjmppage + i + 1, BIOSpage + i);
	}

	// Patches in the BIOS page content
	for (i = 0; i < 0x36; i = i + 3) {
		_RamWrite(BIOSpage + i, OUTa);
		_RamWrite(BIOSpage + i + 1, i & 0xff);
		_RamWrite(BIOSpage + i + 2, RET);
	}

	//**********  Patch CP/M (fake) Disk Paramater Table after the BDOS call entry  **********
	i = DPBaddr;
	_RamWrite(i++, 64);  		/* spt - Sectors Per Track */
	_RamWrite(i++, 0);
	_RamWrite(i++, 5);   		/* bsh - Data allocation "Block Shift Factor" */
	_RamWrite(i++, 0x1F);		/* blm - Data allocation Block Mask */
	_RamWrite(i++, 1);   		/* exm - Extent Mask */
	_RamWrite(i++, 0xFF);		/* dsm - Total storage capacity of the disk drive */
	_RamWrite(i++, 0x07);
	_RamWrite(i++, 255); 		/* drm - Number of the last directory entry */
	_RamWrite(i++, 3);
	_RamWrite(i++, 0xFF);		/* al0 */
	_RamWrite(i++, 0x00);		/* al1 */
	_RamWrite(i++, 0);   		/* cks - Check area Size */
	_RamWrite(i++, 0);
	_RamWrite(i++, 0x02);		/* off - Number of system reserved tracks at the beginning of the ( logical ) disk */
	_RamWrite(i++, 0x00);
	blockShift = _RamRead(DPBaddr + 2);
	blockMask = _RamRead(DPBaddr + 3);
	extentMask = _RamRead(DPBaddr + 4);
	numAllocBlocks = _RamRead16((DPBaddr + 5)) + 1;
	extentsPerDirEntry = extentMask + 1;

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
}

#ifdef DEBUGLOG
uint8 LogBuffer[128];

void _logRegs(void) {
	uint8 J, I;
	uint8 Flags[9] = { 'S','Z','5','H','3','P','N','C' };
	uint8 c = HIGH_REGISTER(AF);
	if (c < 32 || c > 126)
		c = 46;
	for (J = 0, I = LOW_REGISTER(AF); J < 8; ++J, I <<= 1) Flags[J] = I & 0x80 ? Flags[J] : '.';
	sprintf((char*)LogBuffer, "  BC:%04x DE:%04x HL:%04x AF:%02x(%c)|%s| IX:%04x IY:%04x SP:%04x PC:%04x\n",
		WORD16(BC), WORD16(DE), WORD16(HL), HIGH_REGISTER(AF), c, Flags, WORD16(IX), WORD16(IY), WORD16(SP), WORD16(PC)); _sys_logbuffer(LogBuffer);
}

void _logMem(uint16 address, uint8 amount)	// Amount = number of 16 bytes lines, so 1 CP/M block = 8, not 128
{
	uint8 i, m, c, pos;
	uint8 head = 8;
	uint8 hexa[] = "0123456789ABCDEF";
	for (i = 0; i < amount; ++i) {
		pos = 0;
		for (m = 0; m < head; ++m)
			LogBuffer[pos++] = ' ';
		sprintf((char*)LogBuffer, "  %04x: ", address);
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
}

void _logChar(char* txt, uint8 c) {
	uint8 asc[2];

	asc[0] = c > 31 && c < 127 ? c : '.';
	asc[1] = 0;
	sprintf((char*)LogBuffer, "        %s = %02xh:%3d (%s)\n", txt, c, c, asc);
	_sys_logbuffer(LogBuffer);
}

void _logBiosIn(uint8 ch) {
	static const char* BIOSCalls[18] =
	{
		"boot", "wboot", "const", "conin", "conout", "list", "punch/aux", "reader", "home", "seldsk", "settrk", "setsec", "setdma",
		"read", "write", "listst", "sectran", "altwboot"
	};
	int index = ch / 3;
	if (index < 18) {
		sprintf((char*)LogBuffer, "\nBios call: %3d/%02xh (%s) IN:\n", ch, ch, BIOSCalls[index]); _sys_logbuffer(LogBuffer);
	} else {
		sprintf((char*)LogBuffer, "\nBios call: %3d/%02xh IN:\n", ch, ch); _sys_logbuffer(LogBuffer);
	}

	_logRegs();
}

void _logBiosOut(uint8 ch) {
	sprintf((char*)LogBuffer, "               OUT:\n"); _sys_logbuffer(LogBuffer);
	_logRegs();
}

void _logBdosIn(uint8 ch) {
	uint16 address = 0;
	uint8 size = 0;

	static const char* CPMCalls[41] =
	{
		"System Reset", "Console Input", "Console Output", "Reader Input", "Punch Output", "List Output", "Direct I/O", "Get IOByte",
		"Set IOByte", "Print String", "Read Buffered", "Console Status", "Get Version", "Reset Disk", "Select Disk", "Open File",
		"Close File", "Search First", "Search Next", "Delete File", "Read Sequential", "Write Sequential", "Make File", "Rename File",
		"Get Login Vector", "Get Current Disk", "Set DMA Address", "Get Alloc", "Write Protect Disk", "Get R/O Vector", "Set File Attr", "Get Disk Params",
		"Get/Set User", "Read Random", "Write Random", "Get File Size", "Set Random Record", "Reset Drive", "N/A", "N/A", "Write Random 0 fill"
	};

	if (ch < 41) {
		sprintf((char*)LogBuffer, "\nBdos call: %3d/%02xh (%s) IN from 0x%04x:\n", ch, ch, CPMCalls[ch], _RamRead16(SP) - 3); _sys_logbuffer(LogBuffer);
	} else {
		sprintf((char*)LogBuffer, "\nBdos call: %3d/%02xh IN from 0x%04x:\n", ch, ch, _RamRead16(SP) - 3); _sys_logbuffer(LogBuffer);
	}
	_logRegs();
	switch (ch) {
	case 2:
	case 4:
	case 5:
	case 6:
		_logChar("E", LOW_REGISTER(DE)); break;
	case 9:
	case 10:
		address = DE; size = 8; break;
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 22:
	case 23:
	case 30:
	case 35:
	case 36:
		address = DE; size = 3; break;
	case 20:
	case 21:
	case 33:
	case 34:
	case 40:
		address = DE; size = 3; _logMem(address, size);
		sprintf((char*)LogBuffer, "\n");  _sys_logbuffer(LogBuffer);
		address = dmaAddr; size = 8; break;
	default:
		break;
	}
	if (size)
		_logMem(address, size);
}

void _logBdosOut(uint8 ch) {
	uint16 address = 0;
	uint8 size = 0;

	sprintf((char*)LogBuffer, "              OUT:\n"); _sys_logbuffer(LogBuffer);
	_logRegs();
	switch (ch) {
	case 1:
	case 3:
	case 6:
		_logChar("A", HIGH_REGISTER(AF)); break;
	case 10:
		address = DE; size = 8; break;
	case 20:
	case 21:
	case 33:
	case 34:
	case 40:
		address = DE; size = 3; _logMem(address, size);
		sprintf((char*)LogBuffer, "\n");  _sys_logbuffer(LogBuffer);
		address = dmaAddr; size = 8; break;
	case 26:
		address = dmaAddr; size = 8; break;
	case 35:
	case 36:
		address = DE; size = 3; break;
	default:
		break;
	}
	if (size)
		_logMem(address, size);
}
#endif

void _Bios(void) {
	uint8 ch = LOW_REGISTER(PCX);

#ifdef DEBUGLOG
#ifdef LOGONLY
	if (ch == LOGONLY)
#endif
		_logBiosIn(ch);
#endif

	switch (ch) {
	case 0x00:
		Status = 1;			// 0 - BOOT - Ends RunCPM
		break;
	case 0x03:
		Status = 2;			// 1 - WBOOT - Back to CCP
		break;
	case 0x06:				// 2 - CONST - Console status
		SET_HIGH_REGISTER(AF, _chready());
		break;
	case 0x09:				// 3 - CONIN - Console input
		SET_HIGH_REGISTER(AF, _getch());
#ifdef DEBUG
		if (HIGH_REGISTER(AF) == 4)
			Debug = 1;
#endif
		break;
	case 0x0C:				// 4 - CONOUT - Console output
		_putcon(LOW_REGISTER(BC));
		break;
	case 0x0F:				// 5 - LIST - List output
		break;
	case 0x12:				// 6 - PUNCH/AUXOUT - Punch output
		break;
	case 0x15:				// 7 - READER - Reader input (0x1a = device not implemented)
		SET_HIGH_REGISTER(AF, 0x1a);
		break;
	case 0x18:				// 8 - HOME - Home disk head
		break;
	case 0x1B:				// 9 - SELDSK - Select disk drive
		HL = 0x0000;
		break;
	case 0x1E:				// 10 - SETTRK - Set track number
		break;
	case 0x21:				// 11 - SETSEC - Set sector number
		break;
	case 0x24:				// 12 - SETDMA - Set DMA address
		HL = BC;
		dmaAddr = BC;
		break;
	case 0x27:				// 13 - READ - Read selected sector
		SET_HIGH_REGISTER(AF, 0x00);
		break;
	case 0x2A:				// 14 - WRITE - Write selected sector
		SET_HIGH_REGISTER(AF, 0x00);
		break;
	case 0x2D:				// 15 - LISTST - Get list device status
		SET_HIGH_REGISTER(AF, 0x0ff);
		break;
	case 0x30:				// 16 - SECTRAN - Sector translate
		HL = BC;			// HL=BC=No translation (1:1)
		break;
	case 0x33:				// 17 - RETTOCCP - This allows programs ending in RET return to internal CCP
		Status = 3;
		break;
	default:
#ifdef DEBUG	// Show unimplemented BIOS calls only when debugging
		_puts("\r\nUnimplemented BIOS call.\r\n");
		_puts("C = 0x");
		_puthex8(ch);
		_puts("\r\n");
#endif
		break;
	}
#ifdef DEBUGLOG
#ifdef LOGONLY
	if (ch == LOGONLY)
#endif
		_logBiosOut(ch);
#endif

}

void _Bdos(void) {
	uint16	i;
	uint8	j, count, chr, c, ch = LOW_REGISTER(BC);

#ifdef DEBUGLOG
#ifdef LOGONLY
	if (ch == LOGONLY)
#endif
		_logBdosIn(ch);
#endif

	HL = 0x0000;	// HL is reset by the BDOS
	SET_LOW_REGISTER(BC, LOW_REGISTER(DE)); // C ends up equal to E

	switch (ch) {
		/*
		C = 0 : System reset
		Doesn't return. Reloads CP/M
		*/
	case 0:
		Status = 2;	// Same as call to "BOOT"
		break;
		/*
		C = 1 : Console input
		Gets a char from the console
		Returns: A=Char
		*/
	case 1:
		HL = _getche();
#ifdef DEBUG
		if (HL == 4)
			Debug = 1;
#endif
		break;
		/*
		C = 2 : Console output
		E = Char
		Sends the char in E to the console
		*/
	case 2:
		_putcon(LOW_REGISTER(DE));
		break;
		/*
		C = 3 : Auxiliary (Reader) input
		Returns: A=Char
		*/
	case 3:
		HL = 0x1a;
		break;
		/*
		C = 4 : Auxiliary (Punch) output
		*/
	case 4:
#ifdef USE_PUN
		if (!pun_open) {
			pun_dev = _sys_fopen_w((uint8*)pun_file);
			pun_open = TRUE;
		}
		if (pun_dev)
			_sys_fputc(LOW_REGISTER(DE), pun_dev);
#endif
		break;
		/*
		C = 5 : Printer output
		*/
	case 5:
#ifdef USE_LST
		if (!lst_open) {
			lst_dev = _sys_fopen_w((uint8*)lst_file);
			lst_open = TRUE;
		}
		if (lst_dev)
			_sys_fputc(LOW_REGISTER(DE), lst_dev);
#endif
		break;
		/*
		C = 6 : Direct console IO
		E = 0xFF : Checks for char available and returns it, or 0x00 if none (read)
		E = char : Outputs char (write)
		Returns: A=Char or 0x00 (on read)
		*/
	case 6:
		if (LOW_REGISTER(DE) == 0xff) {
			HL = _getchNB();
#ifdef DEBUG
			if (HL == 4)
				Debug = 1;
#endif
		} else {
			_putcon(LOW_REGISTER(DE));
		}
		break;
		/*
		C = 7 : Get IOBYTE
		Gets the system IOBYTE
		Returns: A = IOBYTE
		*/
	case 7:
		HL = _RamRead(0x0003);
		break;
		/*
		C = 8 : Set IOBYTE
		E = IOBYTE
		Sets the system IOBYTE to E
		*/
	case 8:
		_RamWrite(0x0003, LOW_REGISTER(DE));
		break;
		/*
		C = 9 : Output string
		DE = Address of string
		Sends the $ terminated string pointed by (DE) to the screen
		*/
	case 9:
		while ((ch = _RamRead(DE++)) != '$')
			_putcon(ch);
		break;
		/*
		C = 10 (0Ah) : Buffered input
		DE = Address of buffer
		Reads (DE) bytes from the console
		Returns: A = Number os chars read
		DE) = First char
		*/
	case 10:
#ifdef PROFILE
		if (time_start != 0) {
			time_now = millis();
			printf(": %ld\n", time_now - time_start);
			time_start = 0;
		}
#endif
		i = WORD16(DE);
		c = _RamRead(i);	// Gets the number of characters to read
		++i;	// Points to the number read
		count = 0;
		while (c)	// Very simplistic line input
		{
			chr = _getch();
			if (chr == 3 && count == 0) {						// ^C
				_puts("^C");
				Status = 2;
				break;
			}
#ifdef DEBUG
			if (chr == 4)										// ^D
				Debug = 1;
#endif
			if (chr == 5)										// ^E
				_puts("\r\n");
			if ((chr == 0x08 || chr == 0x7F) && count > 0) {	// ^H and DEL
				_puts("\b \b");
				count--;
				continue;
			}
			if (chr == 0x0A || chr == 0x0D) {					// ^J and ^M
#ifdef PROFILE
				time_start = millis();
#endif
				break;
			}
			if (chr == 18) {									// ^R
				_puts("#\r\n  ");
				for (j = 1; j <= count; ++j)
					_putcon(_RamRead(i + j));
			}
			if (chr == 21) {									// ^U
				_puts("#\r\n  ");
				i = WORD16(DE);
				c = _RamRead(i);
				++i;
				count = 0;
			}
			if (chr == 24) {									// ^X
				for (j = 0; j < count; ++j)
					_puts("\b \b");
				i = WORD16(DE);
				c = _RamRead(i);
				++i;
				count = 0;
			}
			if (chr < 0x20 || chr > 0x7E)						// Invalid character
				continue;
			_putcon(chr);
			++count; _RamWrite((i + count) & 0xffff, chr);
			if (count == c)										// Reached the expected count
				break;
		}
		_RamWrite(i & 0xffff, count);	// Saves the number of characters read
		_putcon('\r');	// Gives a visual feedback that read ended
		break;
		/*
		C = 11 (0Bh) : Get console status
		Returns: A=0x00 or 0xFF
		*/
	case 11:
		HL = _chready();
		break;
		/*
		C = 12 (0Ch) : Get version number
		Returns: B=H=system type, A=L=version number
		*/
	case 12:
		HL = 0x22;
		break;
		/*
		C = 13 (0Dh) : Reset disk system
		*/
	case 13:
		roVector = 0;		// Make all drives R/W
		loginVector = 0;
		dmaAddr = 0x0080;
		cDrive = 0;			// userCode remains unchanged
		HL = _CheckSUB();	// Checks if there's a $$$.SUB on the boot disk
		break;
		/*
		C = 14 (0Eh) : Select Disk
		Returns: A=0x00 or 0xFF
		*/
	case 14:
		oDrive = cDrive;
		cDrive = LOW_REGISTER(DE);
		HL = _SelectDisk(LOW_REGISTER(DE) + 1);	// +1 here is to allow SelectDisk to be used directly by disk.h as well
		if (!HL)
			oDrive = cDrive;
		break;
		/*
		C = 15 (0Fh) : Open file
		Returns: A=0x00 or 0xFF
		*/
	case 15:
		HL = _OpenFile(DE);
		break;
		/*
		C = 16 (10h) : Close file
		*/
	case 16:
		HL = _CloseFile(DE);
		break;
		/*
		C = 17 (11h) : Search for first
		*/
	case 17:
		HL = _SearchFirst(DE, TRUE);	// TRUE = Creates a fake dir entry when finding the file
		break;
		/*
		C = 18 (12h) : Search for next
		*/
	case 18:
		HL = _SearchNext(DE, TRUE);		// TRUE = Creates a fake dir entry when finding the file
		break;
		/*
		C = 19 (13h) : Delete file
		*/
	case 19:
		HL = _DeleteFile(DE);
		break;
		/*
		C = 20 (14h) : Read sequential
		*/
	case 20:
		HL = _ReadSeq(DE);
		break;
		/*
		C = 21 (15h) : Write sequential
		*/
	case 21:
		HL = _WriteSeq(DE);
		break;
		/*
		C = 22 (16h) : Make file
		*/
	case 22:
		HL = _MakeFile(DE);
		break;
		/*
		C = 23 (17h) : Rename file
		*/
	case 23:
		HL = _RenameFile(DE);
		break;
		/*
		C = 24 (18h) : Return log-in vector (active drive map)
		*/
	case 24:
		HL = loginVector;	// (todo) improve this
		break;
		/*
		C = 25 (19h) : Return current disk
		*/
	case 25:
		HL = cDrive;
		break;
		/*
		C = 26 (1Ah) : Set DMA address
		*/
	case 26:
		dmaAddr = DE;
		break;
		/*
		C = 27 (1Bh) : Get ADDR(Alloc)
		*/
	case 27:
		HL = SCBaddr;
		break;
		/*
		C = 28 (1Ch) : Write protect current disk
		*/
	case 28:
		roVector = roVector | (1 << cDrive);
		break;
		/*
		C = 29 (1Dh) : Get R/O vector
		*/
	case 29:
		HL = roVector;
		break;
		/*
		C = 30 (1Eh) : Set file attributes (does nothing)
		*/
	case 30:
		HL = 0;
		break;
		/*
		C = 31 (1Fh) : Get ADDR(Disk Parms)
		*/
	case 31:
		HL = DPBaddr;
		break;
		/*
		C = 32 (20h) : Get/Set user code
		*/
	case 32:
		if (LOW_REGISTER(DE) == 0xFF) {
			HL = userCode;
		} else {
			_SetUser(DE);
		}
		break;
		/*
		C = 33 (21h) : Read random
		*/
	case 33:
		HL = _ReadRand(DE);
		break;
		/*
		C = 34 (22h) : Write random
		*/
	case 34:
		HL = _WriteRand(DE);
		break;
		/*
		C = 35 (23h) : Compute file size
		*/
	case 35:
		HL = _GetFileSize(DE);
		break;
		/*
		C = 36 (24h) : Set random record
		*/
	case 36:
		HL = _SetRandom(DE);
		break;
		/*
		C = 37 (25h) : Reset drive
		*/
	case 37:
		break;
		/********** Function 38: Not supported by CP/M 2.2 **********/
		/********** Function 39: Not supported by CP/M 2.2 **********/
		/********** (todo) Function 40: Write random with zero fill **********/
		/*
		C = 40 (28h) : Write random with zero fill (we have no disk blocks, so just write random)
		*/
	case 40:
		HL = _WriteRand(DE);
		break;
#if defined board_digital_io
		/*
		C = 220 (DCh) : PinMode
		*/
	case 220:
		pinMode(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
		/*
		C = 221 (DDh) : DigitalRead
		*/
	case 221:
		HL = digitalRead(HIGH_REGISTER(DE));
		break;
		/*
		C = 222 (DEh) : DigitalWrite
		*/
	case 222:
		digitalWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
		/*
		C = 223 (DFh) : AnalogRead
		*/
	case 223:
		HL = analogRead(HIGH_REGISTER(DE));
		break;
#endif
#if defined board_analog_io
		/*
		C = 224 (E0h) : AnalogWrite
		*/
	case 224:
		analogWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
#endif
		/*
		C = 230 (E6h) : Set 8 bit masking
		*/
	case 230:
		mask8bit = LOW_REGISTER(DE);
		break;
		/*
		C = 231 (E7h) : Host specific BDOS call
		*/
	case 231:
		HL = hostbdos(DE);
		break;
		/*
		C = 232 (E8h) : ESP32 specific BDOS call
		*/
#if defined board_esp32
	case 232:
		HL = esp32bdos(DE);
		break;
#endif
#if defined board_stm32
	case 232:
		HL = stm32bdos(DE);
		break;
#endif
		/*
		C = 249 (F9h) : MakeDisk
		Makes a disk directory if not existent.
		*/
	case 249:
		HL = _MakeDisk(DE);
		break;
		/*
		C = 250 (FAh) : HostOS
		Returns: A = 0x00 - Windows / 0x01 - Arduino / 0x02 - Posix / 0x03 - Dos / 0x04 - Teensy / 0x05 - ESP32 / 0x06 - STM32
		*/
	case 250:
		HL = HostOS;
		break;
		/*
		C = 251 (FBh) : Version
		Returns: A = 0xVv - Version in BCD representation: V.v
		*/
	case 251:
		HL = VersionBCD;
		break;
		/*
		C = 252 (FCh) : CCP version
		Returns: A = 0x00-0x04 = DRI|CCPZ|ZCPR2|ZCPR3|Z80CCP / 0xVv = Internal version in BCD: V.v
		*/
	case 252:
		HL = VersionCCP;
		break;
		/*
		C = 253 (FDh) : CCP address
		*/
	case 253:
		HL = CCPaddr;
		break;
#ifdef HASLUA
		/*
		C = 254 (FEh) : Run Lua file
		*/
	case 254:
		HL = _RunLua(DE);
		break;
#endif
		/*
		Unimplemented calls get listed
		*/
	default:
#ifdef DEBUG	// Show unimplemented BDOS calls only when debugging
		_puts("\r\nUnimplemented BDOS call.\r\n");
		_puts("C = 0x");
		_puthex8(ch);
		_puts("\r\n");
#endif
		break;
	}

	// CP/M BDOS does this before returning
	SET_HIGH_REGISTER(BC, HIGH_REGISTER(HL));
	SET_HIGH_REGISTER(AF, LOW_REGISTER(HL));

#ifdef DEBUGLOG
#ifdef LOGONLY
	if (ch == LOGONLY)
#endif
		_logBdosOut(ch);
#endif

}

#endif
