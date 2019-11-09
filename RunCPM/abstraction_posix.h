#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <glob.h>
#ifdef PROFILE
#include <time.h>
#define millis() clock()/1000
#endif

// Lua scripting support
#ifdef HASLUA
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "lua.h"
lua_State *L;
#endif

#include "posix.h"

#define HostOS 0x02

#define BlkSZ 128	// CP/M block size
#define BlkEX 128	// Number of blocks on an extension
#define ExtSZ (BlkSZ * BlkEX)
#define MaxEX 31	// Maximum value the EX field can take
#define MaxS2 15	// Maximum value the S2 (modules) field can take - Can be set to 63 to emulate CP/M Plus

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
} CPM_DIRENTRY;

glob_t	pglob;
int	dirPos;

static char findNextDirName[17];
static uint16 fileRecords = 0;
static uint16 fileExtents = 0;
static uint16 fileExtentsUsed = 0;
static uint16 firstFreeAllocBlock;

void _mockupDirEntry() {
	CPM_DIRENTRY *DE = (CPM_DIRENTRY*)_RamSysAddr(dmaAddr);
	uint8 blocks;

	for (uint8 i = 0; i < sizeof(CPM_DIRENTRY); ++i) {
		_RamWrite(dmaAddr + i, 0x00); // zero out directory entry
	}
	_HostnameToFCB(dmaAddr, (uint8 *)findNextDirName);

	if (allUsers) {
		DE->dr = currFindUser; // set user code for return
	} else {
		DE->dr = userCode;
	}
	// does file fit in a single directory entry?
	if (fileExtents <= extentsPerDirEntry) {
		if (fileExtents) {
			DE->ex = (fileExtents - 1 + fileExtentsUsed) % (MaxEX + 1);
			DE->s2 = (fileExtents - 1 + fileExtentsUsed) / (MaxEX + 1);
			DE->rc = fileRecords - (BlkEX * (fileExtents - 1));
		}
		blocks = (fileRecords >> blockShift) + ((fileRecords & blockMask) ? 1 : 0);
		fileRecords = 0;
		fileExtents = 0;
		fileExtentsUsed = 0;
	} else { // no, max out the directory entry
		DE->ex = (extentsPerDirEntry - 1 + fileExtentsUsed) % (MaxEX + 1);
		DE->s2 = (extentsPerDirEntry - 1 + fileExtentsUsed) / (MaxEX + 1);
		DE->rc = BlkEX;
		blocks = numAllocBlocks<256 ? 16 : 8;
		// update remaining records and extents for next call
		fileRecords -= BlkEX * extentsPerDirEntry;
		fileExtents -= extentsPerDirEntry;
		fileExtentsUsed += extentsPerDirEntry;
	}
	// phoney up an appropriate number of allocation blocks
	if (numAllocBlocks < 256) {
		for (uint8 i = 0; i < blocks; ++i) {
			DE->al[i] = firstFreeAllocBlock++;
		}
	} else {
		for (uint8 i = 0; i < 2 * blocks; i += 2) {
			DE->al[i] = firstFreeAllocBlock & 0xFF;
			DE->al[i+1] = firstFreeAllocBlock >> 8;
			++firstFreeAllocBlock;
		}
	}
}

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;
	char dir[6] = {'?', FOLDERCHAR, 0, FOLDERCHAR, '*', 0};
	int i;
	struct stat st;
	uint32 bytes;

	if (allExtents && fileRecords) {
		_mockupDirEntry();
		result = 0;
	} else {
		dir[0] = filename[0];
		if (allUsers)
			dir[2] = '?';
		else
			dir[2] = filename[2];
		if (!glob(dir, 0, NULL, &pglob)) {
			for (i = dirPos; i < pglob.gl_pathc; ++i) {
				++dirPos;
				strncpy(findNextDirName,pglob.gl_pathv[i],sizeof(findNextDirName)-1);
				findNextDirName[sizeof(findNextDirName)-1] = 0;
				_HostnameToFCBname((uint8*)findNextDirName, fcbname);
				if (match(fcbname, pattern) && 
					(stat(findNextDirName, &st) == 0) && 
					((st.st_mode & S_IFREG) != 0) && 
					isxdigit(findNextDirName[2]) && 
					(isupper(findNextDirName[2]) || isdigit(findNextDirName[2]))) {
					if (allUsers)
						currFindUser = isdigit(findNextDirName[2]) ? findNextDirName[2]-'0' : findNextDirName[2]-'A'+10;
					if (isdir) {
						// account for host files that aren't multiples of the block size
						// by rounding their bytes up to the next multiple of blocks
						bytes = st.st_size;
						if (bytes & (BlkSZ-1)) {
							bytes = (bytes & ~(BlkSZ-1)) + BlkSZ;
						}
						fileRecords = bytes/BlkSZ;
						fileExtents = fileRecords/BlkEX + ((fileRecords & (BlkEX-1)) ? 1 : 0);
						firstFreeAllocBlock = firstBlockAfterDir;
						_mockupDirEntry();
					} else {
						fileRecords = 0;
						fileExtents = 0;
						fileExtentsUsed = 0;
						firstFreeAllocBlock = firstBlockAfterDir;
					}
					_RamWrite(tmpFCB, filename[0] - '@');
					_HostnameToFCB(tmpFCB, (uint8*)findNextDirName);
					result = 0x00;
					break;
				}
			}
			globfree(&pglob);
		}
	}
	return(result);
}

uint8 _findfirst(uint8 isdir) {
	dirPos = 0;	// Set directory search to start from the first position
	_HostnameToFCBname(filename, pattern);
	fileRecords = 0;
	fileExtents = 0;
	fileExtentsUsed = 0;
	return(_findnext(isdir));
}

uint8 _findnextallusers(uint8 isdir) {
	return _findnext(isdir);
}

uint8 _findfirstallusers(uint8 isdir) {
	dirPos = 0;
	strcpy((char*)pattern,"???????????");
	fileRecords = 0;
	fileExtents = 0;
	fileExtentsUsed = 0;
	return(_findnextallusers(isdir));
}

/* Console abstraction functions */
/*===============================================================================*/

#include <ncurses.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

static struct termios _old_term, _new_term;

void _console_init(void) {
	tcgetattr(0, &_old_term);

	_new_term = _old_term;

	_new_term.c_lflag &= ~ICANON; /* Input available immediately (no EOL needed) */
	_new_term.c_lflag &= ~ECHO; /* Do not echo input characters */
	_new_term.c_lflag &= ~ISIG; /* ^C and ^Z do not generate signals */
	_new_term.c_iflag &= INLCR; /* Translate NL to CR on input */

	tcsetattr(0, TCSANOW, &_new_term); /* Apply changes immediately */

	setvbuf(stdout, (char *)NULL, _IONBF, 0); /* Disable stdout buffering */
}

void _console_reset(void) {
	tcsetattr(0, TCSANOW, &_old_term);
}

int _kbhit(void) {
	struct pollfd pfds[1];

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN | POLLPRI | POLLRDBAND | POLLRDNORM;

	return (poll(pfds, 1, 0) == 1) && (pfds[0].revents & (POLLIN | POLLPRI | POLLRDBAND | POLLRDNORM));
}

uint8 _getch(void) {
	return getchar();
}

void _putch(uint8 ch) {
	putchar(ch);
}

uint8 _getche(void) {
	uint8 ch = _getch();

	_putch(ch);

	return ch;
}

void _clrscr(void) {
	uint8 ch = system("clear");
}

#endif
