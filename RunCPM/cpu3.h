#ifndef CPU_H
#define CPU_H

/* Model 3 - Algorithmic Decoding, based on http://www.z80.info/decoding.htm */
/* 2 MHz on an Arduino Due */

#define CPU_IS "Model 3"

/* Register Definitions */
int32 PCX; /* external view of PC */
int32 AF;  /* AF register */
int32 BC;  /* BC register */
int32 DE;  /* DE register */
int32 HL;  /* HL register */
int32 IX;  /* IX register */
int32 IY;  /* IY register */
int32 PC;  /* program counter */
int32 SP;  /* SP register */
int32 AF1; /* alternate AF register */
int32 BC1; /* alternate BC register */
int32 DE1; /* alternate DE register */
int32 HL1; /* alternate HL register */
int32 IFF; /* Interrupt Flip Flop */
int32 IR;  /* Interrupt (upper) / Refresh (lower) register */
int32 Status = 0; /* Status of the CPU 0=running 1=end request 2=back to CCP */
int32 Debug = 0;
int32 Step = -1;

/* Helper Macros */
/* LOW_REGISTER, HIGH_REGISTER, SET_LOW_REGISTER, SET_HIGH_REGISTER are defined in globals.h */

#define FLAG_C  1
#define FLAG_N  2
#define FLAG_P  4
#define FLAG_3  8
#define FLAG_H  16
#define FLAG_5  32
#define FLAG_Z  64
#define FLAG_S  128

#define SETFLAG(f,c)    (AF = (c) ? AF | FLAG_ ## f : AF & ~FLAG_ ## f)
#define TSTFLAG(f)      ((AF & FLAG_ ## f) != 0)

// the following tables precompute some common subexpressions
static uint8 parityTable[256];
static uint16 rrcaTable[256];
static uint16 xororTable[256];
static uint8 rotateShiftTable[256];
static uint8 cbitsZ80Table[512];
static uint8 cbits2Z80Table[512];

void initTables(void) {
	// 256 bytes tables
	for (int i = 0; i < 256; i++) {
		char c = 0;
		for (int j = 0; j < 8; j++) {
			if (i & (1 << j))
				c++;
		}
		parityTable[i] = (c & 1) ? 0 : 4;
		rrcaTable[i] = ((i & 1) << 15) | ((i >> 1) << 8) | ((i >> 1) & 0x28) | (i & 1);
		xororTable[i] = (i << 8) | (i & 0xa8) | ((i == 0) << 6) | parityTable[i];
		rotateShiftTable[i] = (i & 0xa8) | (((i & 0xff) == 0) << 6) | parityTable[i & 0xff];
	}
	// 512 bytes tables
	for (int i = 0; i < 512; i++) {
		cbitsZ80Table[i] = (i & 0x10) | (((i >> 6) ^ (i >> 5)) & 4) | ((i >> 8) & 1);
		cbits2Z80Table[i] = (i & 0x10) | (((i >> 6) ^ (i >> 5)) & 4) | ((i >> 8) & 1) | 2;
	}
}

#define ADDRMASK        0xffff

/*  Macros for the IN/OUT instructions INI/INIR/IND/INDR/OUTI/OTIR/OUTD/OTDR */
#define INOUTFLAGS(syxz, x)                                             \
    AF = (AF & 0xff00) | (syxz) |               /* SF, YF, XF, ZF   */  \
        ((acu & 0x80) >> 6) |                           /* NF       */  \
        ((acu + (x)) > 0xff ? (FLAG_C | FLAG_H) : 0) |  /* CF, HF   */  \
        parityTable[((acu + (x)) & 7) ^ temp]           /* PF       */

#define INOUTFLAGS_ZERO(x)      INOUTFLAGS(FLAG_Z, x)
#define INOUTFLAGS_NONZERO(x)                                           \
    INOUTFLAGS((HIGH_REGISTER(BC) & 0xa8) | ((HIGH_REGISTER(BC) == 0) << 6), x)

/* Memory Access */
static uint8 GET_BYTE(uint16 a) {
    return _RamRead(a);
}

static void PUT_BYTE(uint16 a, uint8 v) {
    _RamWrite(a, v);
}

static uint16 GET_WORD(uint16 a) {
    return _RamRead(a) | (_RamRead(a + 1) << 8);
}

static void PUT_WORD(uint16 a, uint16 v) {
    _RamWrite(a, v & 0xff);
    _RamWrite(a + 1, (v >> 8) & 0xff);
}

#define RAM_PP(a)   GET_BYTE(a++)
#define RAM_MM(a)   GET_BYTE(a--)

#define PUT_BYTE_PP(a,v) PUT_BYTE(a++, v)
#define PUT_BYTE_MM(a,v) PUT_BYTE(a--, v)

#define PUSH(x) do {            \
    SP--;                       \
    PUT_BYTE(SP, (x) >> 8);     \
    SP--;                       \
    PUT_BYTE(SP, (x) & 0xff);   \
} while (0)

#define POP(x)  {               \
    uint16 l = GET_BYTE(SP);    \
    SP++;                       \
    uint16 h = GET_BYTE(SP);    \
    SP++;                       \
    x = (h << 8) | l;           \
}

/* CPU Interface */
void cpu_out(const uint32 p, const uint32 v) {
#ifdef INT_HANDOFF
    _HardwareOut(p, v);
#else
    if (p == 0xFF) {
        _Bios();
    } else {
        _HardwareOut(p, v);
    }
#endif
}

uint32 cpu_in(const uint32 p) {
    uint32 v;
#ifdef INT_HANDOFF
    v = _HardwareIn(p);
#else
    if (p == 0xFF) {
        _Bdos();
        v = HIGH_REGISTER(AF);
    } else {
        v = _HardwareIn(p);
    }
#endif
    return(v);
}

static inline void Z80reset(void) {
    PC = 0;
    IFF = 0;
    IR = 0;
    Status = 0;
    initTables();
}

/* Decoding Helper Variables */
/* r table: 0=B, 1=C, 2=D, 3=E, 4=H, 5=L, 6=(HL), 7=A */
static uint8 get_reg8(uint8 y) {
    switch (y) {
        case 0: return HIGH_REGISTER(BC);
        case 1: return LOW_REGISTER(BC);
        case 2: return HIGH_REGISTER(DE);
        case 3: return LOW_REGISTER(DE);
        case 4: return HIGH_REGISTER(HL);
        case 5: return LOW_REGISTER(HL);
        case 6: return GET_BYTE(HL);
        case 7: return HIGH_REGISTER(AF);
    }
    return 0;
}

static void set_reg8(uint8 y, uint8 v) {
    switch (y) {
        case 0: SET_HIGH_REGISTER(BC, v); break;
        case 1: SET_LOW_REGISTER(BC, v); break;
        case 2: SET_HIGH_REGISTER(DE, v); break;
        case 3: SET_LOW_REGISTER(DE, v); break;
        case 4: SET_HIGH_REGISTER(HL, v); break;
        case 5: SET_LOW_REGISTER(HL, v); break;
        case 6: PUT_BYTE(HL, v); break;
        case 7: SET_HIGH_REGISTER(AF, v); break;
    }
}

/* Flag Calculation Helpers */
static void alu(uint8 op, uint8 val) {
    uint8 a = HIGH_REGISTER(AF);
    uint16 sum;
    uint8 f = LOW_REGISTER(AF);
    uint8 c = (f & FLAG_C) ? 1 : 0;
    
    switch (op) {
        case 0: /* ADD A, val */
            sum = a + val;
            AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[a ^ val ^ sum];
            break;
        case 1: /* ADC A, val */
            sum = a + val + c;
            AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[a ^ val ^ sum];
            break;
        case 2: /* SUB val */
            sum = a - val;
            AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(a ^ val ^ sum) & 0x1ff];
            break;
        case 3: /* SBC A, val */
            sum = a - val - c;
            AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(a ^ val ^ sum) & 0x1ff];
            break;
        case 4: /* AND val */
            AF = (xororTable[(a & val) & 0xff] | 0x10);
            break;
        case 5: /* XOR val */
            AF = xororTable[(a ^ val) & 0xff];
            break;
        case 6: /* OR val */
            AF = xororTable[(a | val) & 0xff];
            break;
        case 7: /* CP val */
            sum = a - val;
            AF = (AF & ~0xff) | (sum & 0x80) | ((sum & 0xff) ? 0 : FLAG_Z) | (val & 0x28) | cbits2Z80Table[(a ^ val ^ sum) & 0x1ff];
            break;
    }
}

#if defined(DEBUG) || defined(iDEBUG)
#include "debug.h"
#endif

static inline void Z80run(uint32 cpu_delay) {
    uint8 opcode;
    uint8 x, y, z, p, q;
    int mode = 0; // 0=HL, 1=IX, 2=IY
    int8 d = 0;
    uint32 ea = 0; // Effective Address for (IX+d)
    uint8 val;

    static uint32 instr_cnt = 0;
    static uint32 last_millis = 0;

    if (last_millis == 0) last_millis = millis();

    while (!Status) {

        /* Throttling to CPU_DELAY instructions */
		if (cpu_delay != 0) {
	        if (++instr_cnt >= cpu_delay) {
    	        uint32 now = millis();
        	    if ((now - last_millis) < 10) {
            	    uint32 delay_ms = 10 - (now - last_millis);
#ifdef _WIN32
                	Sleep(delay_ms);
#elif defined(ARDUINO)
            	    delay(delay_ms);
#else
                	usleep(delay_ms * 1000);
#endif
            	}
            	last_millis = millis();
            	instr_cnt = 0;
        	}
		}

#ifdef DEBUG
		if (z80_check_breakpoints_on_exec(PC)) {
			Debug = 1;
		}
		if (PC == Step) {
			Debug = 1;
			Step = -1;
		}
		if (Debug)
			Z80debug();
		if (Status)
			break;
#endif

	PCX = PC;

	/* push instruction into trace (before it is executed) */
#if defined(DEBUG) || defined(iDEBUG)
	z80_trace_push(PCX);
#endif

        opcode = RAM_PP(PC);
        // Increment R
        IR = (IR & 0xff00) | ((IR + 1) & 0x7f) | (IR & 0x80);
        
        mode = 0;
        
        // Handle Prefixes
        if (opcode == 0xDD) {
            mode = 1;
            opcode = RAM_PP(PC);
            IR = (IR & 0xff00) | ((IR + 1) & 0x7f) | (IR & 0x80);
        } else if (opcode == 0xFD) {
            mode = 2;
            opcode = RAM_PP(PC);
            IR = (IR & 0xff00) | ((IR + 1) & 0x7f) | (IR & 0x80);
        }
        
        x = (opcode >> 6) & 3;
        y = (opcode >> 3) & 7;
        z = opcode & 7;
        p = y >> 1;
        q = y & 1;

        if (x == 0) {
            switch (z) {
                case 0: // Relative jumps and misc
                    if (y == 0) { /* NOP */ }
                    else if (y == 1) { /* EX AF, AF' */ 
                        int32 tmp = AF; AF = AF1; AF1 = tmp; 
                    }
                    else if (y == 2) { /* DJNZ d */ 
                        d = (int8)RAM_PP(PC);
                        uint8 b = HIGH_REGISTER(BC) - 1;
                        SET_HIGH_REGISTER(BC, b);
                        if (b != 0) PC += d;
                    }
                    else if (y == 3) { /* JR d */
                        d = (int8)RAM_PP(PC);
                        PC += d;
                    }
                    else if (y >= 4) { /* JR cc, d */
                        d = (int8)RAM_PP(PC);
                        int cc = y - 4;
                        int cond = 0;
                        switch (cc) {
                            case 0: cond = !TSTFLAG(Z); break; // NZ
                            case 1: cond = TSTFLAG(Z); break;  // Z
                            case 2: cond = !TSTFLAG(C); break; // NC
                            case 3: cond = TSTFLAG(C); break;  // C
                        }
                        if (cond) PC += d;
                    }
                    break;
                case 1: // 16-bit load immediate / add
                    if (q == 0) { /* LD rp[p], nn */
                        uint16 nn = GET_WORD(PC);
                        PC += 2;
                        if (mode == 0) {
                            if (p == 0) BC = nn;
                            else if (p == 1) DE = nn;
                            else if (p == 2) HL = nn;
                            else SP = nn;
                        } else {
                            if (p == 2) { if (mode == 1) IX = nn; else IY = nn; }
                            else if (p == 0) BC = nn;
                            else if (p == 1) DE = nn;
                            else SP = nn;
                        }
                    } else { /* ADD HL, rp[p] */
                        uint32 val;
                        uint32 base;
                        if (mode == 0) base = HL; else base = (mode == 1) ? IX : IY;
                        
                        if (p == 0) val = BC;
                        else if (p == 1) val = DE;
                        else if (p == 2) val = (mode == 0) ? HL : ((mode == 1) ? IX : IY);
                        else val = SP;
                        
                        base &= 0xffff;
                        val &= 0xffff;
                        uint32 res = base + val;
                        
                        AF = (AF & ~(FLAG_N | FLAG_C | FLAG_H | FLAG_3 | FLAG_5)) |
                             ((res >> 8) & (FLAG_3 | FLAG_5)) |
                             (((base ^ val ^ res) >> 8) & FLAG_H) |
                             ((res >> 16) & FLAG_C);
                        
                        if (mode == 0) HL = res & 0xffff;
                        else if (mode == 1) IX = res & 0xffff;
                        else IY = res & 0xffff;
                    }
                    break;
                case 2: // Indirect loading
                    if (q == 0) {
                        if (p == 0) PUT_BYTE(BC, HIGH_REGISTER(AF)); // LD (BC), A
                        else if (p == 1) PUT_BYTE(DE, HIGH_REGISTER(AF)); // LD (DE), A
                        else if (p == 2) { // LD (nn), HL
                            uint16 nn = GET_WORD(PC); PC += 2;
                            PUT_WORD(nn, (mode == 0) ? HL : ((mode == 1) ? IX : IY));
                        }
                        else { // LD (nn), A
                            uint16 nn = GET_WORD(PC); PC += 2;
                            PUT_BYTE(nn, HIGH_REGISTER(AF));
                        }
                    } else {
                        if (p == 0) SET_HIGH_REGISTER(AF, GET_BYTE(BC)); // LD A, (BC)
                        else if (p == 1) SET_HIGH_REGISTER(AF, GET_BYTE(DE)); // LD A, (DE)
                        else if (p == 2) { // LD HL, (nn)
                            uint16 nn = GET_WORD(PC); PC += 2;
                            uint16 v = GET_WORD(nn);
                            if (mode == 0) HL = v; else if (mode == 1) IX = v; else IY = v;
                        }
                        else { // LD A, (nn)
                            uint16 nn = GET_WORD(PC); PC += 2;
                            SET_HIGH_REGISTER(AF, GET_BYTE(nn));
                        }
                    }
                    break;
                case 3: // INC/DEC rp
                    {
                        int32 *rp_ptr;
                        if (p == 0) rp_ptr = &BC;
                        else if (p == 1) rp_ptr = &DE;
                        else if (p == 2) rp_ptr = (mode == 0) ? &HL : ((mode == 1) ? &IX : &IY);
                        else rp_ptr = &SP;
                        
                        if (q == 0) (*rp_ptr)++; else (*rp_ptr)--;
                        *rp_ptr &= 0xffff;
                    }
                    break;
                case 4: // INC r[y]
                    {
                        uint8 v;
                        if (y == 6) {
                            if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); }
                            else { ea = HL; v = GET_BYTE(ea); }
                        } else {
                            if (mode != 0 && (y == 4 || y == 5)) { // IXH/IXL
                                int32 val = (mode == 1) ? IX : IY;
                                if (y == 4) v = (val >> 8) & 0xff; else v = val & 0xff;
                            } else v = get_reg8(y);
                        }
                        
                        uint8 res = v + 1;
                        AF = (AF & ~0xfe) | (res & 0xa8) | ((res == 0) ? FLAG_Z : 0) | ((res & 0x0f) ? 0 : FLAG_H) | ((res == 0x80) ? FLAG_P : 0);
                        
                        if (y == 6) PUT_BYTE(ea, res);
                        else if (mode != 0 && (y == 4 || y == 5)) {
                             int32 *rp_ptr = (mode == 1) ? &IX : &IY;
                             if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (res << 8);
                             else *rp_ptr = (*rp_ptr & 0xff00) | res;
                        } else set_reg8(y, res);
                    }
                    break;
                case 5: // DEC r[y]
                    {
                        uint8 v;
                        if (y == 6) {
                            if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); }
                            else { ea = HL; v = GET_BYTE(ea); }
                        } else {
                            if (mode != 0 && (y == 4 || y == 5)) {
                                int32 val = (mode == 1) ? IX : IY;
                                if (y == 4) v = (val >> 8) & 0xff; else v = val & 0xff;
                            } else v = get_reg8(y);
                        }
                        
                        uint8 res = v - 1;
                        AF = (AF & ~0xfe) | (res & 0xa8) | ((res == 0) ? FLAG_Z : 0) | ((res & 0x0f) == 0x0f ? FLAG_H : 0) | ((res == 0x7f) ? FLAG_P : 0) | FLAG_N;
                        
                        if (y == 6) PUT_BYTE(ea, res);
                        else if (mode != 0 && (y == 4 || y == 5)) {
                             int32 *rp_ptr = (mode == 1) ? &IX : &IY;
                             if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (res << 8);
                             else *rp_ptr = (*rp_ptr & 0xff00) | res;
                        } else set_reg8(y, res);
                    }
                    break;
                case 6: // LD r[y], n
                    {
                        if (mode != 0 && y == 6) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; }
                        uint8 n = RAM_PP(PC);
                        if (y == 6) {
                            if (mode == 0) PUT_BYTE(HL, n);
                            else PUT_BYTE(ea, n);
                        } else if (mode != 0 && (y == 4 || y == 5)) {
                             int32 *rp_ptr = (mode == 1) ? &IX : &IY;
                             if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (n << 8);
                             else *rp_ptr = (*rp_ptr & 0xff00) | n;
                        } else set_reg8(y, n);
                    }
                    break;
                case 7: // RLCA, RRCA, etc.
                    {
                        uint8 a = HIGH_REGISTER(AF);
                        uint8 f = LOW_REGISTER(AF);
                        uint8 c = (f & FLAG_C) ? 1 : 0;
                        uint32 temp, cbits, acu;
                        switch (y) {
                            case 0: // RLCA
                                AF = ((AF >> 7) & 0x0128) | ((AF << 1) & ~0x1ff) | (AF & 0xc4) | ((AF >> 15) & 1);
                                break;
                            case 1: // RRCA
                                AF = (AF & 0xc4) | rrcaTable[HIGH_REGISTER(AF)];
                                break;
                            case 2: // RLA
                                AF = ((AF << 8) & 0x0100) | ((AF >> 7) & 0x28) | ((AF << 1) & ~0x01ff) | (AF & 0xc4) | ((AF >> 15) & 1);
                                break;
                            case 3: // RRA
                                AF = ((AF & 1) << 15) | (AF & 0xc4) | (rrcaTable[HIGH_REGISTER(AF)] & 0x7fff);
                                break;
                            case 4: // DAA
                                acu = HIGH_REGISTER(AF);
                                temp = LOW_DIGIT(acu);
                                cbits = TSTFLAG(C);
                                if (TSTFLAG(N)) {   /* last operation was a subtract */
                                    int hd = cbits || acu > 0x99;
                                    if (TSTFLAG(H) || (temp > 9)) { /* adjust low digit */
                                        if (temp > 5)
                                            SETFLAG(H, 0);
                                        acu -= 6;
                                        acu &= 0xff;
                                    }
                                    if (hd)
                                        acu -= 0x160;   /* adjust high digit */
                                } else {          /* last operation was an add */
                                    if (TSTFLAG(H) || (temp > 9)) { /* adjust low digit */
                                        SETFLAG(H, (temp > 9));
                                        acu += 6;
                                    }
                                    if (cbits || ((acu & 0x1f0) > 0x90))
                                        acu += 0x60;   /* adjust high digit */
                                }
                                AF = (AF & 0x12) | xororTable[acu & 0xff] | ((acu >> 8) & 1) | cbits;
                                break;
                            case 5: // CPL
                                AF = (~AF & ~0xff) | (AF & 0xc5) | ((~AF >> 8) & 0x28) | 0x12;
                                break;
                            case 6: // SCF
                                AF = (AF & ~0x3b) | ((AF >> 8) & 0x28) | 1;
                                break;
                            case 7: // CCF
                                AF = (AF & ~0x3b) | ((AF >> 8) & 0x28) | ((AF & 1) << 4) | (~AF & 1);
                                break;
                        }
                    }
                    break;
            }
        } else if (x == 1) { // LD r[y], r[z]
            if (y == 6 && z == 6) { // HALT
#ifdef DEBUG
			_puts("\r\n::CPU HALTED::\r\n");	// A halt is a good indicator of broken code
			_puts("Press any key...");
			_getcon();
	#ifdef DEBUGONHALT
			_puts("\r\n");
			Debug = 1;
			Z80debug();
	#endif
#endif
                PC--;
                Status = STATUS_EXIT;
            } else {
                uint8 v;
                int use_ix_d = (z == 6 || y == 6); // If (IX+d) is involved, H/L are not remapped
                
                if (z == 6) {
                    if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); }
                    else v = GET_BYTE(HL);
                } else if (mode != 0 && !use_ix_d && (z == 4 || z == 5)) {
                    int32 val = (mode == 1) ? IX : IY;
                    if (z == 4) v = (val >> 8) & 0xff; else v = val & 0xff;
                } else v = get_reg8(z);
                
                if (y == 6) {
                    if (mode != 0) { 
                        // d is already fetched if z=6? No, z!=6 here because y=6.
                        // If z=6 and y=6, it's HALT.
                        // So if y=6, z!=6.
                        d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d;
                        PUT_BYTE(ea, v);
                    } else PUT_BYTE(HL, v);
                } else if (mode != 0 && !use_ix_d && (y == 4 || y == 5)) {
                     int32 *rp_ptr = (mode == 1) ? &IX : &IY;
                     if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (v << 8);
                     else *rp_ptr = (*rp_ptr & 0xff00) | v;
                } else set_reg8(y, v);
            }
        } else if (x == 2) { // ALU[y] r[z]
            uint8 v;
            if (z == 6) {
                if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); }
                else v = GET_BYTE(HL);
            } else if (mode != 0 && (z == 4 || z == 5)) {
                int32 val = (mode == 1) ? IX : IY;
                if (z == 4) v = (val >> 8) & 0xff; else v = val & 0xff;
            } else v = get_reg8(z);
            alu(y, v);
        } else { // x == 3
            switch (z) {
                case 0: // RET cc
                    {
                        int cc = y;
                        int cond = 0;
                        switch (cc) {
                            case 0: cond = !TSTFLAG(Z); break;
                            case 1: cond = TSTFLAG(Z); break;
                            case 2: cond = !TSTFLAG(C); break;
                            case 3: cond = TSTFLAG(C); break;
                            case 4: cond = !TSTFLAG(P); break;
                            case 5: cond = TSTFLAG(P); break;
                            case 6: cond = !TSTFLAG(S); break;
                            case 7: cond = TSTFLAG(S); break;
                        }
                        if (cond) {
                            uint16 ret; POP(ret); PC = ret;
                        }
                    }
                    break;
                case 1: // POP / RET / EXX / JP / LD SP, HL
                    if (q == 0) { // POP rp2[p]
                        uint16 v; POP(v);
                        if (p == 0) BC = v;
                        else if (p == 1) DE = v;
                        else if (p == 2) { if (mode == 0) HL = v; else if (mode == 1) IX = v; else IY = v; }
                        else AF = v;
                    } else {
                        if (p == 0) { uint16 ret; POP(ret); PC = ret; } // RET
                        else if (p == 1) { // EXX
                            int32 tmp;
                            tmp = BC; BC = BC1; BC1 = tmp;
                            tmp = DE; DE = DE1; DE1 = tmp;
                            tmp = HL; HL = HL1; HL1 = tmp;
                        }
                        else if (p == 2) { // JP (HL) / (IX) / (IY)
                            if (mode == 0) PC = HL; else if (mode == 1) PC = IX; else PC = IY;
                        }
                        else { // LD SP, HL/IX/IY
                            if (mode == 0) SP = HL; else if (mode == 1) SP = IX; else SP = IY;
                        }
                    }
                    break;
                case 2: // JP cc, nn
                    {
                        uint16 nn = GET_WORD(PC); PC += 2;
                        int cc = y;
                        int cond = 0;
                        switch (cc) {
                            case 0: cond = !TSTFLAG(Z); break;
                            case 1: cond = TSTFLAG(Z); break;
                            case 2: cond = !TSTFLAG(C); break;
                            case 3: cond = TSTFLAG(C); break;
                            case 4: cond = !TSTFLAG(P); break;
                            case 5: cond = TSTFLAG(P); break;
                            case 6: cond = !TSTFLAG(S); break;
                            case 7: cond = TSTFLAG(S); break;
                        }
                        if (cond) PC = nn;
                    }
                    break;
                case 3: // Jumps / OUT / IN / EX / DI / EI
                    if (y == 0) { // JP nn
                        uint16 nn = GET_WORD(PC); PC = nn;
                    } else if (y == 1) { // CB Prefix
                        if (mode != 0) {
                            d = (int8)RAM_PP(PC);
                            opcode = RAM_PP(PC);
                        } else {
                            opcode = RAM_PP(PC);
                        }
                        
                        int cb_x = (opcode >> 6) & 3;
                        int cb_y = (opcode >> 3) & 7;
                        int cb_z = opcode & 7;
                        
                        uint8 val;
                        uint32 addr = 0;
                        
                        if (mode != 0) {
                            addr = ((mode == 1) ? IX : IY) + d;
                            val = GET_BYTE(addr);
                        } else {
                            val = get_reg8(cb_z);
                        }
                        
                        uint8 res = val;
                        
                        if (cb_x == 0) { // Rotate/Shift
                            uint8 cbits = 0;
                            switch (cb_y) {
                                case 0: // RLC
                                    res = (val << 1) | (val >> 7); cbits = res & 1; break;
                                case 1: // RRC
                                    res = (val >> 1) | (val << 7); cbits = res & 0x80; break;
                                case 2: // RL
                                    res = (val << 1) | TSTFLAG(C); cbits = val & 0x80; break;
                                case 3: // RR
                                    res = (val >> 1) | (TSTFLAG(C) << 7); cbits = val & 1; break;
                                case 4: // SLA
                                    res = val << 1; cbits = val & 0x80; break;
                                case 5: // SRA
                                    res = (val >> 1) | (val & 0x80); cbits = val & 1; break;
                                case 6: // SLL
                                    res = (val << 1) | 1; cbits = val & 0x80; break;
                                case 7: // SRL
                                    res = val >> 1; cbits = val & 1; break;
                            }
                            AF = (AF & ~0xff) | rotateShiftTable[res] | !!cbits;
                            
                            if (mode != 0) PUT_BYTE(addr, res);
                            else set_reg8(cb_z, res);
                        } else if (cb_x == 1) { // BIT
                            if (val & (1 << cb_y))
                                AF = (AF & ~0xfe) | 0x10 | ((cb_y == 7) << 7);
                            else
                                AF = (AF & ~0xfe) | 0x54;
                            if (cb_z != 6 && mode == 0) // Not (HL)
                                AF |= (val & 0x28);
                        } else if (cb_x == 2) { // RES
                            res &= ~(1 << cb_y);
                            if (mode != 0) PUT_BYTE(addr, res);
                            else set_reg8(cb_z, res);
                        } else { // SET
                            res |= (1 << cb_y);
                            if (mode != 0) PUT_BYTE(addr, res);
                            else set_reg8(cb_z, res);
                        }
                    } else if (y == 2) { // OUT (n), A
                        uint8 n = RAM_PP(PC); cpu_out(n, HIGH_REGISTER(AF));
                    } else if (y == 3) { // IN A, (n)
                        uint8 n = RAM_PP(PC); SET_HIGH_REGISTER(AF, cpu_in(n));
                    } else if (y == 4) { // EX (SP), HL
                        uint16 v; POP(v);
                        uint16 old;
                        if (mode == 0) { old = HL; HL = v; }
                        else if (mode == 1) { old = IX; IX = v; }
                        else { old = IY; IY = v; }
                        PUSH(old);
                    } else if (y == 5) { // EX DE, HL
                        int32 tmp = DE; DE = HL; HL = tmp;
                    } else if (y == 6) { // DI
                        IFF = 0;
                    } else if (y == 7) { // EI
                        IFF = 1;
                    }
                    break;
                case 4: // CALL cc, nn
                    {
                        uint16 nn = GET_WORD(PC); PC += 2;
                        int cc = y;
                        int cond = 0;
                        switch (cc) {
                            case 0: cond = !TSTFLAG(Z); break;
                            case 1: cond = TSTFLAG(Z); break;
                            case 2: cond = !TSTFLAG(C); break;
                            case 3: cond = TSTFLAG(C); break;
                            case 4: cond = !TSTFLAG(P); break;
                            case 5: cond = TSTFLAG(P); break;
                            case 6: cond = !TSTFLAG(S); break;
                            case 7: cond = TSTFLAG(S); break;
                        }
                        if (cond) {
                            PUSH(PC); PC = nn;
                        }
                    }
                    break;
                case 5: // PUSH / CALL / Prefixes
                    if (q == 0) { // PUSH rp2[p]
                        if (p == 0) PUSH(BC);
                        else if (p == 1) PUSH(DE);
                        else if (p == 2) PUSH((mode == 0) ? HL : ((mode == 1) ? IX : IY));
                        else PUSH(AF);
                    } else {
                        if (p == 0) { // CALL nn
                            uint16 nn = GET_WORD(PC); PC += 2;
                            PUSH(PC); PC = nn;
                        } else if (p == 1) { // DD Prefix
                            // Handled at start?
                            // If we see DD here, it means we missed it?
                            // No, DD is 0xDD. x=3, y=3, z=5?
                            // 0xDD = 11011101. x=3, y=3, z=5.
                            // Wait. 0xDD: 11 011 101.
                            // x=3. y=3. z=5.
                            // My table says y=3 (q=1, p=1) is DD.
                            // So yes.
                            // But I handled DD at the top of the loop.
                            // So this case should not be reached if I handled it?
                            // Unless multiple prefixes? DD DD ...
                            // If I handled it, I fetched next opcode.
                            // So we shouldn't see DD here unless it's a second DD.
                            // If second DD, we just ignore the first one (or treat as NOP?).
                            // Z80 ignores repeated prefixes usually.
                        } else if (p == 2) { // ED Prefix
                            opcode = RAM_PP(PC);
                            int ed_x = (opcode >> 6) & 3;
                            int ed_y = (opcode >> 3) & 7;
                            int ed_z = opcode & 7;
                            int ed_p = ed_y >> 1;
                            int ed_q = ed_y & 1;
                            
                            if (ed_x == 1) {
                                switch (ed_z) {
                                    case 0: // IN r[y], (C)
                                        {
                                            uint8 v = cpu_in(LOW_REGISTER(BC));
                                            if (ed_y != 6) set_reg8(ed_y, v);
                                            // IN (C) (ed_y == 6) updates flags too, but doesn't store result
                                            AF = (AF & ~0xfe) | rotateShiftTable[v];
                                        }
                                        break;
                                    case 1: // OUT (C), r[y]
                                        if (ed_y != 6) cpu_out(LOW_REGISTER(BC), get_reg8(ed_y));
                                        else cpu_out(LOW_REGISTER(BC), 0);
                                        break;
                                    case 2: // SBC HL, rp / ADC HL, rp
                                        {
                                            uint32 val;
                                            if (ed_p == 0) val = BC; else if (ed_p == 1) val = DE; else if (ed_p == 2) val = HL; else val = SP;
                                            uint32 sum;
                                            HL &= ADDRMASK;
                                            val &= ADDRMASK;
                                            if (ed_q == 0) { // SBC HL, rp
                                                sum = HL - val - TSTFLAG(C);
                                                AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
                                                    cbits2Z80Table[((HL ^ val ^ sum) >> 8) & 0x1ff];
                                                HL = sum;
                                            } else { // ADC HL, rp
                                                sum = HL + val + TSTFLAG(C);
                                                AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
                                                    cbitsZ80Table[(HL ^ val ^ sum) >> 8];
                                                HL = sum;
                                            }
                                        }
                                        break;
                                    case 3: // LD (nn), rp / LD rp, (nn)
                                        {
                                            uint16 nn = GET_WORD(PC); PC += 2;
                                            if (ed_q == 0) { // LD (nn), rp
                                                if (ed_p == 0) PUT_WORD(nn, BC);
                                                else if (ed_p == 1) PUT_WORD(nn, DE);
                                                else if (ed_p == 2) PUT_WORD(nn, HL);
                                                else PUT_WORD(nn, SP);
                                            } else { // LD rp, (nn)
                                                uint16 v = GET_WORD(nn);
                                                if (ed_p == 0) BC = v;
                                                else if (ed_p == 1) DE = v;
                                                else if (ed_p == 2) HL = v;
                                                else SP = v;
                                            }
                                        }
                                        break;
                                    case 4: // NEG
                                        {
                                            uint8 a = HIGH_REGISTER(AF);
                                            uint8 temp = 0 - a;
                                            AF = (AF & ~0xff) | ((a & 0x0f) ? FLAG_H : 0) | ((a == 0x80) ? FLAG_P : 0) | FLAG_N | (a ? FLAG_C : 0) | (temp & 0xa8) |
                                                ((temp == 0) << 6) | ((a ^ temp) & 0x10);
                                            SET_HIGH_REGISTER(AF, temp);
                                        }
                                        break;
                                    case 5: // RETN / RETI
                                        { uint16 ret; POP(ret); PC = ret; IFF |= IFF >> 1; }
                                        break;
                                    case 6: // IM 0/1/2
                                        // Interrupt modes not fully implemented
                                        break;
                                    case 7: // LD I, A / LD R, A / LD A, I / LD A, R / RRD / RLD
                                        if (ed_y == 0) { // LD I, A
                                            IR = (IR & 0xff) | (AF & ~0xff);
                                        } else if (ed_y == 1) { // LD R, A
                                            IR = (IR & ~0xff) | ((AF >> 8) & 0xff);
                                        } else if (ed_y == 2) { // LD A, I
                                            AF = (AF & 0x29) | (IR & ~0xff) | ((IR >> 8) & 0x80) | (((IR & ~0xff) == 0) << 6) | ((IFF & 2) << 1);
                                        } else if (ed_y == 3) { // LD A, R
                                            AF = (AF & 0x29) | ((IR & 0xff) << 8) | (IR & 0x80) | (((IR & 0xff) == 0) << 6) | ((IFF & 2) << 1);
                                        } else if (ed_y == 4) { // RRD
                                            uint8 temp = GET_BYTE(HL);
                                            uint8 acu = HIGH_REGISTER(AF);
                                            PUT_BYTE(HL, HIGH_DIGIT(temp) | (LOW_DIGIT(acu) << 4));
                                            AF = xororTable[(acu & 0xf0) | LOW_DIGIT(temp)] | (AF & 1);
                                        } else if (ed_y == 5) { // RLD
                                            uint8 temp = GET_BYTE(HL);
                                            uint8 acu = HIGH_REGISTER(AF);
                                            PUT_BYTE(HL, (LOW_DIGIT(temp) << 4) | LOW_DIGIT(acu));
                                            AF = xororTable[(acu & 0xf0) | HIGH_DIGIT(temp)] | (AF & 1);
                                        }
                                        break;
                                }
                            } else if (ed_x == 2) { // Block instructions
                                uint32 temp, acu, sum, cbits, op;
                                if (ed_z <= 3) { // LDI/CPI/INI/OUTI etc.
                                    int repeat = (ed_y >= 6);
                                    int dec = (ed_y & 1); // 0=Inc, 1=Dec
                                    
                                    switch (ed_z) {
                                        case 0: // LD
                                            if (repeat) { // LDIR/LDDR
                                                BC &= ADDRMASK;
                                                if (BC == 0) BC = 0x10000;
                                                do {
                                                    // INCR(2);
                                                    if (dec) { acu = RAM_MM(HL); PUT_BYTE_MM(DE, acu); }
                                                    else { acu = RAM_PP(HL); PUT_BYTE_PP(DE, acu); }
                                                } while (--BC);
                                                acu += HIGH_REGISTER(AF);
                                                AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4);
                                            } else { // LDI/LDD
                                                if (dec) { acu = RAM_MM(HL); PUT_BYTE_MM(DE, acu); }
                                                else { acu = RAM_PP(HL); PUT_BYTE_PP(DE, acu); }
                                                acu += HIGH_REGISTER(AF);
                                                AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) |
                                                    (((--BC & ADDRMASK) != 0) << 2);
                                            }
                                            break;
                                        case 1: // CP
                                            if (repeat) { // CPIR/CPDR
                                                acu = HIGH_REGISTER(AF);
                                                BC &= ADDRMASK;
                                                if (BC == 0) BC = 0x10000;
                                                do {
                                                    // INCR(1);
                                                    if (dec) temp = RAM_MM(HL); else temp = RAM_PP(HL);
                                                    op = --BC != 0;
                                                    sum = acu - temp;
                                                } while (op && sum != 0);
                                                cbits = acu ^ temp ^ sum;
                                                AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
                                                    (((sum - ((cbits & 16) >> 4)) & 2) << 4) |
                                                    (cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |
                                                    op << 2 | 2;
                                                if ((sum & 15) == 8 && (cbits & 16) != 0) AF &= ~8;
                                            } else { // CPI/CPD
                                                acu = HIGH_REGISTER(AF);
                                                if (dec) temp = RAM_MM(HL); else temp = RAM_PP(HL);
                                                sum = acu - temp;
                                                cbits = acu ^ temp ^ sum;
                                                AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
                                                    (((sum - ((cbits & 16) >> 4)) & 2) << 4) | (cbits & 16) |
                                                    ((sum - ((cbits >> 4) & 1)) & 8) |
                                                    ((--BC & ADDRMASK) != 0) << 2 | 2;
                                                if ((sum & 15) == 8 && (cbits & 16) != 0) AF &= ~8;
                                            }
                                            break;
                                        case 2: // IN
                                            if (repeat) { // INIR/INDR
                                                temp = HIGH_REGISTER(BC);
                                                if (temp == 0) temp = 0x100;
                                                do {
                                                    // INCR(1);
                                                    acu = cpu_in(LOW_REGISTER(BC));
                                                    if (dec) { PUT_BYTE(HL, acu); HL--; }
                                                    else { PUT_BYTE(HL, acu); HL++; }
                                                } while (--temp);
                                                temp = HIGH_REGISTER(BC);
                                                SET_HIGH_REGISTER(BC, 0);
                                                if (dec) INOUTFLAGS_ZERO((LOW_REGISTER(BC) - 1) & 0xff);
                                                else INOUTFLAGS_ZERO((LOW_REGISTER(BC) + 1) & 0xff);
                                            } else { // INI/IND
                                                acu = cpu_in(LOW_REGISTER(BC));
                                                if (dec) { PUT_BYTE(HL, acu); HL--; }
                                                else { PUT_BYTE(HL, acu); HL++; }
                                                temp = HIGH_REGISTER(BC);
                                                BC -= 0x100;
                                                if (dec) INOUTFLAGS_NONZERO((LOW_REGISTER(BC) - 1) & 0xff);
                                                else INOUTFLAGS_NONZERO((LOW_REGISTER(BC) + 1) & 0xff);
                                            }
                                            break;
                                        case 3: // OUT
                                            if (repeat) { // OTIR/OTDR
                                                temp = HIGH_REGISTER(BC);
                                                if (temp == 0) temp = 0x100;
                                                do {
                                                    // INCR(1);
                                                    if (dec) { acu = GET_BYTE(HL); HL--; }
                                                    else { acu = GET_BYTE(HL); HL++; }
                                                    cpu_out(LOW_REGISTER(BC), acu);
                                                } while (--temp);
                                                temp = HIGH_REGISTER(BC);
                                                SET_HIGH_REGISTER(BC, 0);
                                                INOUTFLAGS_ZERO(LOW_REGISTER(HL)); 
                                            } else { // OUTI/OUTD
                                                if (dec) { acu = GET_BYTE(HL); HL--; }
                                                else { acu = GET_BYTE(HL); HL++; }
                                                cpu_out(LOW_REGISTER(BC), acu);
                                                temp = HIGH_REGISTER(BC);
                                                BC -= 0x100;
                                                INOUTFLAGS_NONZERO(LOW_REGISTER(HL)); 
                                            }
                                            break;
                                    }
                                }
                            }
                        } else if (p == 3) { // FD Prefix
                            // Handled at start.
                        }
                    }
                    break;
                case 6: // ALU[y] n
                    val = RAM_PP(PC);
                    alu(y, val);
                    break;
                case 7: // RST y*8
                    PUSH(PC);
                    PC = y * 8;
#ifdef INT_HANDOFF
                    if (y == 1) { // RST 8
                        _Bios();
                        POP(PC);
                    } else if (y == 2) { // RST 10H
                        _Bdos();
                        POP(PC);
                    }
#endif
                    break;
            }
        }
    }
}

#include "cpu_mhz.h"

#endif
