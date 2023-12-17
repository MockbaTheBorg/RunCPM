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
	if (consoleOutActive) _putch(ch & mask8bit);
	if (streamOutFile) fputc(ch & mask8bit, streamOutFile);
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
int _nextStreamInChar;

void _getNextStreamInChar(void)
{
	_nextStreamInChar = streamInFile ? fgetc(streamInFile) : EOF;
	if (EOF == _nextStreamInChar) {
		streamInActive = FALSE;
	}
}

uint8 _getStreamInChar(void)
{
	uint8 result = _nextStreamInChar;
	_getNextStreamInChar();
	// TODO: delegate to abstrction_posix.h
	if (0x0a == result) result = 0x0d;
	return result;
}

uint8 _getStreamInCharEcho()
{
	uint8 result = _getStreamInChar();
	_putcon(result);
	return result;
}

void _streamioInit(void)
{
	_getNextStreamInChar();
}

void _streamioReset(void)
{
	if (streamOutFile) fclose(streamOutFile);
}
#endif

uint8 _chready(void)		// Checks if there's a character ready for input
{
#ifdef STREAMIO
	if (streamInActive) return 0xff;
	// TODO: delegate to abstrction_posix.h function canReadFromKbd() or
	// something similar.
	// On Posix, if !streamInActive && streamInFile == stdin, this means
	// EOF on stdin. Assuming that stdin is connected to a file or pipe,
	// further reading from stdin won't read from the keyboard but just
	// continue to yield EOF.
	// On Windows, this problem doesn't exist because of the separete
	// conio.h.
	if (streamInFile == stdin) {
		_puts("EOF on console input from stdin\n");
		exit(EXIT_FAILURE);
	}
#endif
	return(_kbhit() ? 0xff : 0x00);
}

uint8 _getconNB(void)	  // Gets a character, non-blocking, no echo
{
#ifdef STREAMIO
	if (streamInActive) return _getStreamInChar();
	// TODO: Consider adding canReadFromKbd() here, too.
#endif
	return(_kbhit() ? _getch() : 0x00);
}

uint8 _getcon(void)	   // Gets a character, blocking, no echo
{
#ifdef STREAMIO
	if (streamInActive) return _getStreamInChar();
	// TODO: Consider adding canReadFromKbd() here, too.
#endif
	return _getch();
}

uint8 _getconE(void)   // Gets a character, blocking, with echo
{
#ifdef STREAMIO
	if (streamInActive) return _getStreamInCharEcho();
	// TODO: Consider adding canReadFromKbd() here, too.
#endif
	return _getche();
}

#endif
