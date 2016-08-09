/* see main.c for definition */

#define NOP		0x00
#define JP		0xc3
#define RET		0xc9
#define INa		0xdb	// Triggers a BIOS call
#define OUTa	0xd3	// Triggers a BDOS call

#define CCPname "CPM22.BIN"
uint16 CCPaddr = 0xE400;
uint8 CCPmode = 1;

void _PatchCPM(void)
{
	uint16 BDOSjmpaddr = (BDOSjmppage << 8) + 6;
	uint16 BIOSjmpaddr = BIOSjmppage << 8;
	uint16 BDOSaddr = BDOSpage << 8;
	uint16 BIOSaddr = BIOSpage << 8;

	//**********  Patch CP/M page zero into the memory  **********

	/* BIOS entry point */
	_RamWrite(0x0000, JP);		/* JP BIOS+3 (warm boot) */
	_RamWrite(0x0001, 0x03);
	_RamWrite(0x0002, BIOSjmppage);

	/* IOBYTE - Points to Console */
	_RamWrite(0x0003, 0x00);

	/* Current drive/user - A:/0 */
	if (Status!=2)
		_RamWrite(0x0004, 0x00);

	/* BDOS entry point (0x0005) */
	_RamWrite(0x0005, JP);
	_RamWrite(0x0006, 0x06);
	_RamWrite(0x0007, BDOSjmppage);

	//**********  Patch CP/M BDOS/BIOS jump tables into the memory

	_RamWrite(BDOSjmpaddr++, JP);
	_RamWrite(BDOSjmpaddr++, 0x00);
	_RamWrite(BDOSjmpaddr++, BDOSpage);

	/* BIOS jump table */
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x00);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x03);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x06);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x09);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x0c);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x0f);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x12);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x15);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x18);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x1b);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x1e);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x21);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x24);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x27);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x2a);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x2d);
	_RamWrite(BIOSjmpaddr++, BIOSpage);
	_RamWrite(BIOSjmpaddr++, JP);
	_RamWrite(BIOSjmpaddr++, 0x30);
	_RamWrite(BIOSjmpaddr++, BIOSpage);


	//**********  Patch CP/M BDOS/BIOS call tables into the memory  **********

	/* BDOS call table */
	_RamWrite(BDOSaddr++, INa);		/* IN A, N */
	_RamWrite(BDOSaddr++, 0x00);
	_RamWrite(BDOSaddr++, RET);		/* RET */

	/* BIOS call table */
	_RamWrite(BIOSaddr++, OUTa);		/* 0 - Cold boot */
	_RamWrite(BIOSaddr++, 0x00);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 3 - Warm boot */
	_RamWrite(BIOSaddr++, 0x01);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 6 - Console status  */
	_RamWrite(BIOSaddr++, 0x02);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 9 - Console input */
	_RamWrite(BIOSaddr++, 0x03);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* C - Console output */
	_RamWrite(BIOSaddr++, 0x04);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* F - List output */
	_RamWrite(BIOSaddr++, 0x05);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 12 - Punch output */
	_RamWrite(BIOSaddr++, 0x06);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 15 - Reader input */
	_RamWrite(BIOSaddr++, 0x07);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 18 - Home disk */
	_RamWrite(BIOSaddr++, 0x08);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 1B - Select disk */
	_RamWrite(BIOSaddr++, 0x09);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 1E - Select track */
	_RamWrite(BIOSaddr++, 0x0a);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 21 - Select sector */
	_RamWrite(BIOSaddr++, 0x0b);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 24 - Set DMA address */
	_RamWrite(BIOSaddr++, 0x0c);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 27 - Read selected sector */
	_RamWrite(BIOSaddr++, 0x0d);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 2A - Write selected sector */
	_RamWrite(BIOSaddr++, 0x0e);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 2D - List status */
	_RamWrite(BIOSaddr++, 0x0f);
	_RamWrite(BIOSaddr++, RET);
	_RamWrite(BIOSaddr++, OUTa);		/* 30 - Sector translation */
	_RamWrite(BIOSaddr++, 0x10);
	_RamWrite(BIOSaddr++, RET);

	//**********  Patch CP/M (fake) Disk Paramater Block after the BDOS call entry  **********

	_RamWrite(BDOSaddr++, 0x20);		/* spt - Sectors Per Track */
	_RamWrite(BDOSaddr++, 0x00);
	_RamWrite(BDOSaddr++, 0x04);		/* bsh - Data allocation "Block Shift Factor" */
	_RamWrite(BDOSaddr++, 0x0f);		/* blm - Data allocation Block Mask */
	_RamWrite(BDOSaddr++, 0x00);		/* exm - Extent Mask */
	_RamWrite(BDOSaddr++, 0xff);        /* dsm - Total storage capacity of the disk drive */
	_RamWrite(BDOSaddr++, 0x01);
	_RamWrite(BDOSaddr++, 0xfe);		/* drm - Number of the last directory entry */
	_RamWrite(BDOSaddr++, 0x00);
	_RamWrite(BDOSaddr++, 0xF0);		/* al0 */
	_RamWrite(BDOSaddr++, 0x00);		/* al1 */
	_RamWrite(BDOSaddr++, 0x3f);		/* cks - Check area Size */
	_RamWrite(BDOSaddr++, 0x00);
	_RamWrite(BDOSaddr++, 0x02);		/* off - Number of system reserved tracks at the beginning of the ( logical ) disk */
	_RamWrite(BDOSaddr++, 0x00);

}

#ifdef DEBUGLOG
void _logMemory(uint16 pos, uint8 size)
{
	FILE* file = _fopen_a((uint8 *)"RunCPM.log");
	uint16 h = pos;
	uint16 c = pos;
	uint8 l, i;
	uint8 ch;
	uint8 nl = (size / 16) + 1;

//	fprintf(file, "\r\n\t\t");
	for (l = 0; l < nl; l++)
	{
		fprintf(file, "\t\t%04x : ", h);
		for (i = 0; i < 16; i++)
		{
			fprintf(file, "%02x ", _RamRead(h++));
		}
		for (i = 0; i < 16; i++)
		{
			ch = _RamRead(c++);
			fprintf(file, "%c",ch>31 && ch<127 ? ch : '.');
		}
		fprintf(file, "\r\n");
	}
	_fclose(file);
}

char *_logFlags(uint8 ch)
{
	char f[] = "........";
	if (ch & 0b10000000)
		f[0] = "S";
	if (ch & 0b01000000)
		f[1] = "Z";
	if (ch & 0b00100000)
		f[2] = "5";
	if (ch & 0b00010000)
		f[3] = "H";
	if (ch & 0b00001000)
		f[4] = "3";
	if (ch & 0b00000100)
		f[5] = "P";
	if (ch & 0b00000010)
		f[6] = "N";
	if (ch & 0b00000001)
		f[7] = "C";
	f[8] = 0;
	return(f);
}

void _logBiosIn(uint8 ch)
{
	FILE* file = _fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "Bios call: %d (0x%02x) - ", ch, ch);
	switch (ch) {
	case 0:
		fputs("Cold Boot\r\n", file);
		break;
	case 3:
		fputs("Warm Boot\r\n", file);
		break;
	case 6:
		fputs("Console Status\r\n", file);
		break;
	case 9:
		fputs("Console Input\r\n", file);
		break;
	case 12:
		fputs("Console Output\r\n", file);
		break;
	case 15:
		fputs("List Output\r\n", file);
		break;
	case 18:
		fputs("Punch Output\r\n", file);
		break;
	case 21:
		fputs("Reader Input\r\n", file);
		break;
	case 24:
		fputs("Home Disk\r\n", file);
		break;
	case 27:
		fputs("Select Disk\r\n", file);
		break;
	case 30:
		fputs("Select Track\r\n", file);
		break;
	case 33:
		fputs("Select Sector\r\n", file);
		break;
	case 36:
		fputs("Set DMA Address\r\n", file);
		break;
	case 39:
		fputs("Read Selected Sector\r\n", file);
		break;
	case 42:
		fputs("Write Selected Sector\r\n", file);
		break;
	case 45:
		fputs("List Status\r\n", file);
		break;
	case 48:
		fputs("Sector Translation\r\n", file);
		break;
	default:
		fputs("Unknown\r\n", file);
		break;
	}
	fprintf(file, "\tIn : BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_fclose(file);
}

void _logBiosOut(uint8 ch)
{
	FILE* file = _fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "\tOut: BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_fclose(file);
}

void _logBdosIn(uint8 ch)
{
	FILE* file = _fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "Bdos call: %d (0x%02x) - ", ch, ch);
	switch (ch){
	case 0:
		fputs("System Reset\r\n", file);
		break;
	case 1:
		fputs("Console Input\r\n", file);
		break;
	case 2:
		fputs("Console Output\r\n", file);
		break;
	case 3:
		fputs("Reader Input\r\n", file);
		break;
	case 4:
		fputs("Punch Output\r\n", file);
		break;
	case 5:
		fputs("List Output\r\n", file);
		break;
	case 6:
		fputs("Direct Console I/O\r\n", file);
		break;
	case 7:
		fputs("Get I/O Byte\r\n", file);
		break;
	case 8:
		fputs("Set I/O Byte\r\n", file);
		break;
	case 9:
		fputs("Print String\r\n", file);
		break;
	case 10:
		fputs("Read Console Buffer\r\n", file);
		break;
	case 11:
		fputs("Get Console Status\r\n", file);
		break;
	case 12:
		fputs("Return Version Number\r\n", file);
		break;
	case 13:
		fputs("Reset Disk System\r\n", file);
		break;
	case 14:
		fputs("Select Disk\r\n", file);
		break;
	case 15:
		fputs("Open File\r\n", file);
		break;
	case 16:
		fputs("Close File\r\n", file);
		break;
	case 17:
		fputs("Search for First\r\n", file);
		break;
	case 18:
		fputs("Search for Next\r\n", file);
		break;
	case 19:
		fputs("Delete File\r\n", file);
		break;
	case 20:
		fputs("Read Sequential\r\n", file);
		break;
	case 21:
		fputs("Write Sequential\r\n", file);
		break;
	case 22:
		fputs("Make File\r\n", file);
		break;
	case 23:
		fputs("Rename File\r\n", file);
		break;
	case 24:
		fputs("Return Log-in Vector\r\n", file);
		break;
	case 25:
		fputs("Return Current Disk\r\n", file);
		break;
	case 26:
		fputs("Set DMA Address\r\n", file);
		break;
	case 27:
		fputs("Get Addr(Alloc)\r\n", file);
		break;
	case 28:
		fputs("Write Protect Disk\r\n", file);
		break;
	case 29:
		fputs("Get Read/Only Vector\r\n", file);
		break;
	case 30:
		fputs("Set File Attributes\r\n", file);
		break;
	case 31:
		fputs("Get ADDR(Disk Parms)\r\n", file);
		break;
	case 32:
		fputs("Set/Get User Code\r\n", file);
		break;
	case 33:
		fputs("Read Random\r\n", file);
		break;
	case 34:
		fputs("Write Random\r\n", file);
		break;
	case 35:
		fputs("Compute File Size\r\n", file);
		break;
	case 36:
		fputs("Set Random Record\r\n", file);
		break;
	case 37:
		fputs("Reset Drive\r\n", file);
		break;
	case 38:
		fputs("Access Drive (not supported)\r\n", file);
		break;
	case 39:
		fputs("Free Drive (not supported)\r\n", file);
		break;
	case 40:
		fputs("Write Random with Zero Fill\r\n", file);
		break;
#ifdef ARDUINO
	case 220:
		fputs("PinMode\r\n", file);
		break;
	case 221:
		fputs("DigitalRead\r\n", file);
		break;
	case 222:
		fputs("DigitalWrite\r\n", file);
		break;
	case 223:
		fputs("AnalogRead\r\n", file);
		break;
	case 224:
		fputs("AnalogWrite\r\n", file);
		break;
#endif
	default:
		fputs("Unknown\r\n", file);
		break;
	}
	fprintf(file, "\tIn : BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_fclose(file);

	switch (ch){
	case 9:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 30:
	case 33:
	case 34:
	case 35:
	case 36:
	case 40:
		_logMemory(DE, 36);
	default:
		break;
	}
}

void _logBdosOut(uint8 ch)
{
	FILE* file = _fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "\tOut: BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_fclose(file);

	switch (ch){
	case 10:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 30:
	case 33:
	case 34:
	case 35:
	case 36:
	case 40:
#ifdef ARDUINO
	case 220:
	case 221:
	case 222:
	case 223:
	case 224:
#endif
		_logMemory(DE, 36);
	default:
		break;
	}
}
#endif // DEBUGLOG

void _Bios(void)
{
	uint8 ch = LOW_REGISTER(PCX);

#ifdef DEBUGLOG
	_logBiosIn(LOW_REGISTER(PCX));
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
	default:				// Unimplemented calls get listed
		_puts("\r\nUnimplemented BIOS call.\r\n");
		_puts("C = 0x");
		_puthex8(ch);
		_puts("\r\n");
	}
#ifdef DEBUGLOG
	_logBiosOut(LOW_REGISTER(PCX));
#endif

}

void _Bdos(void)
{
	CPM_FCB* F;
	int32	i, c, count;
	uint8	ch = LOW_REGISTER(BC);

#ifdef DEBUGLOG
	_logBdosIn(LOW_REGISTER(BC));
#endif

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
		Returns: A=L=Char
		*/
	case 1:
		SET_HIGH_REGISTER(AF, _getche());
#ifdef DEBUG
		if (HIGH_REGISTER(AF) == 4)
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
		Returns: A=L=Char
		*/
	case 3:
		SET_HIGH_REGISTER(AF, 0x1a);
		break;
		/*
		C = 4 : Auxiliary (Punch) output
		*/
	case 4:
		break;
		/*
		C = 5 : Printer output
		*/
	case 5:
		break;
		/*
		C = 6 : Direct console IO
		E = 0xFF : Checks for char available and returns it, or 0x00 if none
		E = char : Outputs char
		Gets a char from the console
		Returns: A=Char or 0x00
		*/
	case 6:
		if (LOW_REGISTER(DE) == 0xff) {
			SET_HIGH_REGISTER(AF, _getchNB());
#ifdef DEBUG
			if (HIGH_REGISTER(AF) == 4)
				Debug = 1;
#endif
		} else if (LOW_REGISTER(DE) == 0xfe) {	// Undocumented behavior (not clear on DRI User Guide)
			SET_HIGH_REGISTER(AF, _chready());
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
		SET_HIGH_REGISTER(AF, _RamRead(0x0003));
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
		i = DE;
		c = _RamRead(i);	// Gets the number of characters to read
		i++;	// Points to the number read
		count = 0;
		while (TRUE)	// Very simplistic line input
		{
			ch = _getch();
#ifdef DEBUG
			if (ch == 4)
				Debug = 1;
#endif
			if (ch == 3 && count == 0) {
				Status = 2;
				break;
			}
			if (ch == 0x0D || ch == 0x0A)
				break;
			if ((ch == 0x08 || ch == 0x7F) && count > 0) {
				_putcon('\b');
				_putcon(' ');
				_putcon('\b');
				count--;
				continue;
			}
			if (ch < 0x20 || ch > 0x7E)
				continue;
			_putcon(ch);
			count++; _RamWrite(i + count, ch);
			if (count == c)
				break;
		}
		_RamWrite(i, count);	// Saves the number read
		break;
		/*
		C = 11 (0Bh) : Get console status
		Returns: A=0x00 or 0xFF
		*/
	case 11:
		SET_HIGH_REGISTER(AF, _chready());
		break;
		/*
		C = 12 (0Ch) : Get version number
		Returns: B=H=system type, A=L=version number
		*/
	case 12:
		// The undocumented behavior below may be used by applications to verify that they are running on CP/M 2.2
		HL = 0x22;
		SET_HIGH_REGISTER(AF, LOW_REGISTER(HL));	// Undocumented behavior (doesn't follow DRI User Guide)
		SET_HIGH_REGISTER(BC, HIGH_REGISTER(HL));	// Undocumented behavior (doesn't follow DRI User Guide)
		break;
		/*
		C = 13 (0Dh) : Reset disk system
		*/
	case 13:
		roVector = 0;	// Make all drives R/W
		loginVector = 0;
		dmaAddr = 0x0080;
		_RamWrite(0x0004, 0x00);	// Reset default drive to A: and CP/M user to 0 (0x00)
		break;
		/*
		C = 14 (0Eh) : Select Disk
		Returns: A=0x00 or 0xFF
		*/
	case 14:
		if (_SelectDisk(LOW_REGISTER(DE)+1)) {
			_RamWrite(0x0004, (_RamRead(0x0004) & 0xf0) | (LOW_REGISTER(DE) & 0x0f));
		} else {
			_error(errSELECT);
			_RamWrite(0x0004, _RamRead(0x0004) & 0xf0);	// Set current drive to A:, keep user
		}
		break;
		/*
		C = 15 (0Fh) : Open file
		Returns: A=0x00 or 0xFF
		*/
	case 15:
		SET_HIGH_REGISTER(AF, _OpenFile(DE));
		break;
		/*
		C = 16 (10h) : Close file
		*/
	case 16:
		SET_HIGH_REGISTER(AF, _CloseFile(DE));
		break;
		/*
		C = 17 (11h) : Search for first
		*/
	case 17:
		SET_HIGH_REGISTER(AF, _SearchFirst(DE));
		break;
		/*
		C = 18 (12h) : Search for next
		*/
	case 18:
		SET_HIGH_REGISTER(AF, _SearchNext(dmaAddr));
		break;
		/*
		C = 19 (13h) : Delete file
		*/
	case 19:
		SET_HIGH_REGISTER(AF, _DeleteFile(DE));
		break;
		/*
		C = 20 (14h) : Read sequential
		*/
	case 20:
		SET_HIGH_REGISTER(AF, _ReadSeq(DE));
		break;
		/*
		C = 21 (15h) : Write sequential
		*/
	case 21:
		SET_HIGH_REGISTER(AF, _WriteSeq(DE));
		break;
		/*
		C = 22 (16h) : Make file
		*/
	case 22:
		SET_HIGH_REGISTER(AF, _MakeFile(DE));
		break;
		/*
		C = 23 (17h) : Rename file
		*/
	case 23:
		SET_HIGH_REGISTER(AF, _RenameFile(DE));
		break;
		/*
		C = 24 (18h) : Return log-in vector
		*/
	case 24:
		HL = loginVector;	// (todo) improve this
		SET_HIGH_REGISTER(AF, LOW_REGISTER(HL));
		SET_HIGH_REGISTER(BC, HIGH_REGISTER(HL));
		break;
		/*
		C = 25 (19h) : Return current disk
		*/
	case 25:
		SET_HIGH_REGISTER(AF, _RamRead(0x0004) & 0x0f);
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
		HL = (BDOSpage << 8) + 18;
		SET_HIGH_REGISTER(AF, LOW_REGISTER(HL));
		SET_HIGH_REGISTER(BC, HIGH_REGISTER(HL));
		break;
		/*
		C = 28 (1Ch) : Write protect current disk
		*/
	case 28:
		roVector = roVector | (1 << (_RamRead(0x0004) & 0x0f));
		break;
		/*
		C = 29 (1Dh) : Get R/O vector
		*/
	case 29:
		HL = roVector;
		SET_HIGH_REGISTER(AF, LOW_REGISTER(HL));
		SET_HIGH_REGISTER(BC, HIGH_REGISTER(HL));
		break;
		/********** (todo) Function 30: Set file attributes **********/
		/*
		C = 31 (1Fh) : Get ADDR(Disk Parms)
		*/
	case 31:
		HL = (BDOSpage << 8) + 3;
		break;
		/*
		C = 32 (20h) : Get/Set user code
		*/
	case 32:
		if (LOW_REGISTER(DE) == 0xFF) {
			SET_HIGH_REGISTER(AF, user);
		} else {
			user = LOW_REGISTER(DE);
			_RamWrite(0x0004, (_RamRead(0x0004) & 0x0f) | (LOW_REGISTER(DE) << 4));
		}
		break;
		/*
		C = 33 (21h) : Read random
		*/
	case 33:
		SET_HIGH_REGISTER(AF, _ReadRand(DE));
		break;
		/*
		C = 34 (22h) : Write random
		*/
	case 34:
		SET_HIGH_REGISTER(AF, _WriteRand(DE));
		break;
		/*
		C = 35 (23h) : Compute file size
		*/
	case 35:
		F = (CPM_FCB*)&RAM[DE];
		count = _FileSize(DE) >> 7;

		F->r0 = count & 0xff;
		F->r1 = (count >> 8) & 0xff;
		F->r2 = (count >> 16) & 0xff;
		break;
		/*
		C = 36 (24h) : Set random record
		*/
	case 36:
		F = (CPM_FCB*)&RAM[DE];
		count = F->cr & 0x7f;
		count += (F->ex & 0x1f) << 7;
		count += F->s1 << 12;
		count += F->s2 << 20;

		F->r0 = count & 0xff;
		F->r1 = (count >> 8) & 0xff;
		F->r2 = (count >> 16) & 0xff;
		break;
		/*
		C = 37 (25h) : Reset drive
		*/
	case 37:
		SET_HIGH_REGISTER(AF, 00);
		break;
		/********** Function 38: Not supported by CP/M 2.2 **********/
		/********** Function 39: Not supported by CP/M 2.2 **********/
		/********** (todo) Function 40: Write random with zero fill **********/
		/*
		C = 40 (28h) : Write random with zero fill (we have no disk blocks, so just write random)
		*/
	case 40:
		SET_HIGH_REGISTER(AF, _WriteRand(DE));
		break;
#ifdef ARDUINO
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
		SET_HIGH_REGISTER(AF, digitalRead(HIGH_REGISTER(DE)));
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
		/*
		C = 224 (E0h) : AnalogWrite
		*/
	case 224:
		analogWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
#endif
		/*
		Unimplemented calls get listed
		*/
	default:
		_puts("\r\nUnimplemented BDOS call.\r\n");
		_puts("C = 0x");
		_puthex8(ch);
		_puts("\r\n");
	}
#ifdef DEBUGLOG
	_logBdosOut(LOW_REGISTER(BC));
#endif

}
