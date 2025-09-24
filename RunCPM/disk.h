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

#define RW	(roVector & (1 << cDrive))

// Prints out a BDOS error
void _error(uint8 error) {
	_puts("\r\nBdos Err on ");
	_putcon('A' + cDrive);
	_puts(": ");
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
	Status = _getcon();
	_puts("\r\n");
	cDrive = oDrive = _RamRead(DSKByte) & 0x0f;
	Status = STATUS_RESTART;
}

// Selects the disk to be used by the next disk function
int _SelectDisk(uint8 dr) {
	uint8 result = 0xff;
	uint8 disk[2] = { 'A', 0 };

	if (!dr || dr == '?') {
		dr = cDrive;	// This will set dr to defDisk in case no disk is passed
	} else {
		--dr;			// Called from BDOS, set dr back to 0=A: format
	}

	disk[0] += dr;
	if (_sys_select(&disk[0])) {
		loginVector = loginVector | (1 << (disk[0] - 'A'));
		result = 0x00;
	} else {
		cDrive = oDrive = dr;
		_error(errSELECT);
	}

	return(result);
}

// Converts a FCB entry onto a host OS filename string
uint8 _FCBtoHostname(uint16 fcbaddr, uint8* filename) {
	uint8 addDot = TRUE;
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;
	uint8 unique = TRUE;
	uint8 c;

	if (F->dr && F->dr != '?') {
		*(filename++) = (F->dr - 1) + 'A';
	} else {
		*(filename++) = cDrive + 'A';
	}
	*(filename++) = FOLDERCHAR;

	*(filename++) = toupper(tohex(userCode));
	*(filename++) = FOLDERCHAR;

	if (F->dr != '?') {
		while (i < 8) {
			c = F->fn[i] & 0x7F;
#ifdef NOSLASH
			if (c == '/')
				c = '_';
#endif
			if (c > 32)
				*(filename++) = toupper(c);
			if (c == '?')
				unique = FALSE;
			++i;
		}
		i = 0;
		while (i < 3) {
			c = F->tp[i] & 0x7F;
			if (c > 32) {
				if (addDot) {
					addDot = FALSE;
					*(filename++) = '.';  // Only add the dot if there's an extension
				}
#ifdef NOSLASH
				if (c == '/')
					c = '_';
#endif
				*(filename++) = toupper(c);
			}
			if (c == '?')
				unique = FALSE;
			++i;
		}
	} else {
		for (i = 0; i < 8; ++i)
			*(filename++) = '?';
		*(filename++) = '.';
		for (i = 0; i < 3; ++i)
			*(filename++) = '?';
		unique = FALSE;
	}
	*filename = 0x00;

	return(unique);
}

// Converts a host OS filename string onto a FCB entry
void _HostnameToFCB(uint16 fcbaddr, uint8* filename) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
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

// Converts a string name (AB.TXT) onto FCB name (AB      TXT)
void _HostnameToFCBname(uint8* from, uint8* to) {
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

// Creates a fake directory entry for the current dmaAddr FCB
void _mockupDirEntry(uint8 mode) {
	CPM_DIRENTRY* DirEntry = (CPM_DIRENTRY*)_RamSysAddr(dmaAddr);
	uint8 blocks, i;

	for (i = 0; i < sizeof(CPM_DIRENTRY); ++i)
		_RamWrite(dmaAddr + i, 0x00); // zero out directory entry
	unsigned char* shortName;
	if (mode) {
		shortName = (unsigned char*)&findNextDirName[strlen(FILEBASE)];
	} else {
		shortName = (unsigned char*)&findNextDirName[0];
	}
	_HostnameToFCB(dmaAddr, (uint8*)shortName);

    if (allUsers) {
        DirEntry->dr = currFindUser; // set user code for return
    } else {
        DirEntry->dr = userCode;
    }

    /* Ensure S1 is deterministic (zero) — we already zeroed the entry above,
       but make the intent explicit so callers/readers aren't surprised. */
    DirEntry->s1 = 0;

    // does file fit in a single directory entry?
    if (fileExtents <= extentsPerDirEntry) {
        if (fileExtents) {
            DirEntry->ex = (fileExtents - 1 + fileExtentsUsed) % (MaxEX + 1);
            DirEntry->s2 = (fileExtents - 1 + fileExtentsUsed) / (MaxEX + 1);
            DirEntry->rc = fileRecords - (BlkEX * (fileExtents - 1));
        }
        blocks = (fileRecords >> blockShift) + ((fileRecords & blockMask) ? 1 : 0);
        fileRecords = 0;
        fileExtents = 0;
        fileExtentsUsed = 0;
    } else { // no, max out the directory entry
        DirEntry->ex = (extentsPerDirEntry - 1 + fileExtentsUsed) % (MaxEX + 1);
        DirEntry->s2 = (extentsPerDirEntry - 1 + fileExtentsUsed) / (MaxEX + 1);
        DirEntry->rc = BlkEX;
        blocks = numAllocBlocks < 256 ? 16 : 8;
        // update remaining records and extents for next call
        fileRecords -= BlkEX * extentsPerDirEntry;
        fileExtents -= extentsPerDirEntry;
        fileExtentsUsed += extentsPerDirEntry;
    }

    /* SAFETY: clamp blocks so we never overflow DirEntry->al[].
       On small disks AL is 16 bytes (one byte per block),
       on large disks AL is 16 bytes but stored as 8 16-bit values (pairs). */
    uint8 maxBlocks = (numAllocBlocks < 256) ? 16 : 8;
    if (blocks > maxBlocks) blocks = maxBlocks;

    // phoney up an appropriate number of allocation blocks
    if (numAllocBlocks < 256) {
        for (i = 0; i < blocks; ++i)
            DirEntry->al[i] = (uint8)firstFreeAllocBlock++;
    } else {
        for (i = 0; i < 2 * blocks; i += 2) {
            DirEntry->al[i] = firstFreeAllocBlock & 0xFF;
            DirEntry->al[i + 1] = firstFreeAllocBlock >> 8;
            ++firstFreeAllocBlock;
        }
    }
}

// Matches a FCB name to a search pattern
uint8 match(uint8* fcbname, uint8* pattern) {
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

// Returns the size of a file
long _FileSize(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	long r, l = -1;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		l = _sys_filesize(filename);
		if (l != -1) {
			r = l % BlkSZ;
			if (r)
				l = l + BlkSZ - r;
		}
	}
	return(l);
}

// Opens a file
uint8 _OpenFile(uint16 fcbaddr) {
    CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
    uint8 result = 0xff;
    long records;
    int32 i;

    if (!_SelectDisk(F->dr)) {
        _FCBtoHostname(fcbaddr, &filename[0]);
        if (!filename[4])
            return(result);	// Invalid filename

        if (_sys_openfile(&filename[0])) {
            /* Get file size in 128-byte records.
               _FileSize already rounds up to a multiple of BlkSZ. */
            records = _FileSize(fcbaddr) / BlkSZ;
            if (records < 0)	// defensive: filesize failed for some reason
                return(result);

            F->s1 = 0x00;
            F->s2 = 0x80;	// set unmodified flag

            /* rc = number of 128-byte records in the current logical extent.
               Clamp to the extent size (BlkEX). */
            F->rc = (records > BlkEX) ? (uint8)BlkEX : (uint8)records;

            for (i = 0; i < 16; ++i)	// Clean up AL
                F->al[i] = 0x00;

            result = 0x00;
        }
    }
    return(result);
}

// Closes a file
uint8 _CloseFile(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!(F->s2 & 0x80)) {					// if file is modified
			if (!RW) {
				_FCBtoHostname(fcbaddr, &filename[0]);
				if (!filename[4])
					return(result);	// Invalid filename
				if (fcbaddr == BatchFCB)
					_Truncate((char*)filename, F->rc);	// Truncate $$$.SUB to F->rc CP/M records so SUBMIT.COM can work
				result = 0x00;
			} else {
				_error(errWRITEPROT);
			}
		} else {
			result = 0x00;
		}
	}
	return(result);
}

// Creates a file
uint8 _MakeFile(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;
	uint8 i;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (!filename[4])
				return(result);	// Invalid filename
			if (_sys_makefile(&filename[0])) {
				F->ex = 0x00;	// Makefile also initializes the FCB (file becomes "open")
				F->s1 = 0x00;
				F->s2 = 0x00;		// newly created files are already modified
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

// Searches for the first directory file
uint8 _SearchFirst(uint16 fcbaddr, uint8 isdir) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		if (!filename[4])
			return(result);	// Invalid filename
		allUsers = F->dr == '?';
		allExtents = F->ex == '?';
		if (allUsers) {
			result = _findfirstallusers(isdir);
		} else {
			result = _findfirst(isdir);
		}
	}
	return(result);
}

// Searches for the next directory file
uint8 _SearchNext(uint16 fcbaddr, uint8 isdir) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(tmpFCB);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (allUsers) {
			result = _findnextallusers(isdir);
		} else {
			result = _findnext(isdir);
		}
	}
	return(result);
}

// Deletes a file
uint8 _DeleteFile(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
#if defined(USE_PUN) || defined(USE_LST)
	CPM_FCB* T = (CPM_FCB*)_RamSysAddr(tmpFCB);
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
				if (_sys_deletefile(&filename[0])) {
					deleted = 0x00;
				} else {
					_error(errWRITEPROT);
					break;
				}
				result = _SearchFirst(fcbaddr, FALSE);	// FALSE = Does not create a fake dir entry when finding the file
			}
		} else {
			_error(errWRITEPROT);
		}
	}
	return(deleted);
}

// Renames a file
uint8 _RenameFile(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	if (!_SelectDisk(F->dr)) {
		if (!RW) {
			_RamWrite(fcbaddr + 16, _RamRead(fcbaddr));	// Prevents rename from moving files between folders
			_FCBtoHostname(fcbaddr + 16, &newname[0]);
			_FCBtoHostname(fcbaddr, &filename[0]);
			if (!newname[4])
				return(result);	// Invalid filename
			if (!filename[4])
				return(result);	// Invalid filename
			if (_sys_renamefile(&filename[0], &newname[0]))
				result = 0x00;
		} else {
			_error(errWRITEPROT);
		}
	}
	return(result);
}

// Sequential read
uint8 _ReadSeq(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 result = 0xff;

	long fpos = ((F->s2 & MaxS2) * BlkS2 * BlkSZ) +
		(F->ex * BlkEX * BlkSZ) +
		(F->cr * BlkSZ);

	if (!_SelectDisk(F->dr)) {
		_FCBtoHostname(fcbaddr, &filename[0]);
		result = _sys_readseq(&filename[0], fpos);
		if (!result) {	// Read succeeded, adjust FCB
			++F->cr;
			/* CR counts 0..(MaxCR-1) logically (0..127). When we reach MaxCR records
		       we must roll CR to 0 and advance EX. Use >= to catch MaxCR itself. */
		    if (F->cr >= MaxCR) {
		        F->cr = 0;
		        ++F->ex;
		    }
		    if (F->ex > MaxEX) {
		        F->ex = 0;
		        ++F->s2;
		    }
		    /* strip possible high-bit and compare S2 low bits against allowed MaxS2 */
		    if ((F->s2 & 0x7F) > MaxS2)
		        result = 0xfe;
		}
	}
	return(result);
}

// Sequential write
uint8 _WriteSeq(uint16 fcbaddr) {
    CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
    uint8 result = 0xff;

    long fpos = ((F->s2 & MaxS2) * BlkS2 * BlkSZ) +
        (F->ex * BlkEX * BlkSZ) +
        (F->cr * BlkSZ);

    if (!_SelectDisk(F->dr)) {
        if (!RW) {
            _FCBtoHostname(fcbaddr, &filename[0]);
            result = _sys_writeseq(&filename[0], fpos);
            if (!result) {	// Write succeeded, adjust FCB
                /* clear unmodified flag (bit 7) */
                F->s2 &= 0x7F;

                /* advance record index; records are 0..(MaxCR-1) */
                ++F->cr;

                /* if we've rolled past the last record in the extent,
                   start at record 0 of the next extent */
                if (F->cr >= MaxCR) {
                    F->cr = 0;
                    ++F->ex;
                    /* first record in the new extent */
                    F->rc = 1;
                } else {
                    /* still in same extent - increment rc */
                    ++F->rc;
                }

                /* handle extent overflow -> advance S2/module */
                if (F->ex > MaxEX) {
                    F->ex = 0;
                    ++F->s2;
                    /* first record in the new module/extents group */
                    F->rc = 1;
                }

                /* check S2 numeric overflow (ignore high-bit flag) */
                if ((F->s2 & 0x7F) > MaxS2)
                    result = 0xfe;
            }
        } else {
            _error(errWRITEPROT);
        }
    }
    return(result);
}

// Random read
uint8 _ReadRand(uint16 fcbaddr) {
    CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
    uint8 result = 0xff;

    int32 record = (F->r2 << 16) | (F->r1 << 8) | F->r0;
    long fpos = record * BlkSZ;

    if (!_SelectDisk(F->dr)) {
        _FCBtoHostname(fcbaddr, &filename[0]);
        result = _sys_readrand(&filename[0], fpos);
        if (result == 0 || result == 1 || result == 4) {
            // adjust FCB unless error #6 (seek past 8MB - max CP/M file & disk size)
            F->cr = record & (MaxCR - 1);
            F->ex = (record >> 7) & MaxEX;
            /* preserve 0x80 (unmodified) bit in s2 if previously present */
            F->s2 = ((record >> 12) & MaxS2) | (F->s2 & 0x80);
        }
    }
    return(result);
}

// Random write
uint8 _WriteRand(uint16 fcbaddr) {
    CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
    uint8 result = 0xff;

    int32 record = (F->r2 << 16) | (F->r1 << 8) | F->r0;
    long fpos = record * BlkSZ;

    if (!_SelectDisk(F->dr)) {
        if (!RW) {
            _FCBtoHostname(fcbaddr, &filename[0]);
            result = _sys_writerand(&filename[0], fpos);
            if (!result) {	// Write succeeded, adjust FCB
                F->cr = record & (MaxCR - 1);
                F->ex = (record >> 7) & MaxEX;
                F->s2 = (record >> 12) & MaxS2;	// resets unmodified flag
            }
        } else {
            _error(errWRITEPROT);
        }
    }
    return(result);
}

// Returns the size of a CP/M file
uint8 _GetFileSize(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
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

// Set the next random record
uint8 _SetRandom(uint16 fcbaddr) {
    CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
    uint8 result = 0x00;

    int32 count = F->cr & (MaxCR - 1);
    count += (F->ex & MaxEX) << 7;
    count += (F->s2 & MaxS2) << 12;

    F->r0 = count & 0xff;
    F->r1 = (count >> 8) & 0xff;
    F->r2 = (count >> 16) & 0xff;

    return(result);
}

// Sets the current user area
void _SetUser(uint8 user) {
	userCode = user & 0x1f;	// BDOS unoficially allows user areas 0-31
							// this may create folders from G-V if this function is called from an user program
							// It is an unwanted behavior, but kept as BDOS does it
#ifdef NOHIGHUSER
	if(userCode < 16)
#endif
	_MakeUserDir();			// Creates the user dir (0-F[G-V]) if needed
}

// Creates a disk directory folder
uint8 _MakeDisk(uint16 fcbaddr) {
	CPM_FCB* F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	return(_sys_makedisk(F->dr));
}

// Checks if there's a temp submit file present
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

#endif
