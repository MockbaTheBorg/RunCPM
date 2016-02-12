/* see main.c for definition */

uint8 _chready(void)
{
	return(_kbhit() ? 0xff : 0x00);
}

uint8 _getchNB(void)
{
	return(_kbhit() ? _getch() : 0x00);
}

void _putcon(uint8 ch)	// (todo) Do VT100 translation
{
	_putch(ch & 0x7f);
}

void _puts(const char* str)
{
	while (*str) {
		_putcon(*str);
		str++;
	}
}

void _puthex8(uint8 c)
{
	uint8 h;
	h = c >> 4;
	_putcon(h < 10 ? h + 48 : h + 87);
	h = c & 0x0f;
	_putcon(h < 10 ? h + 48 : h + 87);
}

void _puthex16(uint16 w)
{
	_puthex8(w >> 8);
	_puthex8(w & 0x00ff);
}