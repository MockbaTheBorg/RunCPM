#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <glob.h>

// Lua scripting support
#ifdef HASLUA
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "lua.h"
lua_State *L;
#endif

#include "posix.h"

#define HostOS 0x02

glob_t	pglob;
int	dirPos;

uint8 _findnext(uint8 isdir)
{
	uint8 result = 0xff;
#ifdef USER_SUPPORT
	char dir[6] = { '?', FOLDERCHAR, 0, FOLDERCHAR, '*', 0 };
#else
	char dir[4] = { '?', FOLDERCHAR, '*', 0 };
#endif
	char* dirname;
	int i;
	struct stat st;

	dir[0] = filename[0];
#ifdef USER_SUPPORT
	dir[2] = filename[2];
#endif
	if (!glob(dir, 0, NULL, &pglob)) {
		for (i = dirPos; i < pglob.gl_pathc; i++) {
			dirPos++;
			dirname = pglob.gl_pathv[i];
			_HostnameToFCBname((uint8*)dirname, fcbname);
			if (match(fcbname, pattern) && (stat(dirname, &st) == 0) && ((st.st_mode & S_IFREG) != 0)) {
				if (isdir) {
					_HostnameToFCB(dmaAddr, (uint8*)dirname);
					_RamWrite(dmaAddr, 0x00);
				}
				_RamWrite(tmpFCB, filename[0] - '@');
				_HostnameToFCB(tmpFCB, (uint8*)dirname);
				result = 0x00;
				break;
			}
		}
		globfree(&pglob);
	}

	return(result);
}

uint8 _findfirst(uint8 isdir) {
	dirPos = 0;	// Set directory search to start from the first position
	_HostnameToFCBname(filename, pattern);
	return(_findnext(isdir));
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

#endif