/* Memory abstraction functions */
/*===============================================================================*/
bool _RamLoad(char* filename, uint16 address)
{
	SdFile f;
	bool result = false;

	if (f.open(filename, FILE_READ))
	{
		while (f.available())
			_RamWrite(address++, f.read());
		f.close();
		result = true;
	}
	return(result);
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
int		dirPos;

uint8	user = 0;	// Current CP/M user

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

int _sys_select(uint8 *disk)
{
	sd.chdir();
	return(sd.chdir((char*)disk)); // (todo) Test if it is Directory
}

long _sys_filesize(uint8 *filename)
{
	long l = -1;
	SdFile f;
	if (f.open((char*)filename, O_RDONLY)) {
		l = f.fileSize();
		f.close();
	}
	return(l);
}

int _sys_openfile(uint8 *filename)
{
	SdFile f;
	uint8 file = f.open((char*)filename, O_READ);
	if (file != NULL)
		f.close();
	return(file != NULL);
}

int _sys_makefile(uint8 *filename)
{
	SdFile f;
	uint8 file = f.open((char*)filename, O_CREAT | O_WRITE);
	if (file != NULL)
		f.close();
	return(file != NULL);
}

int _sys_deletefile(uint8 *filename)
{
	int result = sd.remove((char*)filename);
	if (result)
		dirPos--;
	return(result);
}

int _sys_renamefile(uint8 *filename, uint8 *newname)
{
	return(sd.rename((char*)filename, (char*)newname));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer)
{
	_puts((char *)buffer);
//	SdFile sd;
//	uint8 s = 0;
//	while (*(buffer+s))	// Computes buffer size
//		s++;
//	int32 file = sd.open(LogName, O_WRITE | O_APPEND);
//	sd.write(buffer, s);
//	sd.close();
}
#endif

uint8 _sys_readseq(uint8 *filename, long fpos)
{
	uint8 result = 0xff;
	SdFile f;
	uint8 bytesread;

	if (f.open((char*)filename, O_READ)) {
		if (f.seekSet(fpos)) {
			_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z (EOF) prior to reading
			bytesread = f.read(_RamSysAddr(dmaAddr), 128);
			if (bytesread) {
				result = 0x00;
			} else {
					result = 0x01;
			}
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writeseq(uint8 *filename, long fpos)
{
	uint8 result = 0xff;
	SdFile f;

	if (f.open((char*)filename, O_RDWR)) {
		if (f.seekSet(fpos)) {
			if (f.write(_RamSysAddr(dmaAddr), 128)) {
				result = 0x00;
			}
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_readrand(uint8 *filename, long fpos)
{
	uint8 result = 0xff;
	SdFile f;
	uint8 bytesread;

	if (f.open((char*)filename, O_READ)) {
		if (f.seekSet(fpos)) {
			_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z prior to reading
			bytesread = f.read(_RamSysAddr(dmaAddr), 128);
			if (bytesread) {
				result = 0x00;
			} else {
				result = 0x01;
			}
		} else {
			result = 0x06;
		}
		f.close();
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writerand(uint8 *filename, long fpos)
{
	uint8 result = 0xff;
	SdFile f;

	if (f.open((char*)filename, O_RDWR)) {
		if (f.seekSet(fpos)) {
			if (f.write(_RamSysAddr(dmaAddr), 128)) {
				result = 0x00;
			}
		} else {
			result = 0x06;
		}
		f.close();
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _GetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;
	uint8 unique = TRUE;

	while (i < 8) {
		if (F->fn[i] > 32) {
			*(filename++) = F->fn[i];
		}
		if (F->fn[i] == '?')
			unique = FALSE;
		i++;
	}
	*(filename++) = '.';
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32) {
			*(filename++) = F->tp[i];
		}
		if (F->tp[i] == '?')
			unique = FALSE;
		i++;
	}
	*filename = 0x00;

	return(unique);
}

void _SetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;

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

bool findNext(uint8* pattern, uint8* fcbname)
{
	bool result = 0;
	SdFile dir;
	uint8 dirname[13];
	bool isfile;
	int i;

	sd.vwd()->rewind();
	for (i = 0; i < dirPos; i++) {
		dir.openNext(sd.vwd(), O_READ);
		dir.close();
	}
	while (dir.openNext(sd.vwd(), O_READ)) {
		dir.getName((char*)dirname, 13);
		isfile = dir.isFile();
		dir.close();
		dirPos++;
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

uint8 _findnext(uint8 dir) {
	uint8 result = 0xff;

	if (findNext(pattern, fcbname)) {
		if(dir) {
			_SetFile(dmaAddr, fcbname);
			_RamWrite(dmaAddr, 0x00);
		}
		_SetFile(tmpFCB, fcbname);
		result = 0x00;
	}

	return(result);
}

uint8 _findfirst(uint8 dir) {
	uint8 result = 0xff;
	dirPos = 0;
	dirToFCB(filename, pattern);

	return(_findnext(dir));
}

uint8 _Truncate(uint8 *fn, uint8 rc)
{
	SdFile file;
	uint8 result = 0xff;

	if (file.open((char *)fn, O_RDWR)) {
		if (file.truncate(rc * 128)) {
			result = 0x00;
		}
		file.close();
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

