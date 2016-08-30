/* see main.c for definition */

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#include <windows.h>
	#include <stdio.h>
	#include <conio.h>
#endif

/* Memory abstraction functions */
/*===============================================================================*/
static uint8 RAM[RAMSIZE];	// RAM must go into memory (SRAM or DRAM)

uint8 _RamRead(uint16 address)
{
	return(RAM[address]);
}

void _RamWrite(uint16 address, uint8 value)
{
	RAM[address] = value;
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

long _sys_fread(void *buffer, long size, long count, FILE *file) {
	return(fread(buffer, size, count, file));
}

long _sys_fwrite(const void *buffer, long size, long count, FILE *file) {
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

int _sys_select(uint8 *disk)
{
	return((uint8)GetFileAttributes((LPCSTR)disk) == 0x10);
}

long _sys_filesize(uint8 *filename)
{
	long l = -1;
	FILE* file = _sys_fopen_r(filename);
	if (file != NULL) {
		_sys_fseek(file, 0, SEEK_END);
		l = _sys_ftell(file);
		_sys_fclose(file);
	}
	return(l);
}

int _sys_openfile(uint8 *filename)
{
	FILE* file = _sys_fopen_r(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_makefile(uint8 *filename)
{
	FILE* file = _sys_fopen_a(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_deletefile(uint8 *filename)
{
	return(!_sys_remove(filename));
}

int _sys_renamefile(uint8 *filename, uint8 *newname)
{
	return(!_sys_rename(&filename[0], &newname[0]));
}

int_sys_readseq(uint8 *filename)
{

}

void _GetFile(uint16 fcbaddr, uint8* filename)
{
	CPM_FCB *F = (CPM_FCB*)&RAM[fcbaddr];
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
	uint8 i = 0;

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

uint8 _findfirst(uint8 dir)
{
	uint8 result = 0xff;
	uint8 found = 0;
	uint8 more = 1;

	hFind = FindFirstFile((LPCSTR)filename, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
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
			found++;
			break;
		}
		if (found) {
			if (dir) {
				_SetFile(dmaAddr, (uint8*)&FindFileData.cFileName[0]); // Create fake DIR entry
				_RamWrite(dmaAddr, 0);	// Sets the user of the requested file correctly on DIR entry
			}
			_SetFile(tmpfcb, (uint8*)&FindFileData.cFileName[0]); // Set the file name onto the tmp FCB
			result = 0x00;
		} else {
			FindClose(hFind);
		}
	}
	return(result);
}

uint8 _findnext(uint8 dir)
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
				if (FindFileData.cFileName[0] != '.')	// Keeps files that are extension only
				{
					more = FindNextFile(hFind, &FindFileData);
					continue;
				}
			}
			found++;
			break;
		}
		if (found) {
			if (dir) {
				_SetFile(dmaAddr, (uint8*)&FindFileData.cFileName[0]);	// Create fake DIR entry
				_RamWrite(dmaAddr, 0);	// Sets the user of the requested file correctly on DIR entry
			}
			_SetFile(tmpfcb, (uint8*)&FindFileData.cFileName[0]); // Set the file name onto the tmp FCB
			result = 0x00;
		} else {
			FindClose(hFind);
		}
	}
	return(result);
}

uint8 _Truncate(char* fn, uint8 rc)
{
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

/* Console abstraction functions */
/*===============================================================================*/

BOOL _signal_handler(DWORD signal)
{
	if (signal == CTRL_C_EVENT)
	{
		_ungetch(3);
		return(TRUE);
	} else {
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

