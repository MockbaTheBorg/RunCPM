/* see main.c for definition */

#include <ctype.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum {true, false} bool;

/* Memory abstraction functions */
/*===============================================================================*/
static uint8 ROM[ROMSIZE];	// ROM must go into code (for Arduino and other hardware)
static uint8 RAM[RAMSIZE];	// RAM must go into memory (SRAM or DRAM)

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
uint8	pattern[14];
uint8	fcbname[14];
uint8	filename[15];
uint8	newname[13];
uint8	drive[2] = {'A', '/'};
uint8	user = 0;	// Current CP/M user
uint16	dmaAddr = 0x0080;
uint16	roVector = 0;
uint16	loginVector = 0;
glob_t	pglob;
int		pglob_pos;

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
	*(filename++) = '/';

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

	filename += 2;
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

void dirToFCB(uint8* from, uint8* to)
{
	int i = 0;

	while (*from != 0 && *from != '.')
	{
		*to = toupper(*from);
		to++; from++; i++;
	}
	while (i < 10) {
		*to = ' ';
		to++;  i++;
	}
	if (*from == '.') {
		from++;
	}
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

bool match(uint8* fcbname, uint8* pattern)
{
	bool result = 1;
	uint8 i;

	for (i = 0; i < 14; i++)
	{
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

uint8 _findnext(void)
{
	uint8 result = 0xff;
	char* file;
	int i;
	struct stat st;

	for (i = pglob_pos; i < pglob.gl_pathc; i++) {
		pglob_pos++;
		file = pglob.gl_pathv[i];
		dirToFCB((uint8*)file, fcbname);
		if (match(fcbname, pattern) && (stat(file, &st) == 0) && ((st.st_mode & S_IFREG) != 0)) {
			_SetFile(dmaAddr, (uint8*)file);
			_RamWrite(dmaAddr, filename[0] - '@'); 	// Sets the drive of the requested file correctly on the FCB
			result = 0x00;
			break;
		}
	}
	if (result != 0x00) {
		globfree(&pglob);
	}
	return(result);
}

uint8 _findfirst(void) {
	uint8 result = 0xff;
	char dir[4] = "A/*";

	dir[0] = filename[0];
	if (glob(dir, 0, NULL, &pglob) == 0) {
		dirToFCB(filename, pattern);
		pglob_pos = 0;
		result = _findnext();
	}
	return(result);
}

uint8 _Truncate(char* fn, uint8 rc)
{
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

void _console_init(void)
{
	tcgetattr(0, &_old_term);

	_new_term = _old_term;

	_new_term.c_lflag &= ~ICANON; /* Input available immediately (no EOL needed) */
	_new_term.c_lflag &= ~ECHO; /* Do not echo input characters */
	_new_term.c_lflag &= ~ISIG; /* ^C and ^Z do not generate signals */
	_new_term.c_iflag &= INLCR; /* Translate NL to CR on input */

	tcsetattr(0, TCSANOW, &_new_term); /* Apply changes immediately */

	setvbuf(stdout, (char *)NULL, _IONBF, 0); /* Disable stdout buffering */
}

void _console_reset(void)
{
	tcsetattr(0, TCSANOW, &_old_term);
}

int _kbhit(void)
{
	struct pollfd pfds[1];

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;

	return poll(pfds, 1, 0);
}

uint8 _getch(void)
{
	return getchar();
}

void _putch(uint8 ch)
{
	putchar(ch);
}

uint8 _getche(void)
{
	uint8 ch = _getch();

	_putch(ch);

	return ch;
}

void _clrscr(void)
{
	system("clear");
}

