#ifndef DISK_H
#define DISK_H

/* see main.c for definition */

#ifdef __linux__
#include <sys/stat.h>
#endif

#ifdef __DJGPP__
#include <dirent.h>
#endif

/*
Disk errors
*/
#define errWRITEPROT 1
#define errSELECT 2

#define RW	(roVector & (1 << F->dr))

void _error(uint8 error) {
	_puts("\r\nBdos Error on ");
	_putcon('A' + cDrive);
	_puts(" : ");
	switch (error) {
	case errWRITEPROT:
		_puts("R/O");
		break;
	case errSELECT:
		_puts("Select");
		break;
	default:
		_puts("\r\nCP/M ERR");
		break;
	}
	_getch();
	_puts("\r\n");
	cDrive = oDrive;
	RAM[0x0004] = (RAM[0x0004] & 0xf0) | oDrive;
	Status = 2;
}

int _SelectDisk(uint8 dr) {
	uint8 result = 0xff;
	uint8 disk[2] = "A";

	if (!dr) {
		dr = cDrive;	// This will set dr to defDisk in case no disk is passed
	} else {
		dr--;			// Called from BDOS, set dr back to 0=A: format
	}

	disk[0] += dr;
	if (_sys_select(&disk[0])) {
		loginVector = loginVector | (1 << (disk[0] - 'A'));
		result = 0x00;
	} else {
		_error(errSELECT);
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
		*(filename++) = cDrive + 'A';
	}
	*(filename++) = FOLDERCHAR;

	while (i < 8) {
		if (F->fn[i] > 32)
			*(filename++) = toupper(F->fn[i]);
		if (F->fn[i] == '?')
			unique = FALSE;
		i++;
	}
	*(filename++) = '.';
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32)
			*(filename++) = toupper(F->tp[i]);
		if (F->tp[i] == '?')
			unique = FALSE;
		i++;
	}
	*filename = 0x00;

	return(unique);
}

void _HostnameToFCB(uint16 fcbaddr, uint8 *filename) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;

	filename++;
	if (*filename == FOLDERCHAR) {	// Skips the drive and / if needed
		filename++;
	} else {
		filename--;
	}

	while (*filename != 0 && *filename != '.') {
		F->fn[i] = toupper(*filename);
		filename++;
		i++;
	}
	while (i < 8) {
		F->fn[i] = ' ';
		i++;
	}
	if (*filename == '.')
		filename++;
	i = 0;
	while (*filename != 0) {
		F->tp[i] = toupper(*filename);
		filename++;
		i++;
	}
	while (i < 3) {
		F->tp[i] = ' ';
		i++;
	}
}

void _HostnameToFCBname(uint8 *from, uint8 *to) {	// Converts a string name (AB.TXT) to FCB name (AB      TXT)
	int i = 0;

	from++;
	if (*from == FOLDERCHAR) {	// Skips the drive and / if needed
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

uint8 match(uint8 *fcbname, uint8 *pattern) {
	uint8 result = 1;
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

long _FileSize(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	long l = -1;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		l = _sys_filesize(filename);
	}
	return(l);
}

uint8 _OpenFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
	long l;
	int32 reqext;	// Required extention to open
	int32 b, i;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		if (_sys_openfile(&filename[0])) {
			l = _FileSize(fcbaddr);

			reqext = (F->ex & 0x1f) | (F->s1 << 5);
			b = l - reqext * 16384;
			for (i = 0; i < 16; i++)
				F->al[i] = (b > i * 1024) ? i + 1 : 0;
			F->s1 = 0x00;
			F->rc = (uint8)(l / 128);
			result = 0x00;
		}
	}
	return(result);
}

uint8 _CloseFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (fcbaddr == BatchFCB)
				_Truncate((char*)filename, F->rc);	// Truncate $$$.SUB to F->rc CP/M records so SUBMIT.COM can work
			F->ex = 0;
			F->cr = 0;
			F->r0 = 0;
			F->r1 = 0;
			F->r2 = 0;
			result = 0x00;
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _MakeFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (_sys_makefile(&filename[0]))
				result = 0x00;
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _SearchFirst(uint16 fcbaddr, uint8 isdir) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _findfirst(isdir);
	}
	return(result);
}

uint8 _SearchNext(uint16 fcbaddr, uint8 isdir) {
	CPM_FCB *F = (CPM_FCB*)&RAM[tmpFCB];
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr))
		result = _findnext(isdir);
	return(result);
}

uint8 _DeleteFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
	uint8 deleted = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			result = _SearchFirst(fcbaddr, FALSE);	// FALSE = Does not create a fake dir entry when finding the file
			while (result != 0xff) {
				_FCBtoHostname(tmpFCB, &filename[0]);
				if (_sys_deletefile(&filename[0]))
					deleted = 0x00;
				result = _SearchFirst(fcbaddr, FALSE);	// FALSE = Does not create a fake dir entry when finding the file
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(deleted);
}

uint8 _RenameFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_RamWrite(fcbaddr + 16, _RamRead(fcbaddr));	// Prevents rename from moving files among folders
			_FCBtoHostname(fcbaddr + 16, &newname[0]);
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (_sys_renamefile(&filename[0], &newname[0]))
				result = 0x00;
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _ReadSeq(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
	long fpos = (F->ex * 16384) + (F->cr * 128);

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _sys_readseq(&filename[0], fpos);
		if (!result) {	// Read succeeded, adjust FCB
			F->cr++;
			if (F->cr > 127) {
				F->cr = 0;
				F->ex++;
			}
			if (F->ex > 127)
				result = 0xff;	// (todo) not sure what to do 
		}
	}
	return(result);
}

uint8 _WriteSeq(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
	long fpos = (F->ex * 16384) + (F->cr * 128);

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			result = _sys_writeseq(&filename[0], fpos);
			if (!result) {	// Write succeeded, adjust FCB
				F->cr++;
				if (F->cr > 127) {
					F->cr = 0;
					F->ex++;
				}
				if (F->ex > 127)
					result = 0xff;	// (todo) not sure what to do 
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _ReadRand(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
	int32 record = F->r0 | (F->r1 << 8);
	long fpos = record * 128;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _sys_readrand(&filename[0], fpos);
		if (!result) {	// Read succeeded, adjust FCB
			F->cr = record & 0x7F;
			F->ex = (record >> 7) & 0x1f;
			F->s1 = (record >> 12) & 0xff;
			F->s2 = (record >> 20) & 0xff;
		}
	}
	return(result);
}

uint8 _WriteRand(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 result = 0xff;
	int32 record = F->r0 | (F->r1 << 8);
	long fpos = record * 128;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			result = _sys_writerand(&filename[0], fpos);
			if (!result) {	// Write succeeded, adjust FCB
				F->cr = record & 0x7F;
				F->ex = (record >> 7) & 0x1f;
				F->s1 = (record >> 12) & 0xff;
				F->s2 = (record >> 20) & 0xff;
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _CheckSUB(void) {
	_HostnameToFCB(tmpFCB, (uint8*)"$$$.SUB");
	return((_SearchFirst(tmpFCB, FALSE) == 0x00) ? 0xFF : 0x00);
}

#endif