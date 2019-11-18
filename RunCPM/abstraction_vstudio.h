#ifndef ABSTRACT_H
#define ABSTRACT_H

/* see main.c for definition */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#ifdef PROFILE
#include <time.h>
#define millis() clock()
#endif
#endif

// Lua scripting support
#ifdef HASLUA
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "lua.h"
lua_State* L;
#endif

#define HostOS 0x00

/* Externals for abstracted functions need to go here */
FILE* _sys_fopen_r(uint8* filename);
int _sys_fseek(FILE* file, long delta, int origin);
long _sys_ftell(FILE* file);
size_t _sys_fread(void* buffer, size_t size, size_t count, FILE* file);
int _sys_fclose(FILE* file);

/* Memory abstraction functions */
/*===============================================================================*/
void _RamLoad(uint8* filename, uint16 address) {
	long l;
	FILE* file = _sys_fopen_r(filename);
	_sys_fseek(file, 0, SEEK_END);
	l = _sys_ftell(file);

	_sys_fseek(file, 0, SEEK_SET);
	_sys_fread(_RamSysAddr(address), 1, l, file); // (todo) This can overwrite past RAM space

	_sys_fclose(file);
}

/* Filesystem (disk) abstraction fuctions */
/*===============================================================================*/
WIN32_FIND_DATA FindFileData;
HANDLE hFind;
int dirPos;
#define FOLDERCHAR '\\'

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
} CPM_DIRENTRY;

BOOL _sys_exists(uint8* filename) {
	return(GetFileAttributesA((char*)filename) != INVALID_FILE_ATTRIBUTES);
}

FILE* _sys_fopen_r(uint8* filename) {
	return(fopen((const char*)filename, "rb"));
}

FILE* _sys_fopen_w(uint8* filename) {
	return(fopen((const char*)filename, "wb"));
}

FILE* _sys_fopen_rw(uint8* filename) {
	return(fopen((const char*)filename, "r+b"));
}

FILE* _sys_fopen_a(uint8* filename) {
	return(fopen((const char*)filename, "a"));
}

int _sys_fseek(FILE* file, long delta, int origin) {
	return(fseek(file, delta, origin));
}

long _sys_ftell(FILE* file) {
	return(ftell(file));
}

size_t _sys_fread(void* buffer, size_t size, size_t count, FILE* file) {
	return(fread(buffer, size, count, file));
}

size_t _sys_fwrite(const void* buffer, size_t size, size_t count, FILE* file) {
	return(fwrite(buffer, size, count, file));
}

int _sys_fputc(int ch, FILE* file) {
	return(fputc(ch, file));
}

int _sys_feof(FILE* file) {
	return(feof(file));
}

int _sys_fflush(FILE* file) {
	return(fflush(file));
}

int _sys_fclose(FILE* file) {
	return(fclose(file));
}

int _sys_remove(uint8* filename) {
	return(remove((const char*)filename));
}

int _sys_rename(uint8* name1, uint8* name2) {
	return(rename((const char*)name1, (const char*)name2));
}

int _sys_select(uint8* disk) {
	uint32 attr = GetFileAttributes((LPCSTR)disk);
	return(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

long _sys_filesize(uint8* filename) {
	long l = -1;
	FILE* file = _sys_fopen_r(filename);
	if (file != NULL) {
		_sys_fseek(file, 0, SEEK_END);
		l = _sys_ftell(file);
		_sys_fclose(file);
	}
	return(l);
}

int _sys_openfile(uint8* filename) {
	FILE* file = _sys_fopen_r(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_makefile(uint8* filename) {
	FILE* file = _sys_fopen_a(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_deletefile(uint8* filename) {
	return(!_sys_remove(filename));
}

int _sys_renamefile(uint8* filename, uint8* newname) {
	return(!_sys_rename(&filename[0], &newname[0]));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8* buffer) {
#ifdef CONSOLELOG
	puts((char*)buffer);
#else
	uint8 s = 0;
	while (*(buffer + s))	// Computes buffer size
		++s;
	FILE* file = _sys_fopen_a((uint8*)LogName);
	_sys_fwrite(buffer, 1, s, file);
	_sys_fclose(file);
#endif
}
#endif

uint8 _sys_readseq(uint8* filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;

	FILE* file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			for (i = 0; i < 128; ++i)
				dmabuf[i] = 0x1a;
			bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
			if (bytesread) {
				for (i = 0; i < 128; ++i)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x01;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writeseq(uint8* filename, long fpos) {
	uint8 result = 0xff;

	FILE* file = _sys_fopen_rw(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file))
				result = 0x00;
		} else {
			result = 0x01;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_readrand(uint8* filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;
	long extSize;

	FILE* file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			for (i = 0; i < 128; ++i)
				dmabuf[i] = 0x1a;
			bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
			if (bytesread) {
				for (i = 0; i < 128; ++i)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			if (fpos >= 65536L * 128) {
				result = 0x06;	// seek past 8MB (largest file size in CP/M)
			} else {
				_sys_fseek(file, 0, SEEK_END);
				extSize = _sys_ftell(file);
				// round file size up to next full logical extent
				extSize = 16384 * ((extSize / 16384) + ((extSize % 16384) ? 1 : 0));
				if (fpos < extSize)
					result = 0x01;	// reading unwritten data
				else
					result = 0x04; // seek to unwritten extent
			}
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writerand(uint8* filename, long fpos) {
	uint8 result = 0xff;

	FILE* file = _sys_fopen_rw(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file))
				result = 0x00;
		} else {
			result = 0x06;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

static char findNextDirName[17];
static uint16 fileRecords = 0;
static uint16 fileExtents = 0;
static uint16 fileExtentsUsed = 0;
static uint16 firstFreeAllocBlock;

// Selects next user area
void NextUserArea() {
	FindClose(hFind);
	filename[2]++; // This needs to be improved to it doesn't stop searching once there's an user area gap
	if (filename[2] == ':')
		filename[2] = 'A';
	hFind = FindFirstFile((LPCSTR)filename, &FindFileData);
}

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;
	uint8 found = 0;
	uint8 more = 1;
	uint32 bytes;

	if (allExtents && fileRecords) {
		_mockupDirEntry();
		result = 0;
	} else {
		if (dirPos == 0) {
			hFind = FindFirstFile((LPCSTR)filename, &FindFileData);
		} else {
			more = FindNextFile(hFind, &FindFileData);
			if (allUsers && !more) {
				NextUserArea();
				more++;
			}
		}

		while (hFind != INVALID_HANDLE_VALUE && more) {	// Skips folders and long file names
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				more = FindNextFile(hFind, &FindFileData);
				if (allUsers && !more) {
					NextUserArea();
					more++;
				}
				continue;
			}
			if (FindFileData.cAlternateFileName[0] != 0) {
				if (FindFileData.cFileName[0] != '.')	// Keeps files that are extension only
				{
					more = FindNextFile(hFind, &FindFileData);
					if (allUsers && !more) {
						NextUserArea();
						more++;
					}
					continue;
				}
			}
			++found; ++dirPos;
			break;
		}
		if (found) {
			if (isdir) {
				for (int i = 0; i < 13; i++)
					findNextDirName[i] = FindFileData.cFileName[i];
				// account for host files that aren't multiples of the block size
				// by rounding their bytes up to the next multiple of blocks
				bytes = FindFileData.nFileSizeLow;
				if (bytes & (BlkSZ - 1)) {
					bytes = (bytes & ~(BlkSZ - 1)) + BlkSZ;
				}
				fileRecords = bytes / BlkSZ;
				fileExtents = fileRecords / BlkEX + ((fileRecords & (BlkEX - 1)) ? 1 : 0);
				fileExtentsUsed = 0;
				firstFreeAllocBlock = firstBlockAfterDir;
				_mockupDirEntry();
			} else {
				fileRecords = 0;
				fileExtents = 0;
				fileExtentsUsed = 0;
				firstFreeAllocBlock = firstBlockAfterDir;
			}
			_RamWrite(tmpFCB, filename[0] - '@');							// Set the requested drive onto the tmp FCB
			_HostnameToFCB(tmpFCB, (uint8*)&FindFileData.cFileName[0]); // Set the file name onto the tmp FCB
			result = 0x00;
		} else {
			FindClose(hFind);
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
	strcpy((char*)pattern, "???????????");
	_HostnameToFCBname(filename, pattern);
	filename[2] = '0';
	fileRecords = 0;
	fileExtents = 0;
	fileExtentsUsed = 0;
	return(_findnextallusers(isdir));
}

uint8 _Truncate(char* fn, uint8 rc) {
	uint8 result = 0x00;
	LARGE_INTEGER fp;
	fp.QuadPart = (LONGLONG)rc * 128;
	wchar_t filename[15];
	MultiByteToWideChar(CP_ACP, 0, fn, -1, filename, 15);
	HANDLE fh = CreateFileW(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		result = 0xff;
	} else {
		if (SetFilePointerEx(fh, fp, NULL, FILE_BEGIN) == 0 || SetEndOfFile(fh) == 0)
			result = 0xff;
	}
	CloseHandle(fh);
	return(result);
}

void _MakeUserDir() {
	uint8 dFolder = cDrive + 'A';
	uint8 uFolder = toupper(tohex(userCode));

	uint8 path[4] = { dFolder, FOLDERCHAR, uFolder, 0 };

	CreateDirectory((char*)path, NULL);
}

uint8 _sys_makedisk(uint8 drive) {
	uint8 result = 0;
	if (drive < 1 || drive>16) {
		result = 0xff;
	} else {
		uint8 dFolder = drive + '@';
		uint8 disk[2] = { dFolder, 0 };
		if (!CreateDirectory((char*)disk, NULL)) {
			result = 0xfe;
		} else {
			uint8 path[4] = { dFolder, FOLDERCHAR, '0', 0 };
			CreateDirectory((char*)path, NULL);
		}
	}

	return(result);
}

#ifdef HASLUA
uint8 _RunLuaScript(char* filename) {

	L = luaL_newstate();
	luaL_openlibs(L);

	// Register Lua functions
	lua_register(L, "BdosCall", luaBdosCall);
	lua_register(L, "RamRead", luaRamRead);
	lua_register(L, "RamWrite", luaRamWrite);
	lua_register(L, "RamRead16", luaRamRead16);
	lua_register(L, "RamWrite16", luaRamWrite16);
	lua_register(L, "ReadReg", luaReadReg);
	lua_register(L, "WriteReg", luaWriteReg);

	int result = luaL_loadfile(L, filename);
	if (result) {
		_puts(lua_tostring(L, -1));
	} else {
		result = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (result)
			_puts(lua_tostring(L, -1));
	}

	lua_close(L);

	return(result);
}
#endif

/* Console abstraction functions */
/*===============================================================================*/
DWORD cOutMode; // Stores initial console mode for the std output
DWORD cInMode; // Stores initial console mode for the std input
TCHAR cTitle[MAX_PATH]; // Stores the initial console title

BOOL _signal_handler(DWORD signal) {
	BOOL result = FALSE;
	if (signal == CTRL_C_EVENT) {
		_ungetch(3);
		result = TRUE;
	}
	return(result);
}

void _console_init(void) {
	HANDLE hOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hInHandle = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(hOutHandle, &cOutMode);
	GetConsoleMode(hInHandle, &cInMode);

	GetConsoleTitle(cTitle, MAX_PATH);

	SetConsoleMode(hOutHandle, cOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
	SetConsoleMode(hInHandle, cInMode | ENABLE_VIRTUAL_TERMINAL_INPUT);
	SetConsoleTitle("RunCPM v" VERSION);

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)_signal_handler, TRUE);
}

void _console_reset(void) {
	HANDLE hOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hInHandle = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hOutHandle, cOutMode);
	SetConsoleMode(hInHandle, cInMode);
	SetConsoleTitle(cTitle);
}

/* Implemented by conio.h
int _kbhit(void)
{

}
*/

/* Implemented by conio.h
byte _getch(void)
{

}
*/

/* Implemented by conio.h
byte _getche(void)
{

}
*/

/* Implemented by conio.h
void _putch(uint8 byte)
{

}
*/

void _clrscr(void) {
	system("cls");
}

#endif
