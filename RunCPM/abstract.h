#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <conio.h>
#include <dir.h>
#include <dirent.h>
#include "posix.h"

#define HostOS 0x03

struct ffblk fnd;

uint8 _findfirst(uint8 isdir) {
	uint8 result = 0xff;
	uint8 found;

	_HostnameToFCBname(filename, pattern);
	found = findfirst(filename, &fnd, 0);
	if (found == 0) {
		if (isdir) {
			_HostnameToFCB(dmaAddr, fnd.ff_name);
			_RamWrite(dmaAddr, 0);	// Sets the user of the requested file correctly on DIR entry
		}
		_RamWrite(tmpFCB, filename[0] - '@');
		_HostnameToFCB(tmpFCB, fnd.ff_name);
		result = 0x00;
	}
	return(result);
}

uint8 _findnext(uint8 isdir) {
	uint8 result = 0xff;
	uint8 more;

	_HostnameToFCBname((uint8*)fnd.ff_name, fcbname);
	more = findnext(&fnd);
	if (more == 0) {
		if (isdir) {
			_HostnameToFCB(dmaAddr, fnd.ff_name);
			_RamWrite(dmaAddr, 0);	// Sets the user of the requested file correctly on DIR entry
		}
		_RamWrite(tmpFCB, filename[0] - '@');
		_HostnameToFCB(tmpFCB, fnd.ff_name);
		result = 0x00;
	}
	return(result);
}

/* Console abstraction functions */
/*===============================================================================*/
void _console_init(void) {
}

void _console_reset(void) {

}

int _kbhit(void) {
	return kbhit();
}

unsigned char _getch(void) {
	return getch();
}

unsigned char _getche(void) {
	return getche();
}

void _putch(uint8 ch) {
	putch(ch);
}

void _clrscr(void) {
	system("cls");
}

#endif
