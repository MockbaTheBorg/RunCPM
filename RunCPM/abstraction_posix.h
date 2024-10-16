#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <errno.h>
#include <glob.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <sys/stat.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#define millis() clock()/1000

#ifdef STREAMIO
#include <termios.h>
#endif

#define HostOS 0x02

/* Externals for abstracted functions need to go here */
FILE* _sys_fopen_r(uint8* filename);
int _sys_fseek(FILE* file, long delta, int origin);
long _sys_ftell(FILE* file);
long _sys_fread(void* buffer, long size, long count, FILE* file);
int _sys_fflush(FILE* file);
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
#define FOLDERCHAR '/'
#define FILEBASE "./"

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

uint8 _sys_exists(uint8* filename) {
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)filename);
	return(!access((const char*)fullpath, F_OK));
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

long _sys_fread(void* buffer, long size, long count, FILE* file) {
	return(fread(buffer, size, count, file));
}

long _sys_fwrite(const void* buffer, long size, long count, FILE* file) {
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
	struct stat st;
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)disk);
	return((stat((char*)fullpath, &st) == 0) && ((st.st_mode & S_IFDIR) != 0));
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
	FILE* file;
#ifdef CONSOLELOG
	puts((char*)buffer);
#else
	uint8 s = 0;
	while (*(buffer + s))	// Computes buffer size
		++s;
	file = _sys_fopen_a((uint8*)LogName);
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

uint8 _Truncate(char* fn, uint8 rc) {
	uint8 result = 0x00;
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)fn);
	if (truncate((char*)fullpath, rc * 128))
		result = 0xff;
	return(result);
}

void _MakeUserDir() {
	uint8 dFolder = cDrive + 'A';
	uint8 uFolder = toupper(tohex(userCode));

	uint8 path[4] = { dFolder, FOLDERCHAR, uFolder, 0 };
	uint8 fullpath[128] = FILEBASE;
	strcat((char*)fullpath, (char*)path);

	mkdir((char*)fullpath, S_IRUSR | S_IWUSR | S_IXUSR);
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
		if (mkdir((char*)fullpath1, S_IRUSR | S_IWUSR | S_IXUSR)) {
			result = 0xfe;
		} else {
			uint8 path[4] = { dFolder, FOLDERCHAR, '0', 0 };
			uint8 fullpath2[128] = FILEBASE;
			strcat((char*)fullpath2, (char*)path);
			mkdir((char*)fullpath2, S_IRUSR | S_IWUSR | S_IXUSR);
		}
	}

	return(result);
}

#ifndef POLLRDBAND
#define POLLRDBAND 0
#endif
#ifndef POLLRDNORM
#define POLLRDNORM 0
#endif

glob_t	pglob;
int	dirPos;

static char findNextDirName[128];
static uint16 fileRecords = 0;
static uint16 fileExtents = 0;
static uint16 fileExtentsUsed = 0;
static uint16 firstFreeAllocBlock;

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;
	char dir[6] = { '?', FOLDERCHAR, 0, FOLDERCHAR, '*', 0 };
	int i;
	struct stat st;
	uint32 bytes;

	if (allExtents && fileRecords) {
		// _SearchFirst was called with '?' in the FCB's EX field, so
		// we need to return all file extents.
		// The last found file was large enough that in CP/M it would
		// have another directory entry, so mock up the next entry
		// for the file.
		_mockupDirEntry(1);
		result = 0;
	} else {
		// Either we're only interested in the first directory entry
		// for each file, or the previously found file has had the
		// required number of dierctory entries returned already.
		dir[0] = filename[0];
		if (allUsers)
			dir[2] = '?';
		else
			dir[2] = filename[2];
		uint8 fullpath[128] = FILEBASE;
		strcat((char*)fullpath, (char*)dir);
		if (!glob((char*)fullpath, 0, NULL, &pglob)) {
			for (i = dirPos; i < pglob.gl_pathc; ++i) {
				++dirPos;
				strncpy(findNextDirName, pglob.gl_pathv[i], sizeof(findNextDirName) - 1);
				findNextDirName[sizeof(findNextDirName) - 1] = 0;
				char* shortName = &findNextDirName[strlen(FILEBASE)];
				_HostnameToFCBname((uint8*)shortName, fcbname);
				if (match(fcbname, pattern) &&
					(stat(findNextDirName, &st) == 0) &&
					((st.st_mode & S_IFREG) != 0) &&
					isxdigit((uint8)shortName[2]) &&
					(isupper((uint8)shortName[2]) || isdigit((uint8)shortName[2]))) {
					if (allUsers)
						currFindUser = isdigit((uint8)shortName[2]) ? shortName[2] - '0' : shortName[2] - 'A' + 10;
					if (isdir) {
						// account for host files that aren't multiples of the block size
						// by rounding their bytes up to the next multiple of blocks
						bytes = st.st_size;
						if (bytes & (BlkSZ - 1))
							bytes = (bytes & ~(BlkSZ - 1)) + BlkSZ;
						// calculate the number of 128 byte records and 16K
						// extents for this file. _mockupDirEntry will use
						// these values to populate the returned directory
						// entry, and decrement the # of records and extents
						// left to process in the file.
						fileRecords = bytes / BlkSZ;
						fileExtents = fileRecords / BlkEX + ((fileRecords & (BlkEX - 1)) ? 1 : 0);
						fileExtentsUsed = 0;
						firstFreeAllocBlock = firstBlockAfterDir;
						_mockupDirEntry(1);
					} else {
						fileRecords = 0;
						fileExtents = 0;
						fileExtentsUsed = 0;
						firstFreeAllocBlock = firstBlockAfterDir;
					}
					_RamWrite(tmpFCB, filename[0] - '@');
					_HostnameToFCB(tmpFCB, (uint8*)shortName);
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
	strcpy((char*)pattern, "???????????");
	fileRecords = 0;
	fileExtents = 0;
	fileExtentsUsed = 0;
	return(_findnextallusers(isdir));
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
		"with the\ngiven name. "
		"After input file's EOF, further console input\nwill be read "
		"from the keyboard.\n");
	fprintf(stderr,
		"  -o output_file: console output will be written to the file "
		"with the\ngiven name, in addition to the screen.\n");
	fprintf(stderr,
		"  -s: console input and output is connected directly to "
		"stdin and stdout.\nSince on Posix keyboard input is read from "
		"stdin, switching from\nstdin to keyboard on stdin EOF is a "
		"no-op. Therefore stdin EOF is an\nerror condition on Posix in "
		"this case.\n");
}

static void _fail_if_stdin_from_tty(char* argv[]) {
	struct termios dummyTermios;
	if (0 == tcgetattr(0, &dummyTermios) ||
	errno != ENOTTY) {
	_file_failure_exit(argv,
		"option -s is illegal when stdin comes from %s",
		"tty");
	}
}

static void _parse_options(int argc, char *argv[]) {
	int c;
	int errflg = 0;
	while ((c = getopt(argc, argv, ":i:o:s")) != -1) {
		switch(c) {
			case 'i':
				streamInputFile = fopen(optarg, "r");
				if (NULL == streamInputFile) {
					_file_failure_exit(argv,
						"error opening console input file %s", optarg);
				}
				streamInputActive = TRUE;
				break;
			case 'o':
				streamOutputFile = fopen(optarg, "w");
				if (NULL == streamOutputFile) {
					_file_failure_exit(argv,
						"error opening console output file %s", optarg);
				}
				break;
			case 's':
				_fail_if_stdin_from_tty(argv);
				streamInputFile = stdin;
				streamOutputFile = stdout;
				streamInputActive = TRUE;
				consoleOutputActive = FALSE;
				break;
			case ':':       /* -i or -o without operand */
				fprintf(stderr,
					"Option -%c requires an operand\n", optopt);
				errflg++;
				break;
			case '?':
				fprintf(stderr,
					"Unrecognized option: '-%c'\n", optopt);
				errflg++;
		}
	}
	if (errflg || optind != argc) {
		_usage(argv);
		exit(EXIT_FAILURE);
	}
}
#endif

#ifdef STREAMIO
void _host_init(int argc, char* argv[]) {
	_parse_options(argc, argv);
	if (chdir(dirname(argv[0]))) {
		_file_failure_exit(argv, "error performing chdir(%s)",
			dirname(argv[0]));
	}
}
#endif

/* Console abstraction functions */
/*===============================================================================*/

static struct termios _old_term, _new_term;

void _console_init(void) {
	tcgetattr(0, &_old_term);

	_new_term = _old_term;

	_new_term.c_lflag &= ~ICANON; /* Input available immediately (no EOL needed) */
	_new_term.c_lflag &= ~ECHO; /* Do not echo input characters */
	_new_term.c_lflag &= ~ISIG; /* ^C and ^Z do not generate signals */
	_new_term.c_iflag &= INLCR; /* Translate NL to CR on input */

	tcsetattr(0, TCSANOW, &_new_term); /* Apply changes immediately */

	setvbuf(stdin, (char*)NULL, _IONBF, 256); /* Enable stdin buffering */
	setvbuf(stdout, (char*)NULL, _IONBF, 0); /* Disable stdout buffering */
}

void _console_reset(void) {
	tcsetattr(0, TCSANOW, &_old_term);
}

#ifdef STREAMIO
extern void _streamioReset(void);

static void _abort_if_kbd_eof() {
	// On Posix, if !streamInputActive && streamInputFile == stdin,
	// this means EOF on stdin. Assuming that stdin is connected to a
	// file or pipe, further reading from stdin won't read from the
	// keyboard but just continue to yield EOF.
	// On Windows, this problem doesn't exist because of the separate
	// conio.h.
	if (!streamInputActive && streamInputFile == stdin) {
		_puts("\nEOF on console input from stdin\n");
		_console_reset();
		_streamioReset();
		exit(EXIT_FAILURE);
	}
}
#endif

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
