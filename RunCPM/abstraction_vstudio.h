#ifndef ABSTRACT_H
#define ABSTRACT_H

/* see main.c for definition */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#endif

// Lua scripting support
#ifdef HASLUA
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "lua.h"
lua_State *L;
#endif

#define HostOS 0x00

/* Externals for abstracted functions need to go here */
FILE *_sys_fopen_r(uint8 *filename);
int _sys_fseek(FILE *file, long delta, int origin);
long _sys_ftell(FILE *file);
size_t _sys_fread(void *buffer, size_t size, size_t count, FILE *file);
int _sys_fclose(FILE *file);

/* Memory abstraction functions */
/*===============================================================================*/
void _RamLoad(uint8 *filename, uint16 address) {
	long l;
	FILE *file = _sys_fopen_r(filename);
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

BOOL _sys_exists(uint8 *filename) {
	return(GetFileAttributesA((char*)filename) != INVALID_FILE_ATTRIBUTES);
}

FILE* _sys_fopen_r(uint8 *filename) {
	return(fopen((const char*)filename, "rb"));
}

FILE* _sys_fopen_w(uint8 *filename) {
	return(fopen((const char*)filename, "wb"));
}

FILE* _sys_fopen_rw(uint8 *filename) {
	return(fopen((const char*)filename, "r+b"));
}

FILE* _sys_fopen_a(uint8 *filename) {
	return(fopen((const char*)filename, "a"));
}

int _sys_fseek(FILE *file, long delta, int origin) {
	return(fseek(file, delta, origin));
}

long _sys_ftell(FILE *file) {
	return(ftell(file));
}

size_t _sys_fread(void *buffer, size_t size, size_t count, FILE *file) {
	return(fread(buffer, size, count, file));
}

size_t _sys_fwrite(const void *buffer, size_t size, size_t count, FILE *file) {
	return(fwrite(buffer, size, count, file));
}

int _sys_feof(FILE *file) {
	return(feof(file));
}

int _sys_fclose(FILE *file) {
	return(fclose(file));
}

int _sys_remove(uint8 *filename) {
	return(remove((const char*)filename));
}

int _sys_rename(uint8 *name1, uint8 *name2) {
	return(rename((const char*)name1, (const char*)name2));
}

int _sys_select(uint8 *disk) {
	return((uint8)GetFileAttributes((LPCSTR)disk) == 0x10);
}

long _sys_filesize(uint8 *filename) {
	long l = -1;
	FILE *file = _sys_fopen_r(filename);
	if (file != NULL) {
		_sys_fseek(file, 0, SEEK_END);
		l = _sys_ftell(file);
		_sys_fclose(file);
	}
	return(l);
}

int _sys_openfile(uint8 *filename) {
	FILE *file = _sys_fopen_r(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_makefile(uint8 *filename) {
	FILE *file = _sys_fopen_a(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_deletefile(uint8 *filename) {
	return(!_sys_remove(filename));
}

int _sys_renamefile(uint8 *filename, uint8 *newname) {
	return(!_sys_rename(&filename[0], &newname[0]));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer) {
#ifdef CONSOLELOG
	puts((char *)buffer);
#else
	uint8 s = 0;
	while (*(buffer + s))	// Computes buffer size
		s++;
	FILE *file = _sys_fopen_a((uint8*)LogName);
	_sys_fwrite(buffer, 1, s, file);
	_sys_fclose(file);
#endif
}
#endif

uint8 _sys_readseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;

	FILE *file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			for (i = 0; i < 128; i++)
				dmabuf[i] = 0x1a;
			bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
			if (bytesread) {
				for (i = 0; i < 128; i++)
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

uint8 _sys_writeseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;

	FILE *file = _sys_fopen_rw(&filename[0]);
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

uint8 _sys_readrand(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;

	FILE *file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			for (i = 0; i < 128; i++)
				dmabuf[i] = 0x1a;
			bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
			if (bytesread) {
				for (i = 0; i < 128; i++)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x06;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writerand(uint8 *filename, long fpos) {
	uint8 result = 0xff;

	FILE *file = _sys_fopen_rw(&filename[0]);
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

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;
	uint8 found = 0;
	uint8 more = 1;

	if (dirPos == 0) {
		hFind = FindFirstFile((LPCSTR)filename, &FindFileData);
	} else {
		more = FindNextFile(hFind, &FindFileData);
	}

	while (hFind != INVALID_HANDLE_VALUE && more) {	// Skips folders and long file names
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			more = FindNextFile(hFind, &FindFileData);
			continue;
		}
		if (FindFileData.cAlternateFileName[0] != 0) {
			if (FindFileData.cFileName[0] != '.')	// Keeps files that are extension only
			{
				more = FindNextFile(hFind, &FindFileData);
				continue;
			}
		}
		found++; dirPos++;
		break;
	}
	if (found) {
		if (isdir) {
			_HostnameToFCB(dmaAddr, (uint8*)&FindFileData.cFileName[0]); // Create fake DIR entry
			_RamWrite(dmaAddr, 0);	// Sets the user of the requested file correctly on DIR entry
		}
		_RamWrite(tmpFCB, filename[0] - '@');							// Set the requested drive onto the tmp FCB
		_HostnameToFCB(tmpFCB, (uint8*)&FindFileData.cFileName[0]); // Set the file name onto the tmp FCB
		result = 0x00;
	} else {
		FindClose(hFind);
	}
	return(result);
}

uint8 _findfirst(uint8 isdir) {
	uint8 result = 0xff;

	dirPos = 0;
	return(_findnext(isdir));
}

uint8 _Truncate(char *fn, uint8 rc) {
	uint8 result = 0x00;
	LARGE_INTEGER fp;
	fp.QuadPart = rc * 128;
	wchar_t filename[15];
	MultiByteToWideChar(CP_ACP, 0, fn, -1, filename, 4096);
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

#ifdef USER_SUPPORT
void _MakeUserDir() {
	uint8 dFolder = cDrive + 'A';
	uint8 uFolder = toupper(tohex(userCode));

	uint8 path[4] = { dFolder, FOLDERCHAR, uFolder, 0 };

	CreateDirectory((char*)path, NULL);
}
#endif

#ifdef HASLUA
uint8 _RunLuaScript(char *filename) {

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

BOOL _signal_handler(DWORD signal) {
	BOOL result = FALSE;
	if (signal == CTRL_C_EVENT) {
		_ungetch(3);
		result = TRUE;
	}
	return(result);
}

void _console_init(void) {
	HANDLE hConsoleHandle = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;

	SetConsoleMode(hConsoleHandle, dwMode);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)_signal_handler, TRUE);
}

void _console_reset(void) {

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
void _putch(byte)
{

}
*/

void _clrscr(void) {
	system("cls");
}

#endif