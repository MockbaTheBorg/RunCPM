#ifndef RAM_H
#define RAM_H

/* see main.c for definition */

#ifndef RAM_FAST
static uint8 RAM[MEMSIZE]; // Definition of the emulated RAM

uint8 *_RamSysAddr(uint16 address) {
    if (address < CCPaddr) {
        return (&RAM[curBankBase + address]);
    } else {
        return (&RAM[address]);
    }
}

uint8 _RamRead(uint16 address) {
    if (address < CCPaddr) {
        return (RAM[curBankBase + address]);
    } else {
        return (RAM[address]);
    }
}

uint16 _RamRead16(uint16 address) {
    if (address < CCPaddr) {
        uint32 bankAddress = curBankBase + address;

        return (RAM[bankAddress] + (RAM[bankAddress + 1] << 8));
    } else {
        return (RAM[address] + (RAM[address + 1] << 8));
    }
}

void _RamWrite(uint16 address, uint8 value) {
    if (address < CCPaddr) {
        RAM[curBankBase + address] = value;
    } else {
        RAM[address] = value;
    }
}

void _RamWrite16(uint16 address, uint16 value) {
    // Z80 is a "little indian" (8 bit era joke)
    _RamWrite(address, value & 0xff);
    _RamWrite(address + 1, (value >> 8) & 0xff);
}
#endif

#endif
