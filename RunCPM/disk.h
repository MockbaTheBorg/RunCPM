/* see main.c for definition */

/*
Disk errors
*/
#define errWRITEPROT 1
#define errSELECT 2

#define RW	(roVector&(1<<_RamRead(0x0004)))

static void _error(uint8 error)
{
	_puts("\nBdos Error on ");
	_putcon('A' + _RamRead(0x0004));
	_puts(": ");
	switch (error) {
	case errWRITEPROT:
		_puts("R/O");
		break;
	case errSELECT:
		_puts("SELECT");
		break;
	default:
		_puts("\nCP/M ERR");
		break;
	}
	_getch();
	_puts("\n");
	Status = 2;
}

/*
Creates a fake directory entry as we don't have a real disk drive
*/
void _FCBtoDIR(uint16 fcbaddr)
{
	CPM_DIR* d = (CPM_DIR*)&RAM[dmaAddr];

	_RamFill(dmaAddr, 128, 0xE5);
	_RamCopy(fcbaddr, 32, dmaAddr);
	d->uu = 0x00;
}

int _SelectDisk(dr)
{
	uint8 result;
	uint8 disk[2] = "A";

	if (dr) {
		disk[0] += (dr - 1);
	} else {
		disk[0] += _RamRead(0x0004);
	}
#ifdef ARDUINO
	SD.chdir();
	result = SD.chdir((char*)disk); // (todo) Test if it is Directory
#endif
#ifdef _WIN32
	result = (uint8)GetFileAttributes((LPCSTR)disk);
	result = (result == 0x10);
#endif
	if (result)
		loginVector = loginVector | (1 << (disk[0] - 'A'));
	return(result);
}

long _FileSize(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif
	long l = -1;

	if (_SelectDisk(F->dr)) {
		_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
		file = sd.open((char*)filename, O_RDONLY);
#endif
#ifdef _WIN32
		file = _fopen_r(&filename[0]);
#endif
		if (file != NULL) {
#ifdef ARDUINO
			l = sd.fileSize();
			sd.close();
#endif
#ifdef _WIN32
			_fseek(file, 0, SEEK_END);
			l = _ftell(file);
			_fclose(file);
#endif
		}
	}
	return(l);
}

uint8 _OpenFile(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif
	long l;
	int32 reqext;	// Required extention to open
	int32 b, i;

	if (_SelectDisk(F->dr)) {
		_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
		file = sd.open((char*)filename, O_READ);
#endif
#ifdef _WIN32
		file = _fopen_r(&filename[0]);
#endif
		if (file != NULL) {
#ifdef ARDUINO
			l = sd.fileSize();
			sd.close();
#endif
#ifdef _WIN32
			_fseek(file, 0, SEEK_END);
			l = _ftell(file);
			_fclose(file);
#endif

			reqext = (F->ex & 0x1f) | (F->s1 << 5);
			b = l - reqext * 16384;
			for (i = 0; i < 16; i++)
				F->al[i] = (b > i * 1024) ? i + 1 : 0;
			F->s1 = 0x00;
			F->rc = 0x80;
			result = 0x00;
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _CloseFile(uint16 fcbaddr)
{
	return(0x00);
}

uint8 _MakeFile(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif

	if (_SelectDisk(F->dr)) {
		if (!RW) {
			_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
			file = sd.open((char*)filename, O_CREAT | O_WRITE);
#endif
#ifdef _WIN32
			file = _fopen_w(&filename[0]);
#endif
			if (file != NULL) {
				result = 0x00;
#ifdef ARDUINO
				sd.close();
#endif
#ifdef _WIN32
				_fclose(file);
#endif
			}
		} else {
			_error(errWRITEPROT);
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _DeleteFile(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (_SelectDisk(F->dr)) {
		if (!RW) {
			_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
			if (SD.remove((char*)filename)) {
#endif
#ifdef _WIN32
			if (!_remove(&filename[0])) {
#endif
				result = 0x00;
			}
		} else {
			_error(errWRITEPROT);
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _RenameFile(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	uint8 newname[13];

	if (_SelectDisk(F->dr)) {
		if (!RW) {
			_GetFile(fcbaddr + 16, &newname[0]);
			_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
			if (SD.rename((char*)filename, (char*)newname)) {
#endif
#ifdef _WIN32
			if (!_rename(&filename[0], &newname[0])) {
#endif
				result = 0x00;
			}
		} else {
			_error(errWRITEPROT);
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _SearchFirst(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (_SelectDisk(F->dr)) {
		_GetFile(fcbaddr, &filename[0]);
		result = _findfirst();
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _SearchNext(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (_SelectDisk(F->dr)) {
		result = _findnext();
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _ReadSeq(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif
	uint8 bytesread;

	long fpos = (F->ex * 16384) + (F->cr * 128);

	if (_SelectDisk(F->dr)) {
		_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
		file = sd.open((char*)filename, O_READ);
#endif
#ifdef _WIN32
		file = _fopen_r(&filename[0]);
#endif
		if (file != NULL) {
#ifdef ARDUINO
			if (sd.seekSet(fpos)) {
#endif
#ifdef _WIN32
			if (!_fseek(file, fpos, 0)) {
#endif
				_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z prior to reading
#ifdef ARDUINO
				bytesread = sd.read(&RAM[dmaAddr], 128);
#endif
#ifdef _WIN32
				bytesread = (uint8)_fread(&RAM[dmaAddr], 1, 128, file);
#endif
				if (bytesread) {
					F->cr++;
					if (F->cr > 127) {
						F->cr = 0;
						F->ex++;
					}
					if (F->ex > 127) {
						// (todo) not sure what to do 
						result = 0xff;
					}
					result = 0x00;
				} else {
						result = 0x01;
				}
			} else {
				result = 0x01;
			}
#ifdef ARDUINO
			sd.close();
#endif
#ifdef _WIN32
			_fclose(file);
#endif
		} else {
			result = 0x10;
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _WriteSeq(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif

	long fpos = (F->ex * 16384) + (F->cr * 128);

	if (_SelectDisk(F->dr)) {
		if (!RW) {
			_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
			file = sd.open((char*)filename, O_RDWR);
#endif
#ifdef _WIN32
			file = _fopen_rw(&filename[0]);
#endif
			if (file != NULL) {
#ifdef ARDUINO
				if (sd.seekSet(fpos)) {
					if (sd.write(&RAM[dmaAddr], 128)) {
#endif
#ifdef _WIN32
				if (!_fseek(file, fpos, 0)) {
					if (_fwrite(&RAM[dmaAddr], 1, 128, file)) {
#endif
						F->cr++;
						if (F->cr > 127) {
							F->cr = 0;
							F->ex++;
						}
						if (F->ex > 127) {
							// (todo) not sure what to do 
						}
						result = 0x00;
					}
				} else {
					result = 0x01;
				}
#ifdef ARDUINO
				sd.close();
#endif
#ifdef _WIN32
				_fclose(file);
#endif
			} else {
				result = 0x10;
			}
		} else {
			_error(errWRITEPROT);
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _ReadRand(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif
	uint8 bytesread;
	int32 record = F->r0 | (F->r1 << 8);
	long fpos = record * 128;

	if (_SelectDisk(F->dr)) {
		_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
		file = sd.open((char*)filename, O_READ);
#endif
#ifdef _WIN32
		file = _fopen_r(&filename[0]);
#endif
		if (file != NULL) {
#ifdef ARDUINO
			if (sd.seekSet(fpos)) {
#endif
#ifdef _WIN32
			if (!_fseek(file, fpos, 0)) {
#endif
				_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z prior to reading
#ifdef ARDUINO
				bytesread = sd.read(&RAM[dmaAddr], 128);
#endif
#ifdef _WIN32
				bytesread = (uint8)_fread(&RAM[dmaAddr], 1, 128, file);
#endif
				if (bytesread) {
					F->cr = record & 0x7F;
					F->ex = (record >> 7) & 0x1f;
					F->s1 = (record >> 12) & 0xff;
					F->s2 = (record >> 20) & 0xff;
					result = 0x00;
				} else {
					result = 0x01;
				}
			} else {
				result = 0x06;
			}
#ifdef ARDUINO
			sd.close();
#endif
#ifdef _WIN32
			_fclose(file);
#endif
		} else {
			result = 0x10;
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}

uint8 _WriteRand(uint16 fcbaddr)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
#ifdef ARDUINO
	SdFile sd;
	int32 file;
#endif
#ifdef _WIN32
	FILE* file;
#endif
	int32 record = F->r0 | (F->r1 << 8);
	long fpos = record * 128;

	if (_SelectDisk(F->dr)) {
		if (!RW) {
			_GetFile(fcbaddr, &filename[0]);
#ifdef ARDUINO
			file = sd.open((char*)filename, O_RDWR);
#endif
#ifdef _WIN32
			file = _fopen_rw(&filename[0]);
#endif
			if (file != NULL) {
#ifdef ARDUINO
				if (sd.seekSet(fpos)) {
					if (sd.write(&RAM[dmaAddr], 128)) {
#endif
#ifdef _WIN32
				if (!_fseek(file, fpos, 0)) {
					if (_fwrite(&RAM[dmaAddr], 1, 128, file)) {
#endif
						F->cr = record & 0x7F;
						F->ex = (record >> 7) & 0x1f;
						F->s1 = (record >> 12) & 0xff;
						F->s2 = (record >> 20) & 0xff;
						result = 0x00;
					}
				} else {
					result = 0x06;
				}
#ifdef ARDUINO
				sd.close();
#endif
#ifdef _WIN32
				_fclose(file);
#endif
			} else {
				result = 0x10;
			}
		} else {
			_error(errWRITEPROT);
		}
	} else {
		_error(errSELECT);
	}
	return(result);
}


