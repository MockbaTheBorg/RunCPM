/* Memory abstraction functions */
/*===============================================================================*/
bool _RamLoad(char *filename, uint16 address) {
	SdFile f;
	bool result = false;

	if (f.open(filename, FILE_READ)) {
		while (f.available())
			_RamWrite(address++, f.read());
		f.close();
		result = true;
	}
	return(result);
}

/* Filesystem (disk) abstraction fuctions */
/*===============================================================================*/
int		dirPos;

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

int _sys_select(uint8 *disk) {
	uint8 result = FALSE;
	SdFile f;

	if (f.open((char *)disk, O_READ)) {
		if (f.isDir())
			result = TRUE;
		f.close();
	}
	return(result);
}

long _sys_filesize(uint8 *filename) {
	long l = -1;
	SdFile f;
	if (f.open((char*)filename, O_RDONLY)) {
		l = f.fileSize();
		f.close();
	}
	return(l);
}

int _sys_openfile(uint8 *filename) {
	SdFile f;
	uint8 file = f.open((char*)filename, O_READ);
	if (file > 0)
		f.close();
	return(file > 0);
}

int _sys_makefile(uint8 *filename) {
	SdFile f;
	uint8 file = f.open((char*)filename, O_CREAT | O_WRITE);
	if (file > 0)
		f.close();
	return(file > 0);
}

int _sys_deletefile(uint8 *filename) {
	return(sd.remove((char *)filename));
}

int _sys_renamefile(uint8 *filename, uint8 *newname) {
	return(sd.rename((char*)filename, (char*)newname));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer) {
#ifdef CONSOLELOG
	_puts((char *)buffer);
#else
	SdFile f;
	uint8 s = 0;
	while (*(buffer+s))	// Computes buffer size
		s++;
	if(f.open(LogName, O_CREAT | O_APPEND | O_WRITE)) {
		f.write(buffer, s);
		f.flush();
		f.close();
	}
}
#endif
#endif

bool _sys_extendfile(char *fn, long fpos)
{
	uint8 result = true;
	SdFile f;
	long i;

	if (f.open(fn, O_WRITE | O_APPEND)) {
		if (fpos > f.fileSize()) {
			for (i = 0; i < f.fileSize() - fpos; i++) {
				if (f.write((uint8_t)0) < 0) {
					result = false;
					break;
				}
			}
		}
		f.close();
	} else {
		result = false;
	}

	return(result);
}

uint8 _sys_readseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	SdFile f;
	uint8 bytesread;
	uint8 file = 0;

	if (_sys_extendfile((char*)filename, fpos)) {
		file = f.open((char*)filename, O_READ);
	}
	if (file > 0) {
		if (f.seekSet(fpos)) {
			_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z (EOF) prior to reading
			bytesread = f.read(_RamSysAddr(dmaAddr), 128);
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writeseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	SdFile f;
	uint8 file = 0;

	if (_sys_extendfile((char*)filename, fpos)) {
		file = f.open((char*)filename, O_RDWR);
	}
	if (file > 0) {
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

uint8 _sys_readrand(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	SdFile f;
	uint8 bytesread;
	uint8 file = 0;

	if (_sys_extendfile((char*)filename, fpos)) {
		file = f.open((char*)filename, O_READ);
	}
	if (file > 0) {
		if (f.seekSet(fpos)) {
			_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z prior to reading
			bytesread = f.read(_RamSysAddr(dmaAddr), 128);
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x06;
		}
		f.close();
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writerand(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	SdFile f;
	uint8 file = 0;

	if (_sys_extendfile((char*)filename, fpos)) {
		file = f.open((char*)filename, O_RDWR);
	}
	if (file > 0) {
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

uint8 _FCBtoHostname(uint16 fcbaddr, uint8 *filename) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;
	uint8 unique = TRUE;

	if (F->dr) {
		*(filename++) = (F->dr - 1) + 'A';
	} else {
		*(filename++) = (_RamRead(0x0004) & 0x0f) + 'A';
	}
	*(filename++) = '/';

	while (i < 8) {
		if (F->fn[i] > 32)
			*(filename++) = F->fn[i];
		if (F->fn[i] == '?')
			unique = FALSE;
		i++;
	}
	*(filename++) = '.';
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32)
			*(filename++) = F->tp[i];
		if (F->tp[i] == '?')
			unique = FALSE;
		i++;
	}
	*filename = 0x00;

	return(unique);
}

void _HostnameToFCB(uint16 fcbaddr, uint8 *filename) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 *dest = &F->fn[0];

	while (*filename)
		*dest++ = toupper(*filename++);
}

void _HostnameToFCBname(uint8 *from, uint8 *to) // Converts a string name (AB.TXT) to FCB name (AB      TXT)
{
	int i = 0;

	from++;
	if (*from == '/') {	// Skips the drive and / if needed
		from++;
	} else {
		from--;
	}

	while (*from != 0 && *from != '.') {
		*to = toupper(*from);
		to++; from++; i++;
	}
	while (i < 8) {
		*to = ' ';
		to++;  i++;
	}
	if (*from == '.')
		from++;
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

bool match(uint8 *fcbname, uint8 *pattern) {
	bool result = 1;
	uint8 i;

	for (i = 0; i < 12; i++) {
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

bool findNext(uint8 *pattern) {
	SdFile f;
	uint8 path[2] = "?";
	uint8 dirname[13];
	bool isfile, result = FALSE;
	int i;

	path[0] = filename[0];

	sd.chdir((char *)path, true);	// This switches sd momentarily to the folder
									// (todo) Get rid of these chdir() someday
	if (dirPos)
		sd.vwd()->seekSet(dirPos);

	while (f.openNext(sd.vwd(), O_READ)) {
		dirPos = sd.vwd()->curPosition();
		f.getName((char*)dirname, 13);
		isfile = f.isFile();
		f.close();
		if (!isfile)
			continue;
		_HostnameToFCBname(dirname, fcbname);
		if (match(fcbname, pattern)) {
			result = TRUE;
			break;
		}
	}
	sd.chdir("/", true);			// (todo) Get rid of these chdir() someday

	return(result);
}

uint8 _findnext(uint8 dir) {
	uint8 result = 0xff;

	if (findNext(pattern)) {
		if (dir) {
			_HostnameToFCB(dmaAddr, fcbname);
			_RamWrite(dmaAddr, 0x00);
		}
		_HostnameToFCB(tmpFCB, fcbname);
		result = 0x00;
	}

	return(result);
}

uint8 _findfirst(uint8 dir) {
	dirPos = 0;	// Set directory search to start from the first position
	_HostnameToFCBname(filename, pattern);
	return(_findnext(dir));
}

uint8 _Truncate(char *filename, uint8 rc) {
	uint8 result = 0xff;
	SdFile f;

	if (f.open((char*)filename, O_RDWR)) {
		if (f.truncate(rc * 128)) {
			result = 0x00;
		}
		f.close();
	}

	return(result);
}

/* Console abstraction functions */
/*===============================================================================*/

int _kbhit(void) {
	return(Serial.available());
}

uint8 _getch(void) {
	while (!Serial.available());
	return(Serial.read());
}

uint8 _getche(void) {
	uint8 ch = _getch();
	Serial.write(ch);
	return(ch);
}

void _putch(uint8 ch) {
	Serial.write(ch);
}

void _clrscr(void) {
	Serial.println("\b[H\b[J");
}

