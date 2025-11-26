#ifndef CPU_MHZ_H
#define CPU_MHZ_H

#include <stdint.h>

/* T-states for main Z80 instructions */
static const uint8 z80_tstates_main[256] = {
    4, 10, 7, 6, 4, 4, 7, 4, 4, 11, 7, 6, 4, 4, 7, 4,
    13, 10, 7, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4,
    12, 10, 16, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4,
    12, 10, 7, 6, 11, 11, 10, 4, 7, 7, 7, 7, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    4, 4, 4, 4, 4, 4, 7, 4, 5, 5, 5, 5, 5, 5, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 5, 5, 5, 5, 5, 5, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    5, 10, 10, 11, 10, 11, 7, 12, 10, 10, 10, 10, 7, 10, 7, 12,
    5, 10, 12, 11, 10, 11, 7, 12, 10, 12, 10, 10, 7, 10, 7, 12,
    5, 10, 12, 11, 10, 11, 7, 12, 10, 12, 10, 10, 7, 10, 7, 12,
    5, 10, 12, 11, 10, 11, 7, 12, 10, 12, 10, 10, 7, 10, 7, 11
};

/* Run a small Z80 code and measure the time to estimate emulated clock.
   This will load a small z80 code into RAM, run it until halt, then compute the 
   estimated clock at which the CPU is running */
void Z80estimateClock(void) {
	const uint8 testCode[] = {
#ifdef ARDUINO
		0x11, 0xF4, 0x01, // LD DE, 500
		0x01, 0xE8, 0x03, // LD BC, 1000
#else
		0x11, 0xE8, 0x03, // LD DE, 1000
		0x01, 0x10, 0x27, // LD BC, 10000
#endif
		0x0B,             // DEC BC
		0x78,             // LD A, B
		0xB1,             // OR C
		0x20, 0xFB,       // JR NZ, -5
		0x1B,             // DEC DE
		0x7A,             // LD A, D
		0xB3,             // OR E
		0x20, 0xF3,       // JR NZ, -13
		0x76              // HALT
	};

	uint64 time_start = 0;
	uint64 time_now = 0;
	
	// Load test code into RAM at address 0x0000
	for (uint16 addr = 0; addr < sizeof(testCode); addr++) {
		PUT_BYTE(addr, testCode[addr]);
	}
	
	// Reset CPU
	Z80reset();
	
	// Start timing
	time_start = millis();
	
	// Run until HALT
	while (Status == STATUS_RUNNING) {
		Z80run(0);
	}
	
	// End timing
	time_now = millis();
	
	// Calculate total T-states executed
	uint8 t_ld_de = z80_tstates_main[0x11];
	uint8 t_ld_bc = z80_tstates_main[0x01];
	uint8 t_dec = z80_tstates_main[0x0B];
	uint8 t_ld_a = z80_tstates_main[0x78];
	uint8 t_or = z80_tstates_main[0xB1];
	uint8 t_jr = z80_tstates_main[0x20];
	uint8 t_halt = z80_tstates_main[0x76];
	
#ifdef ARDUINO
	uint16 outer_iters = 500;
	uint16 inner_iters = 1000;
#else
	uint16 outer_iters = 1000;
	uint16 inner_iters = 10000;
#endif
	
	uint64 inner_body = t_dec + t_ld_a + t_or + t_jr;
	uint64 inner_exit = t_dec + t_ld_a + t_or + 7; // JR not taken
	uint64 total_inner = t_ld_bc + (inner_iters - 1) * inner_body + inner_exit;
	
	uint64 outer_body = total_inner + t_dec + t_ld_a + t_or + t_jr;
	uint64 outer_exit = total_inner + t_dec + t_ld_a + t_or + 7; // JR not taken
	uint64 total_outer = t_ld_de + (outer_iters - 1) * outer_body + outer_exit;

	// Final T-states count (full count, not in millions)
    uint64 tstates = total_outer + t_halt;

    // Calculate elapsed time in milliseconds
    uint64 elapsedTime = time_now - time_start;

    // Estimate clock speed in Hz
    if (elapsedTime == 0) elapsedTime = 1; // Prevent division by zero
    uint64 estimatedHz = (tstates * 1000) / elapsedTime;

    // Convert to MHz
    uint32 estimatedMHz = (uint32)(estimatedHz / 1000000);
    char buffer[64];
    sprintf(buffer, "%llu T-states in %llu ms\r\n", tstates, elapsedTime);
    _puts(buffer);
    sprintf(buffer, "Estimated Z80 clock speed: %u MHz\r\n", estimatedMHz);
    _puts(buffer);
	
	// Reset CPU
	Z80reset();
}

#endif // CPU_MHZ_H