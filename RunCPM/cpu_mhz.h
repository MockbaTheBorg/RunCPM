#ifndef CPU_MHZ_H
#define CPU_MHZ_H

/* Helper function to get current time in milliseconds */
static uint32 get_time_ms(void) {
	// Return the current time in milliseconds for GCC compiler
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

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
uint32 Z80estimateClock(void) {
	const uint8 testCode[] = {
		0x11, 0xF4, 0x01, // LD DE, 500
		0x01, 0xE8, 0x03, // LD BC, 1000
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
	uint32 startTime, endTime;
	uint32 tstates = 0;
	
	// Load test code into RAM at address 0x0000
	for (uint16 addr = 0; addr < sizeof(testCode); addr++) {
		PUT_BYTE(addr, testCode[addr]);
	}
	
	// Reset CPU
	Z80reset();
	
	// Start timing
	startTime = get_time_ms();
	
	// Run until HALT
	while (Status == STATUS_RUNNING) {
		Z80run();
	}
	
	// End timing
	endTime = get_time_ms();
	
	// Calculate total T-states executed
	uint32 t_ld_de = z80_tstates_main[0x11];
	uint32 t_ld_bc = z80_tstates_main[0x01];
	uint32 t_dec = z80_tstates_main[0x0B];
	uint32 t_ld_a = z80_tstates_main[0x78];
	uint32 t_or = z80_tstates_main[0xB1];
	uint32 t_jr = z80_tstates_main[0x20];
	uint32 t_halt = z80_tstates_main[0x76];
	
	uint32 inner_iters = 1000;
	uint32 outer_iters = 500;
	
	uint32 inner_body = t_dec + t_ld_a + t_or + t_jr;
	uint32 inner_exit = t_dec + t_ld_a + t_or + 7; // JR not taken
	uint32 total_inner = t_ld_bc + (inner_iters - 1) * inner_body + inner_exit;
	
	uint32 outer_body = total_inner + t_dec + t_ld_a + t_or + t_jr;
	uint32 outer_exit = total_inner + t_dec + t_ld_a + t_or + 7; // JR not taken
	
	tstates = t_ld_de + (outer_iters - 1) * outer_body + outer_exit + t_halt;

	// Calculate elapsed time in milliseconds
	uint32 elapsedTime = endTime - startTime;
	
	// Estimate clock speed in Hz
	if (elapsedTime == 0) elapsedTime = 1; // Prevent division by zero
	uint32 estimatedHz = (tstates * 1000) / elapsedTime;
	
	// Convert to MHz
	uint32 estimatedMHz = estimatedHz / 1000000;
	return estimatedMHz;
}

/* Using the Z80estimateClock function, print the estimated clock speed */
void Z80printEstimatedClock(void) {
	uint32 clock = Z80estimateClock();
	printf("Estimated Z80 clock speed: %u MHz\n", clock);
}

#endif // CPU_MHZ_H