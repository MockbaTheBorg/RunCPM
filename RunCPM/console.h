#ifndef CONSOLE_H
#define CONSOLE_H

/* see main.c for definition */

uint8 mask8bit = 0x7f;		// TO be used for masking 8 bit characters (XMODEM related)
							// If set to 0x7f, RunCPM masks the 8th bit of characters sent
							// to the console. This is the standard CP/M behavior.
							// If set to 0xff, RunCPM passes 8 bit characters. This is
							// required for XMODEM to work.
							// Use the CONSOLE7 and CONSOLE8 programs to change this on the fly.

void _putcon(uint8 ch)		// Puts a character
{
#ifdef STREAMIO
	if (console_log != stdout) _putch(ch & mask8bit);
	if (console_log) fputc(ch & mask8bit, console_log);
#else
	_putch(ch & mask8bit);
#endif

}

void _puts(const char* str)	// Puts a \0 terminated string
{
	while (*str)
		_putcon(*(str++));
}

void _puthex8(uint8 c)		// Puts a HH hex string
{
	_putcon(tohex(c >> 4));
	_putcon(tohex(c & 0x0f));
}

void _puthex16(uint16 w)	// puts a HHHH hex string
{
	_puthex8(w >> 8);
	_puthex8(w & 0x00ff);
}

#ifdef STREAMIO
int nextConInChar;

void _getNextConInChar(void)
{
	nextConInChar = console_in ? fgetc(console_in) : EOF;
}

uint8 _conInCharReady(void)
{
	return EOF != nextConInChar;
}

uint8 _getConInChar(void)
{
	uint8 result = nextConInChar;
	_getNextConInChar();
	if (0x0a == result) result = 0x0d;
	return result;
}

uint8 _getConInCharEcho()
{
	uint8 result = _getConInChar();
	_putcon(result);
	return result;
}

void _streamioInit(void)
{
	_getNextConInChar();
}
#endif

uint8 _chready(void)		// Checks if there's a character ready for input
{
#ifdef STREAMIO
	if (_conInCharReady()) return 0xff;
#endif
	return(_kbhit() ? 0xff : 0x00);
}

uint8 _getconNB(void)	  // Gets a character, non-blocking, no echo
{
#ifdef STREAMIO
	if (_conInCharReady()) return _getConInChar();
#endif
	return(_kbhit() ? _getch() : 0x00);
}

uint8 _getcon(void)	   // Gets a character, blocking, no echo
{
#ifdef STREAMIO
	if (_conInCharReady()) return _getConInChar();
#endif
	return _getch();
}

uint8 _getconE(void)   // Gets a character, blocking, with echo
{
#ifdef STREAMIO
	if (_conInCharReady()) return _getConInCharEcho();
#endif
	return _getche();
}

#endif
