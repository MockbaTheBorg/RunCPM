#ifndef CONSOLE_H
#define CONSOLE_H

/* see main.c for definition */

uint8 _chready(void)		// Checks if there's a character ready for input
{
	return(_kbhit() ? 0xff : 0x00);
}

uint8 _getchNB(void)		// Gets a character, non-blocking, no echo
{
	return(_kbhit() ? _getch() : 0x00);
}

void _putcon(uint8 ch)		// Puts a character
{
	_putch(ch & 0x7f);
}

void _puts(const char *str)	// Puts a \0 terminated string
{
	while (*str)
		_putcon(*(str++));
}

void _puthex8(uint8 c)		// Puts a HH hex string
{
	uint8 h;
	h = c >> 4;
	_putcon(tohex(h));
	h = c & 0x0f;
	_putcon(tohex(h));
}

void _puthex16(uint16 w)	// puts a HHHH hex string
{
	_puthex8(w >> 8);
	_puthex8(w & 0x00ff);
}

#endif