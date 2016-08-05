/* see main.c for definition */

void _RamFill(uint16 address, int size, uint8 value)
{
	while (size--)
	{
		_RamWrite(address++, value);
	}
}

void _RamCopy(uint16 source, int size, uint16 destination)
{
	while (size--)
	{
		_RamWrite(destination++, _RamRead(source++));
	}
}

#ifdef ARDUINO
bool _RamLoad(char* filename, uint16 address)
{
	File file = SD.open(filename, FILE_READ);
	bool result = true;

	if (file)
	{
		while (file.available())
		{
			_RamWrite(address++, file.read());
		}
		file.close();
	}
	else {
		result = false;
	}
	return(result);
}

bool _RamVerify(char* filename, uint16 address)
{
	File file = SD.open(filename, FILE_READ);
	byte c;
	bool result = true;

	if (file)
	{
		while (file.available())
		{
			c = file.read();
			if (c != _RamRead(address++))
			{
				result = false;
				break;
			}
		}
	}
	else {
		result = false;
	}
	return(result);
}
#else
void _RamLoad(FILE* file, uint16 address)
{
	long l;

	_fseek(file, 0, SEEK_END);
	l = _ftell(file);

	_fseek(file, 0, SEEK_SET);
	_fread(&RAM[address], 1, l, file); // (todo) This can overwrite past RAM space
}
#endif
