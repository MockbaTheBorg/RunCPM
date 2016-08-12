/* see main.c for definition */

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#include "windows.h"
	#include "stdio.h"
#endif

/* Memory abstraction functions */
/*===============================================================================*/

uint8 _RamRead(uint16 address)
{
	if (address < ROMSTART) {
		return(RAM[address]);
	} else {
		return(ROM[address - ROMSTART]);
	}
}

void _RamWrite(uint16 address, uint8 value)
{
	if (address < ROMSTART) {
		RAM[address] = value;
	} else {
		ROM[address-ROMSTART] = value;
	}
}

/* Filesystem (disk) abstraction fuctions */
/*===============================================================================*/
uint8	filename[15];
uint8	newname[13];
uint16	dmaAddr = 0x0080;
uint16	roVector = 0;
uint16	loginVector = 0;

uint8	user = 0;	// Current CP/M user

WIN32_FIND_DATA FindFileData;
HANDLE hFind;

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

typedef struct {
	uint8 uu;
	uint8 fn[8];
	uint8 tp[3];
	uint8 xl, bc, xh, rc;
	uint8 al[16];
} CPM_DIR;

FILE* _fopen_r(uint8 *filename) {
	return(fopen((const char*)filename, "rb"));
}

FILE* _fopen_w(uint8 *filename) {
	return(fopen((const char*)filename, "wb"));
}

FILE* _fopen_rw(uint8 *filename) {
	return(fopen((const char*)filename, "r+b"));
}

FILE* _fopen_a(uint8 *filename) {
	return(fopen((const char*)filename, "a"));
}

int _fseek(FILE* file, long delta, int origin) {
	return(fseek(file, delta, origin));
}

long _ftell(FILE* file) {
	return(ftell(file));
}

long _fread(void *buffer, long size, long count, FILE* file) {
	return(fread(buffer, size, count, file));
}

long _fwrite(const void *buffer, long size, long count, FILE* file) {
	return(fwrite(buffer, size, count, file));
}

int _feof(FILE* file) {
	return(feof(file));
}

int _fclose(FILE* file) {
	return(fclose(file));
}

int _remove(uint8 *filename) {
	return(remove((const char*)filename));
}

int _rename(uint8 *name1, uint8 *name2) {
	return(rename((const char*)name1, (const char*)name2));
}

void _GetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	uint8 i = 0;
	if (F->dr) {
		*(filename++) = (F->dr - 1) + 'A';
	} else {
		*(filename++) = (_RamRead(0x0004) & 0x0f) + 'A';
	}
	*(filename++) = '\\';

	while (i < 8) {
		if (F->fn[i] > 32) {
			*(filename++) = F->fn[i];
		}
		i++;
	}
	*(filename++) = '.';
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32) {
			*(filename++) = F->tp[i];
		}
		i++;
	}
	*filename = 0x00;
}

void _SetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB* F = (CPM_FCB*)&RAM[fcbaddr];
	int32 i = 0;

	while (*filename != 0 && *filename != '.') {
		F->fn[i] = toupper(*filename);
		filename++;
		i++;
	}
	while (i < 8) {
		F->fn[i] = ' ';
		i++;
	}
	if (*filename == '.') {
		filename++;
	}
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

uint8 _findfirst(void) {
	uint8 result = 0xff;
	uint8 found = 0;
	uint8 more = 1;

	hFind = FindFirstFile((LPCSTR)filename, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		while (hFind != INVALID_HANDLE_VALUE&&more) {	// Skips folders and long file names
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				more = FindNextFile(hFind, &FindFileData);
				continue;
			}
			if (FindFileData.cAlternateFileName[0] != 0) {
				more = FindNextFile(hFind, &FindFileData);
				continue;
			}
			found++;
			break;
		}
		if (found) {
			_SetFile(dmaAddr, (uint8*)&FindFileData.cFileName[0]); // (todo) Create fake DIR entry
			_RamWrite(dmaAddr, 0x00);
			result = 0x00;
		} else {
			FindClose(hFind);
		}
	}
	return(result);
}

uint8 _findnext(void)
{
	uint8 result = 0xff;
	uint8 found = 0;
	uint8 more = 1;

	more = FindNextFile(hFind, &FindFileData);
	if (more) {
		while (more) {	// Skips folders and long file names
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				more = FindNextFile(hFind, &FindFileData);
				continue;
			}
			if (FindFileData.cAlternateFileName[0] != 0) {
				more = FindNextFile(hFind, &FindFileData);
				continue;
			}
			found++;
			break;
		}
		if (found) {
			_SetFile(dmaAddr, (uint8*)&FindFileData.cFileName[0]);	// (todo) Create fake DIR entry
			_RamWrite(dmaAddr, 0x00);
			result = 0x00;
		} else {
			FindClose(hFind);
		}
	}
	return(result);
}

/* Console abstraction functions */
/*===============================================================================*/

#include <conio.h>

BOOL _signal_handler(DWORD signal)
{
	if (signal == CTRL_C_EVENT)
	{
		_ungetch(3);
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}

void _console_init(void)
{
	HANDLE hConsoleHandle = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;

	SetConsoleMode(hConsoleHandle, dwMode);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)_signal_handler, TRUE);
}

void _console_reset(void)
{

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

void _clrscr(void)
{
	system("cls");
}

