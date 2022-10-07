#ifndef RAM_H
#define RAM_H

/* see main.c for definition */

#ifndef RAM_FAST
static uint8 RAM[MEMSIZE];			// Definition of the emulated RAM

uint8* _RamSysAddr(uint16 address) {
	return(&RAM[address * curBank]);
}

uint8 _RamRead(uint16 address) {
	return(RAM[address * curBank]);
}

uint16 _RamRead16(uint16 address) {
	return(RAM[address * curBank] + (RAM[(address * curBank) + 1] << 8));
}

void _RamWrite(uint16 address, uint8 value) {
	RAM[address * curBank] = value;
}

void _RamWrite16(uint16 address, uint16 value) {
	// Z80 is a "little indian" (8 bit era joke)
	_RamWrite(address, value & 0xff);
	_RamWrite(address + 1, (value >> 8) & 0xff);
}
#endif

#endif
