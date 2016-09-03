/* see main.c for definition */

#include <ctype.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum {
	true, false
} bool;

/* Externals for abstracted functions need to go here */
FILE* _sys_fopen_r(uint8 *filename);
int _sys_fseek(FILE *file, long delta, int origin);
long _sys_ftell(FILE *file);
long _sys_fread(void *buffer, long size, long count, FILE *file);
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
uint8	pattern[14];
uint8	fcbname[14];
uint8	filename[15];
uint8	newname[13];
uint8	drive[2] = { 'A', '/' };
uint8	user = 0;	// Current CP/M user
uint16	dmaAddr = 0x0080;
uint16	roVector = 0;
uint16	loginVector = 0;
glob_t	pglob;
int	dirPos;

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

bool _sys_exists(uint8 *filename) {
	return(TRUE);	// (todo) Make this work
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

int _sys_select(uint8 *disk) {
	struct stat st;
	return((stat((char*)disk, &st) == 0) && ((st.st_mode & S_IFDIR) != 0));
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
	uint8 result=_sys_remove(filename);
	printf("Deleting %s = result=%d\n", (char*)filename, result);
	if(!result)
		printf("Failed\n");
	return(!result);
}

int _sys_renamefile(uint8 *filename, uint8 *newname) {
	return(!_sys_rename(&filename[0], &newname[0]));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer) {
#ifdef CONSOLELOG
	_puts((char *)buffer);
#else
	uint8 s = 0;
	while (*(buffer + s))	// Computes buffer size
		s++;
	FILE *file = _sys_fopen_a(LogName);
	_sys_fwrite(buffer, 1, s, file);
	_sys_fclose(file);
}
#endif
#endif

uint8 _sys_readseq(uint8 *filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;

	FILE *file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z (EOF) prior to reading
			bytesread = (uint8)_sys_fread(_RamSysAddr(dmaAddr), 1, 128, file);
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
			if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file)) {
				result = 0x00;
			}
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

	FILE *file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			_RamFill(dmaAddr, 128, 0x1a);	// Fills the buffer with ^Z prior to reading
			bytesread = (uint8)_sys_fread(_RamSysAddr(dmaAddr), 1, 128, file);
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
			if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file)) {
				result = 0x00;
			}
		} else {
			result = 0x06;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _GetFile(uint16 fcbaddr, uint8 *filename) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 i = 0;
	uint8 unique = TRUE;
	
	if (F->dr) {
		*(filename++) = (F->dr - 1) + 'A';
	} else {
		*(filename++) = (_RamRead(0x0004) & 0x0f) + 'A';
	}
	*(filename++) = '/';

	while (i < 8) {
		if (F->fn[i] > 32)
			*(filename++) = F->fn[i];
		if (F->fn[i] == '?')
			unique = FALSE;
		i++;
	}
	*(filename++) = '.';
	i = 0;
	while (i < 3) {
		if (F->tp[i] > 32)
			*(filename++) = F->tp[i];
		if (F->tp[i] == '?')
			unique = FALSE;
		i++;
	}
	*filename = 0x00;

	return(unique);
}

void _SetFile(uint16 fcbaddr, uint8 *filename) {
	CPM_FCB *F = (CPM_FCB*)_RamSysAddr(fcbaddr);
	uint8 *dest = &F->fn[0];

	while (*filename)
		*dest++ = toupper(*filename++);
}

void nameToFCB(uint8 *from, uint8 *to) {
	int i = 0;

	from++;
	if (*from == '/') {	// Skips the drive and / if needed
		from++;
	} else {
		from--;
	}

	while (*from != 0 && *from != '.') {
		*to = toupper(*from);
		to++; from++; i++;
	}
	while (i < 10) {
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

bool match(uint8 *fcbname, uint8 *pattern) {
	bool result = 1;
	uint8 i;
	for (i = 0; i < 14; i++) {
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

uint8 findNext()
{
	bool result = FALSE;
	char dir[4] = "?/*";
	char* file;
	int i;
	struct stat st;

	dir[0] = filename[0];
	if (!glob(dir, 0, NULL, &pglob)) {
		for (i = dirPos; i < pglob.gl_pathc; i++) {
			dirPos++;
			file = pglob.gl_pathv[i];
			nameToFCB((uint8*)file, fcbname);
			if (match(fcbname, pattern) && (stat(file, &st) == 0) && ((st.st_mode & S_IFREG) != 0)) {
				result = TRUE;
				break;
			}
		}
		globfree(&pglob);
	}

	return(result);
}

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;

	if (findNext()) {	// Finds a file matching the pattern, starting from dirPos
		if (isdir) {
			_SetFile(dmaAddr, fcbname);
			_RamWrite(dmaAddr, 0x00);
		}
		_SetFile(tmpFCB, fcbname);	// DIR will print the name from tmpFCB
		result = 0x00;
	}

	return(result);
}

uint8 _findfirst(uint8 isdir) {
	dirPos = 0;	// Set directory search to start from the first position
	nameToFCB(filename, pattern);
	return(_findnext(isdir));
}

uint8 _Truncate(char *fn, uint8 rc) {
	uint8 result = 0x00;
	if (truncate(fn, rc * 128)) {
		result = 0xff;
	}
	return(result);
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
	pfds[0].events = POLLIN;

	return poll(pfds, 1, 0);
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
	system("clear");
}

