#ifndef ABSTRACT_H
#define ABSTRACT_H

/* see main.c for definition */

//#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <wincon.h>
//#endif
#define millis() clock()

#define HostOS 0x00

/* Externals for abstracted functions need to go here */
FILE* _sys_fopen_r(uint8* filename);
int _sys_fseek(FILE* file, long delta, int origin);
long _sys_ftell(FILE* file);
size_t _sys_fread(void* buffer, size_t size, size_t count, FILE* file);
int _sys_fclose(FILE* file);

/* Memory abstraction functions */
/*===============================================================================*/
uint16 _RamLoad(uint8* filename, uint16 address, uint16 maxsize) {
	long l;
	FILE* file = _sys_fopen_r(filename);
	_sys_fseek(file, 0, SEEK_END);
	l = _sys_ftell(file);
	if (maxsize && l > maxsize)
		l = maxsize;
	_sys_fseek(file, 0, SEEK_SET);
	_sys_fread(_RamSysAddr(address), 1, l, file); // (todo) This can overwrite past RAM space

	_sys_fclose(file);
	return (uint16)l;
}

/* Filesystem (disk) abstraction functions */
/*===============================================================================*/
WIN32_FIND_DATA FindFileData;
HANDLE hFind;
int dirPos;
#define FOLDERCHAR '\\'
#define FILEBASE ".\\"

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
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(GetFileAttributesA((char*)fullpath) != INVALID_FILE_ATTRIBUTES);
}

FILE* _sys_fopen_r(uint8* filename) {
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(fopen((const char*)fullpath, "rb"));
}

FILE* _sys_fopen_w(uint8* filename) {
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(fopen((const char*)fullpath, "wb"));
}

FILE* _sys_fopen_rw(uint8* filename) {
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(fopen((const char*)fullpath, "r+b"));
}

FILE* _sys_fopen_a(uint8* filename) {
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(fopen((const char*)fullpath, "a"));
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
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(remove((const char*)fullpath));
}

int _sys_rename(uint8* name1, uint8* name2) {
	uint8 fullpath1[128] = FILEBASE;
	strcat((char*)fullpath1, (char*)name1);
	uint8 fullpath2[128] = FILEBASE;
	strcat((char*)fullpath2, (char*)name2);
	return(rename((const char*)fullpath1, (const char*)fullpath2));
}

int _sys_select(uint8* disk) {
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)disk);
	uint32 attr = GetFileAttributes((LPCSTR)fullpath);
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
				result = fpos < extSize ? 0x01 : 0x04;
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
	filename[2]++; // This needs to be improved so it doesn't stop searching once there's an user area gap
	if (filename[2] == ':')
		filename[2] = 'A';
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	hFind = FindFirstFile((LPCSTR)fullpath, &FindFileData);
}

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;
	uint8 found = 0;
	uint8 more = 1;
	uint32 bytes;

	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);

	if (allExtents && fileRecords) {
		_mockupDirEntry(0);
		result = 0;
	} else {
		if (dirPos == 0) {
			hFind = FindFirstFile((LPCSTR)fullpath, &FindFileData);
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
				_mockupDirEntry(0);
			} else {
				fileRecords = 0;
				fileExtents = 0;
				fileExtentsUsed = 0;
				firstFreeAllocBlock = firstBlockAfterDir;
			}
			_RamWrite(tmpFCB, filename[0] - '@');						// Set the requested drive onto the tmp FCB
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
	wchar_t filename[128];
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, fn);
	MultiByteToWideChar(CP_ACP, 0, (char*)fullpath, -1, filename, (int)strlen((char*)fullpath)+1);
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
	uint8 fullpath[128] = FILEBASE;

	strcat((char*)fullpath, (char*)path);
	CreateDirectory((char*)fullpath, NULL);
}

uint8 _sys_makedisk(uint8 drive) {
	uint8 result = 0;
	if (drive < 1 || drive>16) {
		result = 0xff;
	} else {
		uint8 dFolder = drive + '@';
		uint8 disk[2] = { dFolder, 0 };
		uint8 fullpath1[128] = FILEBASE;
		strcat((char*)fullpath1, (char*)disk);
		if (!CreateDirectory((char*)fullpath1, NULL)) {
			result = 0xfe;
		} else {
			uint8 path[4] = { dFolder, FOLDERCHAR, '0', 0 };
			uint8 fullpath2[128] = FILEBASE;
			strcat((char*)fullpath2, (char*)path);
			CreateDirectory((char*)fullpath2, NULL);
		}
	}

	return(result);
}

/* Hardware abstraction functions */
/*===============================================================================*/
void _HardwareOut(const uint32 Port, const uint32 Value) {

}

uint32 _HardwareIn(const uint32 Port) {
	return 0;
}

/* Host initialization functions */
/*===============================================================================*/

#ifdef STREAMIO
static void _abort_if_kbd_eof() {
}

static void _file_failure_exit(char *argv[], char* fmt, char* filename)
{
	fprintf(stderr, "%s: ", argv[0]);
	fprintf(stderr, fmt, filename);
	if (errno) {
		fprintf(stderr, ": %s", strerror(errno));
	}
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

static void _usage(char *argv[]) {
	fprintf(stderr,
		"RunCPM - an emulator to run CP/M programs on modern hosts\n"
		"usage: %s [-i input_file] [-o output_file] [-s]\n", argv[0]);
	fprintf(stderr,
		"  -i input_file: console input will be read from the file "
		"with the\n     given name. "
		"After input file's EOF, further console input\n     will be read "
		"from the keyboard.\n");
	fprintf(stderr,
		"  -o output_file: console output will be written to the file "
		"with the\n     given name, in addition to the screen.\n");
	fprintf(stderr,
		"  -s: console input and output is connected directly to "
		"stdin and stdout.\n");
}

#define SET_OPTARG ++i; if (i >= argc) {++errflg; break;} optarg = argv[i];

static void _parse_options(int argc, char *argv[]) {
	int errflg = 0;
	char *optarg;
	for (int i = 1; i < argc && errflg == 0; ++i) {
		if (strcmp("-i", argv[i]) == 0) {
			/* ++i;
			if (i >= argc) {
				++errflg;
				break;
			}
			optarg = argv[i]; */
			SET_OPTARG
			streamInputFile = fopen(optarg, "r");
			if (NULL == streamInputFile) {
				_file_failure_exit(argv,
					"error opening console input file %s", optarg);
			}
			streamInputActive = TRUE;
			continue;
		}
		if (strcmp("-o", argv[i]) == 0) {
			SET_OPTARG
			streamOutputFile = fopen(optarg, "w");
			if (NULL == streamOutputFile) {
				_file_failure_exit(argv,
					"error opening console output file %s", optarg);
			}
			continue;
		}
		if (strcmp("-s", argv[i]) == 0) {
			streamInputFile = stdin;
			streamOutputFile = stdout;
			streamInputActive = TRUE;
			consoleOutputActive = FALSE;
			continue;
		}
		fprintf(stderr,
			"Unrecognized option: '%s'\n", argv[i]);
		errflg++;
	}
	if (errflg) {
		_usage(argv);
		exit(EXIT_FAILURE);
	}
}
#endif

#ifdef STREAMIO
void _host_init(int argc, char* argv[]) {
	_parse_options(argc, argv);
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

//	SetConsoleMode(hOutHandle, cOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
//	SetConsoleMode(hInHandle, cInMode | ENABLE_VIRTUAL_TERMINAL_INPUT);
	SetConsoleTitle("RunCPM v" VERSION);

	setvbuf(stdin, NULL, _IONBF, 256);
	setvbuf(stdout, NULL, _IONBF, 0);

	if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)_signal_handler, TRUE)) {
		_puts("Error setting ^C signal handler.\n");
		exit(EXIT_FAILURE);
	}
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
