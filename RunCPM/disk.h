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

/*
FCB related numbers
*/
#define BlkSZ 128	// CP/M block size
#define	BlkEX 128	// Number of blocks on an extension
#define BlkS2 4096	// Number of blocks on a S2 (module)
#define MaxCR 128	// Maximum value the CR field can take
#define MaxRC 127	// Maximum value the RC field can take
#define MaxEX 31	// Maximum value the EX field can take
#define MaxS2 15	// Maximum value the S2 (modules) field can take - Can be set to 63 to emulate CP/M Plus

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
	Status = _getch();
	_puts("\r\n");
	cDrive = oDrive;
	_RamWrite(0x0004, (_RamRead(0x0004) & 0xf0) | oDrive);
	Status = 2;
}

int _SelectDisk(uint8 dr) {
	uint8 result = 0xff;
	uint8 disk[2] = { 'A', 0 };

	if (!dr) {
		dr = cDrive;	// This will set dr to defDisk in case no disk is passed
	} else {
		--dr;			// Called from BDOS, set dr back to 0=A: format
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
	uint8 addDot = TRUE;
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;
	uint8 unique = TRUE;

	if (F->dr) {
		*(filename++) = (F->dr - 1) + 'A';
	} else {
		*(filename++) = cDrive + 'A';
	}
	*(filename++) = FOLDERCHAR;

	*(filename++) = toupper(tohex(userCode));
	*(filename++) = FOLDERCHAR;

	while (i < 8) {
		if (F->fn[i] > 32)
			*(filename++) = toupper(F->fn[i]);
		if (F->fn[i] == '?')
			unique = FALSE;
		++i;
	}
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32) {
			if (addDot) {
				addDot = FALSE;
				*(filename++) = '.';	// Only add the dot if there's an extension
			}
			*(filename++) = toupper(F->tp[i]);
		}
		if (F->tp[i] == '?')
			unique = FALSE;
		++i;
	}
	*filename = 0x00;

	return(unique);
}

void _HostnameToFCB(uint16 fcbaddr, uint8 *filename) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;

	++filename;
	if (*filename == FOLDERCHAR) {	// Skips the drive and / if needed
		filename += 3;
	} else {
		--filename;
	}

	while (*filename != 0 && *filename != '.') {
		F->fn[i] = toupper(*filename);
		++filename;
		++i;
	}
	while (i < 8) {
		F->fn[i] = ' ';
		++i;
	}
	if (*filename == '.')
		++filename;
	i = 0;
	while (*filename != 0) {
		F->tp[i] = toupper(*filename);
		++filename;
		++i;
	}
	while (i < 3) {
		F->tp[i] = ' ';
		++i;
	}
}

void _HostnameToFCBname(uint8 *from, uint8 *to) {	// Converts a string name (AB.TXT) to FCB name (AB      TXT)
	int i = 0;

	++from;
	if (*from == FOLDERCHAR) {	// Skips the drive and / if needed
		from += 3;
	} else {
		--from;
	}

	while (*from != 0 && *from != '.') {
		*to = toupper(*from);
		++to; ++from; ++i;
	}
	while (i < 8) {
		*to = ' ';
		++to;  ++i;
	}
	if (*from == '.')
		++from;
	i = 0;
	while (*from != 0) {
		*to = toupper(*from);
		++to; ++from; ++i;
	}
	while (i < 3) {
		*to = ' ';
		++to;  ++i;
	}
	*to = 0;
}

uint8 match(uint8 *fcbname, uint8 *pattern) {
	uint8 result = 1;
	uint8 i;

	for (i = 0; i < 12; ++i) {
		if (*pattern == '?' || *pattern == *fcbname) {
			++pattern; ++fcbname;
			continue;
		} else {
			result = 0;
			break;
		}
	}
	return(result);
}

long _FileSize(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	long r, l = -1;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		l = _sys_filesize(filename);
		r = l % BlkSZ;
		if (r)
			l = l + BlkSZ - r;
	}
	return(l);
}

uint8 _OpenFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;
	long len;
	int32 i;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		if (_sys_openfile(&filename[0])) {

			len = _FileSize(fcbaddr) / 128;	// Compute the len on the file in blocks

			F->s1 = 0x00;
			F->s2 = 0x00;
	
			F->rc = len > MaxRC ? 0x80 : (uint8)len;
			for (i = 0; i < 16; ++i)	// Clean up AL
				F->al[i] = 0x00;

			result = 0x00;
		}
	}
	return(result);
}

uint8 _CloseFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (fcbaddr == BatchFCB)
				_Truncate((char*)filename, F->rc);	// Truncate $$$.SUB to F->rc CP/M records so SUBMIT.COM can work
			result = 0x00;
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _MakeFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;
	uint8 i;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (_sys_makefile(&filename[0])) {
				F->ex = 0x00;	// Makefile also initializes the FCB (file becomes "open")
				F->s1 = 0x00;
				F->s2 = 0x00;
				F->rc = 0x00;
				for (i = 0; i < 16; ++i)	// Clean up AL
					F->al[i] = 0x00;
				F->cr = 0x00;
				result = 0x00;
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _SearchFirst(uint16 fcbaddr, uint8 isdir) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _findfirst(isdir);
	}
	return(result);
}

uint8 _SearchNext(uint16 fcbaddr, uint8 isdir) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(tmpFCB);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr))
		result = _findnext(isdir);
	return(result);
}

uint8 _DeleteFile(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
#if defined(USE_PUN) || defined(USE_LST)
	CPM_FCB *T = (CPM_FCB*)_RamSysAddr(tmpFCB);
#endif
	uint8 result = 0xff;
	uint8 deleted = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			result = _SearchFirst(fcbaddr, FALSE);	// FALSE = Does not create a fake dir entry when finding the file
			while (result != 0xff) {
#ifdef USE_PUN
				if (!strcmp((char*)T->fn, "PUN     TXT") && pun_open) {
					_sys_fclose(pun_dev);
					pun_open = FALSE;
				}
#endif
#ifdef USE_LST
				if (!strcmp((char*)T->fn, "LST     TXT") && lst_open) {
					_sys_fclose(lst_dev);
					lst_open = FALSE;
				}
#endif
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
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
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
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	long fpos =	((F->s2 & MaxS2) * BlkS2 * BlkSZ) + 
				(F->ex * BlkEX * BlkSZ) + 
				(F->cr * BlkSZ);

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _sys_readseq(&filename[0], fpos);
		if (!result) {	// Read succeeded, adjust FCB
			++F->cr;
			if (F->cr > MaxCR) {
				F->cr = 1;
				++F->ex;
			}
			if (F->ex > MaxEX) {
				F->ex = 0;
				++F->s2;
			}
			if (F->s2 > MaxS2)
				result = 0xfe;	// (todo) not sure what to do 
		}
	}
	return(result);
}

uint8 _WriteSeq(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	long fpos =	((F->s2 & MaxS2) * BlkS2 * BlkSZ) +
				(F->ex * BlkEX * BlkSZ) + 
				(F->cr * BlkSZ);

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			result = _sys_writeseq(&filename[0], fpos);
			if (!result) {	// Write succeeded, adjust FCB
				++F->cr;
				if (F->cr > MaxCR) {
					F->cr = 1;
					++F->ex;
				}
				if (F->ex > MaxEX) {
					F->ex = 0;
					++F->s2;
				}
				if (F->s2 > MaxS2)
					result = 0xfe;	// (todo) not sure what to do 
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _ReadRand(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	int32 record = (F->r2 << 16) | (F->r1 << 8) | F->r0;
	long fpos = record * BlkSZ;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _sys_readrand(&filename[0], fpos);
		if (!result) {	// Read succeeded, adjust FCB
			F->cr = record & 0x7F;
			F->ex = (record >> 7) & 0x1f;
			F->s2 = (record >> 12) & 0xff;
		}
	}
	return(result);
}

uint8 _WriteRand(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	int32 record = (F->r2 << 16) | (F->r1 << 8) | F->r0;
	long fpos = record * BlkSZ;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			result = _sys_writerand(&filename[0], fpos);
			if (!result) {	// Write succeeded, adjust FCB
				F->cr = record & 0x7F;
				F->ex = (record >> 7) & 0x1f;
				F->s2 = (record >> 12) & 0xff;
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

uint8 _GetFileSize(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;
	int32 count = _FileSize(DE) >> 7;

	if (count != -1) {
		F->r0 = count & 0xff;
		F->r1 = (count >> 8) & 0xff;
		F->r2 = (count >> 16) & 0xff;
		result = 0x00;
	}
	return(result);
}

uint8 _SetRandom(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0x00;

	int32 count = F->cr & 0x7f;
		  count += (F->ex & 0x1f) << 7;
		  count += F->s2 << 12;

	F->r0 = count & 0xff;
	F->r1 = (count >> 8) & 0xff;
	F->r2 = (count >> 16) & 0xff;

	return(result);
}

void _SetUser(uint8 user) {
	userCode = user & 0x1f;	// BDOS unoficially allows user areas 0-31
							// this may create folders from G-V if this function is called from an user program
							// It is an unwanted behavior, but kept as BDOS does it
	_MakeUserDir();			// Creates the user dir (0-F[G-V]) if needed
}

uint8 _MakeDisk(uint16 fcbaddr) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	return(_sys_makedisk(F->dr));
}

uint8 _CheckSUB(void) {
	uint8 result;
	uint8 oCode = userCode;							// Saves the current user code (original BDOS does not do this)
	_HostnameToFCB(tmpFCB, (uint8*)"$???????.???");	// The original BDOS in fact only looks for a file which start with $
#ifdef BATCHA
	_RamWrite(tmpFCB, 1);							// Forces it to be checked on drive A:
#endif
#ifdef BATCH0
	userCode = 0;									// Forces it to be checked on user 0
#endif
	result = (_SearchFirst(tmpFCB, FALSE) == 0x00) ? 0xff : 0x00;
	userCode = oCode;								// Restores the current user code
	return(result);
}

#ifdef HASLUA
uint8 _RunLua(uint16 fcbaddr) {
	uint8 luascript[17];
	uint8 result = 0xff;

	if (_FCBtoHostname(fcbaddr, &luascript[0])) {	// Script name must be unique
		if (!_SearchFirst(fcbaddr, FALSE)) {			// and must exist
			result = _RunLuaScript((char*)&luascript[0]);
		}
	}

	return(result);
}
#endif

#endif