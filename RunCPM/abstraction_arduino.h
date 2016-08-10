/* Memory abstraction functions */
/*===============================================================================*/

//#define BDOSjmppage	0xec
//#define BIOSjmppage	0xfa
//#define BDOSpage	0xfb
//#define BIOSpage	0xfc
#define BDOSjmppage	0xfc
#define BIOSjmppage	0xfd
#define BDOSpage	0xfe
#define BIOSpage	0xff

#define ROMSTART (BDOSpage<<8)
#define ROMSIZE 0x10000-ROMSTART
#define RAMSIZE ROMSTART

uint8 ROM[ROMSIZE];	// ROM must go into code
uint8 RAM[RAMSIZE];	// RAM must go into memory

uint8 _RamRead(uint16 address)
{
	if (address < ROMSTART) {
		return(RAM[address]);
	} else {
		return(ROM[address - ROMSTART]);
	}
}

void _RamWrite(uint16 address, uint8 value)
{
	if (address < ROMSTART) {
		RAM[address] = value;
	} else {
		ROM[address - ROMSTART] = value;
	}
}

/* Filesystem (disk) abstraction fuctions */
/*===============================================================================*/
uint8	pattern[12];
uint8	fcbname[12];
uint8	filename[15];
uint8	newname[13];
uint16	dmaAddr = 0x0080;
uint16	roVector = 0;
uint16	loginVector = 0;

uint8	user = 0;	// Current CP/M user

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

typedef struct {
	uint8 uu;
	uint8 fn[8];
	uint8 tp[3];
	uint8 xl, bc, xh, rc;
	uint8 al[16];
} CPM_DIR;

void _GetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	unsigned char i = 0, j = 0;

	while (i < 8) {
		if (F->fn[i] > 32) {
			*(filename + j++) = F->fn[i];
		}
		i++;
	}
	*(filename + j++) = '.';
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32) {
			*(filename + j++) = F->tp[i];
		}
		i++;
	}
	*(filename + j) = 0x00;
}

void _SetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	int i = 0;

	while (*filename != 0 && *filename != '.') {
		F->fn[i] = toupper(*filename);
		*filename++;
		i++;
	}
	while (i < 8) {
		F->fn[i] = ' ';
		i++;
	}
	if (*filename == '.') {
		*filename++;
	}
	i = 0;
	while (*filename != 0) {
		F->tp[i] = toupper(*filename);
		*filename++;
		i++;
	}
	while (i < 3) {
		F->tp[i] = ' ';
		i++;
	}
}

void _SetFCBFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];

	int i = 0;
	while (i < 8) {
		F->fn[i] = toupper(*filename);
		*filename++;
		i++;
	}
	i = 0;
	while (i < 3) {
		F->tp[i] = toupper(*filename);
		*filename++;
		i++;
	}
}

void dirToFCB(uint8* from, uint8* to)
{
	int i = 0;

	while (*from != 0 && *from != '.')
	{
		*to = toupper(*from);
		to++; from++; i++;
	}
	while (i < 8) {
		*to = ' ';
		to++;  i++;
	}
	if (*from == '.') {
		from++;
	}
	i = 0;
	while (*from != 0) {
		*to = toupper(*from);
		to++; from++; i++;
	}
	while (i < 3) {
		*to = ' ';
		to++;  i++;
	}
	*to = 0;
}

bool match(uint8* fcbname, uint8* pattern)
{
	bool result = 1;
	uint8 i;

	for (i = 0; i < 12; i++)
	{
		if (*pattern == '?' || *pattern == *fcbname) {
			pattern++; fcbname++;
			continue;
		} else {
			result = 0;
			break;
		}
	}
	return(result);
}

bool findFirst(uint8* pattern, uint8* fcbname)
{
	bool result = 0;
	uint8 dirname[13];
	bool isfile;

	SD.vwd()->rewind();
	while (dir.openNext(SD.vwd(), O_READ)) {
		dir.getName((char*)dirname, 13);
		isfile = dir.isFile();
		dir.close();
		if (!isfile)
			continue;
		dirToFCB(dirname, fcbname);
		if (match(fcbname, pattern)) {
			result = 1;
			break;
		}
	}
	return(result);
}

bool findNext(uint8* pattern, uint8* fcbname)
{
	bool result = 0;
	uint8 dirname[13];
	uint8 temp[12];
	uint8* oldname = &temp[0];
	bool isfile;

	SD.vwd()->rewind();
	while (dir.openNext(SD.vwd(), O_READ)) {
		dir.getName((char*)dirname, 13);
		isfile = dir.isFile();
		dir.close();
		if (!isfile)
			continue;
		dirToFCB(dirname, oldname);
		if (match(oldname, fcbname))
			break;
	}
	while (dir.openNext(SD.vwd(), O_READ)) {
		dir.getName((char*)dirname, 13);
		dir.close();
		//		if (!dir.isFile())
		//			continue;
		dirToFCB(dirname, fcbname);
		if (match(fcbname, pattern)) {
			result = 1;
			break;
		}
	}
	return(result);
}

uint8 _findfirst(void) {
	uint8 result = 0xff;

	dirToFCB(filename, pattern);
	if (findFirst(pattern, fcbname)) {
		_SetFCBFile(dmaAddr, fcbname);
		_RamWrite(dmaAddr, 0x00);
		result = 0x00;
	}

	return(result);
}

uint8 _findnext(void) {
	uint8 result = 0xff;

	dirToFCB(filename, pattern);
	if (findNext(pattern, fcbname)) {
		_SetFCBFile(dmaAddr, fcbname);
		_RamWrite(dmaAddr, 0x00);
		result = 0x00;
	}

	return(result);
}

/* Console abstraction functions */
/*===============================================================================*/

int _kbhit(void)
{
	return(Serial.available());
}

uint8 _getch(void)
{
	while (!Serial.available()) { ; }
	return(Serial.read());
}

uint8 _getche(void)
{
	uint8 ch = _getch();
	Serial.write(ch);
	return(ch);
}

void _putch(uint8 ch)
{
	Serial.write(ch);
}

void _clrscr(void)
{
	Serial.println("\b[H\b[J");
}

