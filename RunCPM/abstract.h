/* see main.c for definition */
#include <conio.h>
#include <ctype.h>
#include <dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
uint8	filename[15];
uint8	newname[13];
uint16	dmaAddr = 0x0080;
uint16	roVector = 0;
uint16	loginVector = 0;

uint8	user = 0;	// Current CP/M user

struct ffblk fnd;

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
	uint8 found;

	found = findfirst(filename, &fnd, 0);
	if (found == 0) {
		_SetFile(dmaAddr, fnd.ff_name);
		_RamWrite(dmaAddr, 0x00);
		result = 0x00;
	}
	return(result);
}

uint8 _findnext(void)
{
	uint8 result = 0xff;
	uint8 more;

	more = findnext(&fnd);
	if (more == 0) {
		_SetFile(dmaAddr, fnd.ff_name);
		_RamWrite(dmaAddr, 0x00);
		result = 0x00;
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
void _console_init(void)
{
}

void _console_reset(void)
{

}

int _kbhit(void)
{
	return kbhit();
}

unsigned char _getch(void)
{
	return getch();
}

unsigned char _getche(void)
{
	return getche();
}

void _putch(byte)
{
	putch(byte);
}

void _clrscr(void)
{
	system("cls");
}

