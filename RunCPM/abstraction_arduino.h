#ifndef ABSTRACT_H
#define ABSTRACT_H

#define HostOS 0x01

/* Memory abstraction functions */
/*===============================================================================*/
bool _RamLoad(char *filename, uint16 address) {
	File f;
	bool result = false;

	if (f = SD.open(filename, FILE_READ)) {
		while (f.available())
			_RamWrite(address++, f.read());
		f.close();
		result = true;
	}
	return(result);
}

/* Filesystem (disk) abstraction fuctions */
/*===============================================================================*/
File root;
#define FOLDERCHAR '/'

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
	File f;

	digitalWrite(LED, HIGH);
	if (f = SD.open((char *)disk, O_READ)) {
		if (f.isDirectory())
			result = TRUE;
		f.close();
	}
	digitalWrite(LED, LOW);
	return(result);
}

long _sys_filesize(uint8 *filename) {
	long l = -1;
	File f;

	digitalWrite(LED, HIGH);
	if (f = SD.open((char *)filename, O_RDONLY)) {
		l = f.size();
		f.close();
	}
	digitalWrite(LED, LOW);
	return(l);
}

int _sys_openfile(uint8 *filename) {
	File f;
	int result = 0;

	digitalWrite(LED, HIGH);
	f = SD.open((char *)filename, O_READ);
	if (f) {
		f.close();
		result = 1;
	}
	digitalWrite(LED, LOW);
	return(result);
}

int _sys_makefile(uint8 *filename) {
	File f;
	int result = 0;

	digitalWrite(LED, HIGH);
	f = SD.open((char *)filename, O_CREAT | O_WRITE);
	if (f) {
		f.close();
		result = 1;
	}
	digitalWrite(LED, LOW);
	return(result);
}

int _sys_deletefile(uint8 *filename) {
	digitalWrite(LED, HIGH);
	return(SD.remove((char *)filename));
	digitalWrite(LED, LOW);
}

int _sys_movefile(char *filename, char *newname, int size) {
	File fold, fnew;
	int i, result = false;
	uint8 c;

	digitalWrite(LED, HIGH);
	if (fold = SD.open(filename, O_READ)) {
		if (fnew = SD.open(newname, O_CREAT | O_WRITE)) {
			result = true;
			for (i = 0; i < size; i++) {
				c = fold.read();
				if (fnew.write(c) < 1) {
					result = false;
					break;
				}
			}
			fnew.close();
		}
		fold.close();
	}
	if (result)
		SD.remove(filename);
	else
		SD.remove(newname);
	digitalWrite(LED, LOW);
	return(result);
}

int _sys_renamefile(uint8 *filename, uint8 *newname) {
	return _sys_movefile((char *)filename, (char *)newname, _sys_filesize(filename));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer) {
#ifdef CONSOLELOG
	puts((char *)buffer);
#else
	File f;
	uint8 s = 0;
	while (*(buffer+s))	// Computes buffer size
		s++;
	if(f = SD.open(LogName, O_CREAT | O_APPEND | O_WRITE)) {
		f.write(buffer, s);
		f.flush();
		f.close();
	}
#endif
}
#endif

bool _sys_extendfile(char *fn, unsigned long fpos)
{
	uint8 result = true;
	File f;
	unsigned long i;

	digitalWrite(LED, HIGH);
	if (f = SD.open(fn, O_WRITE | O_APPEND)) {
		if (fpos > f.size()) {
			for (i = 0; i < f.size() - fpos; i++) {
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
	digitalWrite(LED, LOW);
	return(result);
}

uint8 _sys_readseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	File f;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;

	digitalWrite(LED, HIGH);
	if (_sys_extendfile((char*)filename, fpos))
		f = SD.open((char*)filename, O_READ);
	if (f) {
		if (f.seek(fpos)) {
			for (i = 0; i < 128; i++)
				dmabuf[i] = 0x1a;
			bytesread = f.read(&dmabuf[0], 128);
			if (bytesread) {
				for (i = 0; i < 128; i++)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW);
	return(result);
}

uint8 _sys_writeseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	File f;

	digitalWrite(LED, HIGH);
	if (_sys_extendfile((char*)filename, fpos))
		f = SD.open((char*)filename, O_RDWR);
	if (f) {
		if (f.seek(fpos)) {
			if (f.write(_RamSysAddr(dmaAddr), 128))
				result = 0x00;
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW);
	return(result);
}

uint8 _sys_readrand(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	File f;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;

	digitalWrite(LED, HIGH);
	if (_sys_extendfile((char*)filename, fpos))
		f = SD.open((char*)filename, O_READ);
	if (f) {
		if (f.seek(fpos)) {
			for (i = 0; i < 128; i++)
				dmabuf[i] = 0x1a;
			bytesread = f.read(&dmabuf[0], 128);
			if (bytesread) {
				for (i = 0; i < 128; i++)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x06;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW);
	return(result);
}

uint8 _sys_writerand(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	File f;

	digitalWrite(LED, HIGH);
	if (_sys_extendfile((char*)filename, fpos)) {
		f = SD.open((char*)filename, O_RDWR);
	}
	if (f) {
		if (f.seek(fpos)) {
			if (f.write(_RamSysAddr(dmaAddr), 128))
				result = 0x00;
		} else {
			result = 0x06;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW);
	return(result);
}

uint8 _findnext(uint8 isdir) {
	File f;
	uint8 result = 0xff;
	uint8 dirname[13];
	char* fname;
	bool isfile;
	unsigned int i;

	digitalWrite(LED, HIGH);
	while (f = root.openNextFile()) {
	fname = f.name();
	for (i = 0; i < strlen(fname) + 1 && i < 13; i++)
		dirname[i] = fname[i];
		isfile = !f.isDirectory();
		f.close();
		if (!isfile)
			continue;
		_HostnameToFCBname(dirname, fcbname);
		if (match(fcbname, pattern)) {
			if (isdir) {
				_HostnameToFCB(dmaAddr, dirname);
				_RamWrite(dmaAddr, 0x00);
			}
			_RamWrite(tmpFCB, filename[0] - '@');
			_HostnameToFCB(tmpFCB, dirname);
			result = 0x00;
			break;
		}
	}
	digitalWrite(LED, LOW);
	return(result);
}

uint8 _findfirst(uint8 isdir) {
#ifdef USER_SUPPORT
	uint8 path[4] = { '?', FOLDERCHAR, '?', 0 };
#else
	uint8 path[2] = { '?', 0 };
#endif
	path[0] = filename[0];
#ifdef USER_SUPPORT
	path[2] = filename[2];
#endif
	if (root)
		root.close();
	root = SD.open((char *)path); // Set directory search to start from the first position
	_HostnameToFCBname(filename, pattern);
	return(_findnext(isdir));
}

uint8 _Truncate(char *filename, uint8 rc) {
	uint8 result = 0xff;
	File f;

	if (_sys_movefile(filename, (char *)"$$$$$$$$.$$$", _sys_filesize((uint8 *)filename))) {
		if (_sys_movefile((char *)"$$$$$$$$.$$$", filename, rc * 128)) {
			result = 0x00;
		}
	}
	return(result);
}

#ifdef USER_SUPPORT
void _MakeUserDir() {
	uint8 dFolder = cDrive + 'A';
	uint8 uFolder = toupper(tohex(userCode));

	uint8 path[4] = { dFolder, FOLDERCHAR, uFolder, 0 };

	digitalWrite(LED, HIGH);
	SD.mkdir((char*)path);
	digitalWrite(LED, LOW);
}
#endif

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
	Serial.println("\e[H\e[J");
}

#endif
