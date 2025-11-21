#ifndef CPU_H
#define CPU_H

/* Model 4 - Algorithmic Decoding, based on http://www.z80.info/decoding.htm, just faster */

#define CPU_IS "Model 4"

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
#else
static void Z80debug(void) {}
#endif

#ifdef DEBUG
#define DO_DEBUG_HALT 1
#else
#define DO_DEBUG_HALT 0
#endif
#ifdef DEBUGONHALT
#define DO_DEBUG_ON_HALT 1
#else
#define DO_DEBUG_ON_HALT 0
#endif
#ifdef INT_HANDOFF
#define DO_INT_HANDOFF 1
#else
#define DO_INT_HANDOFF 0
#endif

#define EXEC_OP(op_val) \
    { \
        uint8 opcode = op_val; \
        uint8 x, y, z, p, q; \
        int8 d = 0; \
        uint32 ea = 0; \
        uint8 val; \
        if (opcode == 0xDD) { \
            mode = 1; \
            opcode = RAM_PP(PC); \
            IR = (IR & 0xff00) | ((IR + 1) & 0x7f) | (IR & 0x80); \
        } else if (opcode == 0xFD) { \
            mode = 2; \
            opcode = RAM_PP(PC); \
            IR = (IR & 0xff00) | ((IR + 1) & 0x7f) | (IR & 0x80); \
        } \
        x = (opcode >> 6) & 3; \
        y = (opcode >> 3) & 7; \
        z = opcode & 7; \
        p = y >> 1; \
        q = y & 1; \
        if (x == 0) { \
            switch (z) { \
                case 0: \
                    if (y == 0) { /* NOP */ } \
                    else if (y == 1) { /* EX AF, AF' */ \
                        int32 tmp = AF; AF = AF1; AF1 = tmp; \
                    } \
                    else if (y == 2) { /* DJNZ d */ \
                        d = (int8)RAM_PP(PC); \
                        uint8 b = HIGH_REGISTER(BC) - 1; \
                        SET_HIGH_REGISTER(BC, b); \
                        if (b != 0) PC += d; \
                    } \
                    else if (y == 3) { /* JR d */ \
                        d = (int8)RAM_PP(PC); \
                        PC += d; \
                    } \
                    else if (y >= 4) { /* JR cc, d */ \
                        d = (int8)RAM_PP(PC); \
                        int cc = y - 4; \
                        int cond = 0; \
                        switch (cc) { \
                            case 0: cond = !TSTFLAG(Z); break; \
                            case 1: cond = TSTFLAG(Z); break; \
                            case 2: cond = !TSTFLAG(C); break; \
                            case 3: cond = TSTFLAG(C); break; \
                        } \
                        if (cond) PC += d; \
                    } \
                    break; \
                case 1: \
                    if (q == 0) { /* LD rp[p], nn */ \
                        uint16 nn = GET_WORD(PC); \
                        PC += 2; \
                        if (mode == 0) { \
                            if (p == 0) BC = nn; \
                            else if (p == 1) DE = nn; \
                            else if (p == 2) HL = nn; \
                            else SP = nn; \
                        } else { \
                            if (p == 2) { if (mode == 1) IX = nn; else IY = nn; } \
                            else if (p == 0) BC = nn; \
                            else if (p == 1) DE = nn; \
                            else SP = nn; \
                        } \
                    } else { /* ADD HL, rp[p] */ \
                        uint32 val; \
                        uint32 base; \
                        if (mode == 0) base = HL; else base = (mode == 1) ? IX : IY; \
                        if (p == 0) val = BC; \
                        else if (p == 1) val = DE; \
                        else if (p == 2) val = (mode == 0) ? HL : ((mode == 1) ? IX : IY); \
                        else val = SP; \
                        base &= 0xffff; \
                        val &= 0xffff; \
                        uint32 res = base + val; \
                        AF = (AF & ~(FLAG_N | FLAG_C | FLAG_H | FLAG_3 | FLAG_5)) | \
                             ((res >> 8) & (FLAG_3 | FLAG_5)) | \
                             (((base ^ val ^ res) >> 8) & FLAG_H) | \
                             ((res >> 16) & FLAG_C); \
                        if (mode == 0) HL = res & 0xffff; \
                        else if (mode == 1) IX = res & 0xffff; \
                        else IY = res & 0xffff; \
                    } \
                    break; \
                case 2: \
                    if (q == 0) { \
                        if (p == 0) PUT_BYTE(BC, HIGH_REGISTER(AF)); \
                        else if (p == 1) PUT_BYTE(DE, HIGH_REGISTER(AF)); \
                        else if (p == 2) { \
                            uint16 nn = GET_WORD(PC); PC += 2; \
                            PUT_WORD(nn, (mode == 0) ? HL : ((mode == 1) ? IX : IY)); \
                        } \
                        else { \
                            uint16 nn = GET_WORD(PC); PC += 2; \
                            PUT_BYTE(nn, HIGH_REGISTER(AF)); \
                        } \
                    } else { \
                        if (p == 0) SET_HIGH_REGISTER(AF, GET_BYTE(BC)); \
                        else if (p == 1) SET_HIGH_REGISTER(AF, GET_BYTE(DE)); \
                        else if (p == 2) { \
                            uint16 nn = GET_WORD(PC); PC += 2; \
                            uint16 v = GET_WORD(nn); \
                            if (mode == 0) HL = v; else if (mode == 1) IX = v; else IY = v; \
                        } \
                        else { \
                            uint16 nn = GET_WORD(PC); PC += 2; \
                            SET_HIGH_REGISTER(AF, GET_BYTE(nn)); \
                        } \
                    } \
                    break; \
                case 3: \
                    { \
                        int32 *rp_ptr; \
                        if (p == 0) rp_ptr = &BC; \
                        else if (p == 1) rp_ptr = &DE; \
                        else if (p == 2) rp_ptr = (mode == 0) ? &HL : ((mode == 1) ? &IX : &IY); \
                        else rp_ptr = &SP; \
                        if (q == 0) (*rp_ptr)++; else (*rp_ptr)--; \
                        *rp_ptr &= 0xffff; \
                    } \
                    break; \
                case 4: \
                    { \
                        uint8 v; \
                        if (y == 6) { \
                            if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); } \
                            else { ea = HL; v = GET_BYTE(ea); } \
                        } else { \
                            if (mode != 0 && (y == 4 || y == 5)) { \
                                int32 val = (mode == 1) ? IX : IY; \
                                if (y == 4) v = (val >> 8) & 0xff; else v = val & 0xff; \
                            } else v = get_reg8(y); \
                        } \
                        uint8 res = v + 1; \
                        AF = (AF & ~0xfe) | (res & 0xa8) | ((res == 0) ? FLAG_Z : 0) | ((res & 0x0f) ? 0 : FLAG_H) | ((res == 0x80) ? FLAG_P : 0); \
                        if (y == 6) PUT_BYTE(ea, res); \
                        else if (mode != 0 && (y == 4 || y == 5)) { \
                             int32 *rp_ptr = (mode == 1) ? &IX : &IY; \
                             if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (res << 8); \
                             else *rp_ptr = (*rp_ptr & 0xff00) | res; \
                        } else set_reg8(y, res); \
                    } \
                    break; \
                case 5: \
                    { \
                        uint8 v; \
                        if (y == 6) { \
                            if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); } \
                            else { ea = HL; v = GET_BYTE(ea); } \
                        } else { \
                            if (mode != 0 && (y == 4 || y == 5)) { \
                                int32 val = (mode == 1) ? IX : IY; \
                                if (y == 4) v = (val >> 8) & 0xff; else v = val & 0xff; \
                            } else v = get_reg8(y); \
                        } \
                        uint8 res = v - 1; \
                        AF = (AF & ~0xfe) | (res & 0xa8) | ((res == 0) ? FLAG_Z : 0) | ((res & 0x0f) == 0x0f ? FLAG_H : 0) | ((res == 0x7f) ? FLAG_P : 0) | FLAG_N; \
                        if (y == 6) PUT_BYTE(ea, res); \
                        else if (mode != 0 && (y == 4 || y == 5)) { \
                             int32 *rp_ptr = (mode == 1) ? &IX : &IY; \
                             if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (res << 8); \
                             else *rp_ptr = (*rp_ptr & 0xff00) | res; \
                        } else set_reg8(y, res); \
                    } \
                    break; \
                case 6: \
                    { \
                        if (mode != 0 && y == 6) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; } \
                        uint8 n = RAM_PP(PC); \
                        if (y == 6) { \
                            if (mode == 0) PUT_BYTE(HL, n); \
                            else PUT_BYTE(ea, n); \
                        } else if (mode != 0 && (y == 4 || y == 5)) { \
                             int32 *rp_ptr = (mode == 1) ? &IX : &IY; \
                             if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (n << 8); \
                             else *rp_ptr = (*rp_ptr & 0xff00) | n; \
                        } else set_reg8(y, n); \
                    } \
                    break; \
                case 7: \
                    { \
                        uint8 a = HIGH_REGISTER(AF); \
                        uint8 f = LOW_REGISTER(AF); \
                        uint8 c = (f & FLAG_C) ? 1 : 0; \
                        uint32 temp, cbits, acu; \
                        switch (y) { \
                            case 0: \
                                AF = ((AF >> 7) & 0x0128) | ((AF << 1) & ~0x1ff) | (AF & 0xc4) | ((AF >> 15) & 1); \
                                break; \
                            case 1: \
                                AF = (AF & 0xc4) | rrcaTable[HIGH_REGISTER(AF)]; \
                                break; \
                            case 2: \
                                AF = ((AF << 8) & 0x0100) | ((AF >> 7) & 0x28) | ((AF << 1) & ~0x01ff) | (AF & 0xc4) | ((AF >> 15) & 1); \
                                break; \
                            case 3: \
                                AF = ((AF & 1) << 15) | (AF & 0xc4) | (rrcaTable[HIGH_REGISTER(AF)] & 0x7fff); \
                                break; \
                            case 4: \
                                acu = HIGH_REGISTER(AF); \
                                temp = LOW_DIGIT(acu); \
                                cbits = TSTFLAG(C); \
                                if (TSTFLAG(N)) {   /* last operation was a subtract */ \
                                    int hd = cbits || acu > 0x99; \
                                    if (TSTFLAG(H) || (temp > 9)) { /* adjust low digit */ \
                                        if (temp > 5) \
                                            SETFLAG(H, 0); \
                                        acu -= 6; \
                                        acu &= 0xff; \
                                    } \
                                    if (hd) \
                                        acu -= 0x160;   /* adjust high digit */ \
                                } else {          /* last operation was an add */ \
                                    if (TSTFLAG(H) || (temp > 9)) { /* adjust low digit */ \
                                        SETFLAG(H, (temp > 9)); \
                                        acu += 6; \
                                    } \
                                    if (cbits || ((acu & 0x1f0) > 0x90)) \
                                        acu += 0x60;   /* adjust high digit */ \
                                } \
                                AF = (AF & 0x12) | xororTable[acu & 0xff] | ((acu >> 8) & 1) | cbits; \
                                break; \
                            case 5: \
                                AF = (~AF & ~0xff) | (AF & 0xc5) | ((~AF >> 8) & 0x28) | 0x12; \
                                break; \
                            case 6: \
                                AF = (AF & ~0x3b) | ((AF >> 8) & 0x28) | 1; \
                                break; \
                            case 7: \
                                AF = (AF & ~0x3b) | ((AF >> 8) & 0x28) | ((AF & 1) << 4) | (~AF & 1); \
                                break; \
                        } \
                    } \
                    break; \
            } \
        } else if (x == 1) { \
            if (y == 6 && z == 6) { \
if (DO_DEBUG_HALT) { \
			_puts("\r\n::CPU HALTED::\r\n"); \
			_puts("Press any key..."); \
			_getcon(); \
if (DO_DEBUG_HALT) { \
			_puts("\r\n"); \
			Debug = 1; \
			Z80debug(); \
} \
} \
                PC--; \
                Status = STATUS_EXIT; \
            } else { \
                uint8 v; \
                int use_ix_d = (z == 6 || y == 6); \
                if (z == 6) { \
                    if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); } \
                    else v = GET_BYTE(HL); \
                } else if (mode != 0 && !use_ix_d && (z == 4 || z == 5)) { \
                    int32 val = (mode == 1) ? IX : IY; \
                    if (z == 4) v = (val >> 8) & 0xff; else v = val & 0xff; \
                } else v = get_reg8(z); \
                if (y == 6) { \
                    if (mode != 0) { \
                        d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; \
                        PUT_BYTE(ea, v); \
                    } else PUT_BYTE(HL, v); \
                } else if (mode != 0 && !use_ix_d && (y == 4 || y == 5)) { \
                     int32 *rp_ptr = (mode == 1) ? &IX : &IY; \
                     if (y == 4) *rp_ptr = (*rp_ptr & 0xff) | (v << 8); \
                     else *rp_ptr = (*rp_ptr & 0xff00) | v; \
                } else set_reg8(y, v); \
            } \
        } else if (x == 2) { \
            uint8 v; \
            if (z == 6) { \
                if (mode != 0) { d = (int8)RAM_PP(PC); ea = ((mode == 1) ? IX : IY) + d; v = GET_BYTE(ea); } \
                else v = GET_BYTE(HL); \
            } else if (mode != 0 && (z == 4 || z == 5)) { \
                int32 val = (mode == 1) ? IX : IY; \
                if (z == 4) v = (val >> 8) & 0xff; else v = val & 0xff; \
            } else v = get_reg8(z); \
            alu(y, v); \
        } else { \
            switch (z) { \
                case 0: \
                    { \
                        int cc = y; \
                        int cond = 0; \
                        switch (cc) { \
                            case 0: cond = !TSTFLAG(Z); break; \
                            case 1: cond = TSTFLAG(Z); break; \
                            case 2: cond = !TSTFLAG(C); break; \
                            case 3: cond = TSTFLAG(C); break; \
                            case 4: cond = !TSTFLAG(P); break; \
                            case 5: cond = TSTFLAG(P); break; \
                            case 6: cond = !TSTFLAG(S); break; \
                            case 7: cond = TSTFLAG(S); break; \
                        } \
                        if (cond) { \
                            uint16 ret; POP(ret); PC = ret; \
                        } \
                    } \
                    break; \
                case 1: \
                    if (q == 0) { \
                        uint16 v; POP(v); \
                        if (p == 0) BC = v; \
                        else if (p == 1) DE = v; \
                        else if (p == 2) { if (mode == 0) HL = v; else if (mode == 1) IX = v; else IY = v; } \
                        else AF = v; \
                    } else { \
                        if (p == 0) { uint16 ret; POP(ret); PC = ret; } \
                        else if (p == 1) { \
                            int32 tmp; \
                            tmp = BC; BC = BC1; BC1 = tmp; \
                            tmp = DE; DE = DE1; DE1 = tmp; \
                            tmp = HL; HL = HL1; HL1 = tmp; \
                        } \
                        else if (p == 2) { \
                            if (mode == 0) PC = HL; else if (mode == 1) PC = IX; else PC = IY; \
                        } \
                        else { \
                            if (mode == 0) SP = HL; else if (mode == 1) SP = IX; else SP = IY; \
                        } \
                    } \
                    break; \
                case 2: \
                    { \
                        uint16 nn = GET_WORD(PC); PC += 2; \
                        int cc = y; \
                        int cond = 0; \
                        switch (cc) { \
                            case 0: cond = !TSTFLAG(Z); break; \
                            case 1: cond = TSTFLAG(Z); break; \
                            case 2: cond = !TSTFLAG(C); break; \
                            case 3: cond = TSTFLAG(C); break; \
                            case 4: cond = !TSTFLAG(P); break; \
                            case 5: cond = TSTFLAG(P); break; \
                            case 6: cond = !TSTFLAG(S); break; \
                            case 7: cond = TSTFLAG(S); break; \
                        } \
                        if (cond) PC = nn; \
                    } \
                    break; \
                case 3: \
                    if (y == 0) { \
                        uint16 nn = GET_WORD(PC); PC = nn; \
                    } else if (y == 1) { \
                        if (mode != 0) { \
                            d = (int8)RAM_PP(PC); \
                            opcode = RAM_PP(PC); \
                        } else { \
                            opcode = RAM_PP(PC); \
                        } \
                        int cb_x = (opcode >> 6) & 3; \
                        int cb_y = (opcode >> 3) & 7; \
                        int cb_z = opcode & 7; \
                        uint8 val; \
                        uint32 addr = 0; \
                        if (mode != 0) { \
                            addr = ((mode == 1) ? IX : IY) + d; \
                            val = GET_BYTE(addr); \
                        } else { \
                            val = get_reg8(cb_z); \
                        } \
                        uint8 res = val; \
                        if (cb_x == 0) { \
                            uint8 cbits = 0; \
                            switch (cb_y) { \
                                case 0: \
                                    res = (val << 1) | (val >> 7); cbits = res & 1; break; \
                                case 1: \
                                    res = (val >> 1) | (val << 7); cbits = res & 0x80; break; \
                                case 2: \
                                    res = (val << 1) | TSTFLAG(C); cbits = val & 0x80; break; \
                                case 3: \
                                    res = (val >> 1) | (TSTFLAG(C) << 7); cbits = val & 1; break; \
                                case 4: \
                                    res = val << 1; cbits = val & 0x80; break; \
                                case 5: \
                                    res = (val >> 1) | (val & 0x80); cbits = val & 1; break; \
                                case 6: \
                                    res = (val << 1) | 1; cbits = val & 0x80; break; \
                                case 7: \
                                    res = val >> 1; cbits = val & 1; break; \
                            } \
                            AF = (AF & ~0xff) | rotateShiftTable[res] | !!cbits; \
                            if (mode != 0) PUT_BYTE(addr, res); \
                            else set_reg8(cb_z, res); \
                        } else if (cb_x == 1) { \
                            if (val & (1 << cb_y)) \
                                AF = (AF & ~0xfe) | 0x10 | ((cb_y == 7) << 7); \
                            else \
                                AF = (AF & ~0xfe) | 0x54; \
                            if (cb_z != 6 && mode == 0) \
                                AF |= (val & 0x28); \
                        } else if (cb_x == 2) { \
                            res &= ~(1 << cb_y); \
                            if (mode != 0) PUT_BYTE(addr, res); \
                            else set_reg8(cb_z, res); \
                        } else { \
                            res |= (1 << cb_y); \
                            if (mode != 0) PUT_BYTE(addr, res); \
                            else set_reg8(cb_z, res); \
                        } \
                    } else if (y == 2) { \
                        uint8 n = RAM_PP(PC); cpu_out(n, HIGH_REGISTER(AF)); \
                    } else if (y == 3) { \
                        uint8 n = RAM_PP(PC); SET_HIGH_REGISTER(AF, cpu_in(n)); \
                    } else if (y == 4) { \
                        uint16 v; POP(v); \
                        uint16 old; \
                        if (mode == 0) { old = HL; HL = v; } \
                        else if (mode == 1) { old = IX; IX = v; } \
                        else { old = IY; IY = v; } \
                        PUSH(old); \
                    } else if (y == 5) { \
                        int32 tmp = DE; DE = HL; HL = tmp; \
                    } else if (y == 6) { \
                        IFF = 0; \
                    } else if (y == 7) { \
                        IFF = 1; \
                    } \
                    break; \
                case 4: \
                    { \
                        uint16 nn = GET_WORD(PC); PC += 2; \
                        int cc = y; \
                        int cond = 0; \
                        switch (cc) { \
                            case 0: cond = !TSTFLAG(Z); break; \
                            case 1: cond = TSTFLAG(Z); break; \
                            case 2: cond = !TSTFLAG(C); break; \
                            case 3: cond = TSTFLAG(C); break; \
                            case 4: cond = !TSTFLAG(P); break; \
                            case 5: cond = TSTFLAG(P); break; \
                            case 6: cond = !TSTFLAG(S); break; \
                            case 7: cond = TSTFLAG(S); break; \
                        } \
                        if (cond) { \
                            PUSH(PC); PC = nn; \
                        } \
                    } \
                    break; \
                case 5: \
                    if (q == 0) { \
                        if (p == 0) PUSH(BC); \
                        else if (p == 1) PUSH(DE); \
                        else if (p == 2) PUSH((mode == 0) ? HL : ((mode == 1) ? IX : IY)); \
                        else PUSH(AF); \
                    } else { \
                        if (p == 0) { \
                            uint16 nn = GET_WORD(PC); PC += 2; \
                            PUSH(PC); PC = nn; \
                        } else if (p == 1) { \
                        } else if (p == 2) { \
                            opcode = RAM_PP(PC); \
                            int ed_x = (opcode >> 6) & 3; \
                            int ed_y = (opcode >> 3) & 7; \
                            int ed_z = opcode & 7; \
                            int ed_p = ed_y >> 1; \
                            int ed_q = ed_y & 1; \
                            if (ed_x == 1) { \
                                switch (ed_z) { \
                                    case 0: \
                                        { \
                                            uint8 v = cpu_in(LOW_REGISTER(BC)); \
                                            if (ed_y != 6) set_reg8(ed_y, v); \
                                            AF = (AF & ~0xfe) | rotateShiftTable[v]; \
                                        } \
                                        break; \
                                    case 1: \
                                        if (ed_y != 6) cpu_out(LOW_REGISTER(BC), get_reg8(ed_y)); \
                                        else cpu_out(LOW_REGISTER(BC), 0); \
                                        break; \
                                    case 2: \
                                        { \
                                            uint32 val; \
                                            if (ed_p == 0) val = BC; else if (ed_p == 1) val = DE; else if (ed_p == 2) val = HL; else val = SP; \
                                            uint32 sum; \
                                            HL &= ADDRMASK; \
                                            val &= ADDRMASK; \
                                            if (ed_q == 0) { \
                                                sum = HL - val - TSTFLAG(C); \
                                                AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) | \
                                                    cbits2Z80Table[((HL ^ val ^ sum) >> 8) & 0x1ff]; \
                                                HL = sum; \
                                            } else { \
                                                sum = HL + val + TSTFLAG(C); \
                                                AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) | \
                                                    cbitsZ80Table[(HL ^ val ^ sum) >> 8]; \
                                                HL = sum; \
                                            } \
                                        } \
                                        break; \
                                    case 3: \
                                        { \
                                            uint16 nn = GET_WORD(PC); PC += 2; \
                                            if (ed_q == 0) { \
                                                if (ed_p == 0) PUT_WORD(nn, BC); \
                                                else if (ed_p == 1) PUT_WORD(nn, DE); \
                                                else if (ed_p == 2) PUT_WORD(nn, HL); \
                                                else PUT_WORD(nn, SP); \
                                            } else { \
                                                uint16 v = GET_WORD(nn); \
                                                if (ed_p == 0) BC = v; \
                                                else if (ed_p == 1) DE = v; \
                                                else if (ed_p == 2) HL = v; \
                                                else SP = v; \
                                            } \
                                        } \
                                        break; \
                                    case 4: \
                                        { \
                                            uint8 a = HIGH_REGISTER(AF); \
                                            uint8 temp = 0 - a; \
                                            AF = (AF & ~0xff) | ((a & 0x0f) ? FLAG_H : 0) | ((a == 0x80) ? FLAG_P : 0) | FLAG_N | (a ? FLAG_C : 0) | (temp & 0xa8) | \
                                                ((temp == 0) << 6) | ((a ^ temp) & 0x10); \
                                            SET_HIGH_REGISTER(AF, temp); \
                                        } \
                                        break; \
                                    case 5: \
                                        { uint16 ret; POP(ret); PC = ret; IFF |= IFF >> 1; } \
                                        break; \
                                    case 6: \
                                        break; \
                                    case 7: \
                                        if (ed_y == 0) { \
                                            IR = (IR & 0xff) | (AF & ~0xff); \
                                        } else if (ed_y == 1) { \
                                            IR = (IR & ~0xff) | ((AF >> 8) & 0xff); \
                                        } else if (ed_y == 2) { \
                                            AF = (AF & 0x29) | (IR & ~0xff) | ((IR >> 8) & 0x80) | (((IR & ~0xff) == 0) << 6) | ((IFF & 2) << 1); \
                                        } else if (ed_y == 3) { \
                                            AF = (AF & 0x29) | ((IR & 0xff) << 8) | (IR & 0x80) | (((IR & 0xff) == 0) << 6) | ((IFF & 2) << 1); \
                                        } else if (ed_y == 4) { \
                                            uint8 temp = GET_BYTE(HL); \
                                            uint8 acu = HIGH_REGISTER(AF); \
                                            PUT_BYTE(HL, HIGH_DIGIT(temp) | (LOW_DIGIT(acu) << 4)); \
                                            AF = xororTable[(acu & 0xf0) | LOW_DIGIT(temp)] | (AF & 1); \
                                        } else if (ed_y == 5) { \
                                            uint8 temp = GET_BYTE(HL); \
                                            uint8 acu = HIGH_REGISTER(AF); \
                                            PUT_BYTE(HL, (LOW_DIGIT(temp) << 4) | LOW_DIGIT(acu)); \
                                            AF = xororTable[(acu & 0xf0) | HIGH_DIGIT(temp)] | (AF & 1); \
                                        } \
                                        break; \
                                } \
                            } else if (ed_x == 2) { \
                                uint32 temp, acu, sum, cbits, op; \
                                if (ed_z <= 3) { \
                                    int repeat = (ed_y >= 6); \
                                    int dec = (ed_y & 1); \
                                    switch (ed_z) { \
                                        case 0: \
                                            if (repeat) { \
                                                BC &= ADDRMASK; \
                                                if (BC == 0) BC = 0x10000; \
                                                do { \
                                                    if (dec) { acu = RAM_MM(HL); PUT_BYTE_MM(DE, acu); } \
                                                    else { acu = RAM_PP(HL); PUT_BYTE_PP(DE, acu); } \
                                                } while (--BC); \
                                                acu += HIGH_REGISTER(AF); \
                                                AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4); \
                                            } else { \
                                                if (dec) { acu = RAM_MM(HL); PUT_BYTE_MM(DE, acu); } \
                                                else { acu = RAM_PP(HL); PUT_BYTE_PP(DE, acu); } \
                                                acu += HIGH_REGISTER(AF); \
                                                AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) | \
                                                    (((--BC & ADDRMASK) != 0) << 2); \
                                            } \
                                            break; \
                                        case 1: \
                                            if (repeat) { \
                                                acu = HIGH_REGISTER(AF); \
                                                BC &= ADDRMASK; \
                                                if (BC == 0) BC = 0x10000; \
                                                do { \
                                                    if (dec) temp = RAM_MM(HL); else temp = RAM_PP(HL); \
                                                    op = --BC != 0; \
                                                    sum = acu - temp; \
                                                } while (op && sum != 0); \
                                                cbits = acu ^ temp ^ sum; \
                                                AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) | \
                                                    (((sum - ((cbits & 16) >> 4)) & 2) << 4) | \
                                                    (cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) | \
                                                    op << 2 | 2; \
                                                if ((sum & 15) == 8 && (cbits & 16) != 0) AF &= ~8; \
                                            } else { \
                                                acu = HIGH_REGISTER(AF); \
                                                if (dec) temp = RAM_MM(HL); else temp = RAM_PP(HL); \
                                                sum = acu - temp; \
                                                cbits = acu ^ temp ^ sum; \
                                                AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) | \
                                                    (((sum - ((cbits & 16) >> 4)) & 2) << 4) | (cbits & 16) | \
                                                    ((sum - ((cbits >> 4) & 1)) & 8) | \
                                                    ((--BC & ADDRMASK) != 0) << 2 | 2; \
                                                if ((sum & 15) == 8 && (cbits & 16) != 0) AF &= ~8; \
                                            } \
                                            break; \
                                        case 2: \
                                            if (repeat) { \
                                                temp = HIGH_REGISTER(BC); \
                                                if (temp == 0) temp = 0x100; \
                                                do { \
                                                    acu = cpu_in(LOW_REGISTER(BC)); \
                                                    if (dec) { PUT_BYTE(HL, acu); HL--; } \
                                                    else { PUT_BYTE(HL, acu); HL++; } \
                                                } while (--temp); \
                                                temp = HIGH_REGISTER(BC); \
                                                SET_HIGH_REGISTER(BC, 0); \
                                                if (dec) INOUTFLAGS_ZERO((LOW_REGISTER(BC) - 1) & 0xff); \
                                                else INOUTFLAGS_ZERO((LOW_REGISTER(BC) + 1) & 0xff); \
                                            } else { \
                                                acu = cpu_in(LOW_REGISTER(BC)); \
                                                if (dec) { PUT_BYTE(HL, acu); HL--; } \
                                                else { PUT_BYTE(HL, acu); HL++; } \
                                                temp = HIGH_REGISTER(BC); \
                                                BC -= 0x100; \
                                                if (dec) INOUTFLAGS_NONZERO((LOW_REGISTER(BC) - 1) & 0xff); \
                                                else INOUTFLAGS_NONZERO((LOW_REGISTER(BC) + 1) & 0xff); \
                                            } \
                                            break; \
                                        case 3: \
                                            if (repeat) { \
                                                temp = HIGH_REGISTER(BC); \
                                                if (temp == 0) temp = 0x100; \
                                                do { \
                                                    if (dec) { acu = GET_BYTE(HL); HL--; } \
                                                    else { acu = GET_BYTE(HL); HL++; } \
                                                    cpu_out(LOW_REGISTER(BC), acu); \
                                                } while (--temp); \
                                                temp = HIGH_REGISTER(BC); \
                                                SET_HIGH_REGISTER(BC, 0); \
                                                INOUTFLAGS_ZERO(LOW_REGISTER(HL)); \
                                            } else { \
                                                if (dec) { acu = GET_BYTE(HL); HL--; } \
                                                else { acu = GET_BYTE(HL); HL++; } \
                                                cpu_out(LOW_REGISTER(BC), acu); \
                                                temp = HIGH_REGISTER(BC); \
                                                BC -= 0x100; \
                                                INOUTFLAGS_NONZERO(LOW_REGISTER(HL)); \
                                            } \
                                            break; \
                                    } \
                                } \
                            } \
                        } else if (p == 3) { \
                        } \
                    } \
                    break; \
                case 6: \
                    val = RAM_PP(PC); \
                    alu(y, val); \
                    break; \
                case 7: \
                    PUSH(PC); \
                    PC = y * 8; \
if (DO_INT_HANDOFF) { \
                    if (y == 1) { \
                        _Bios(); \
                        POP(PC); \
                    } else if (y == 2) { \
                        _Bdos(); \
                        POP(PC); \
                    } \
} \
                    break; \
            } \
        } \
    }

static inline void Z80run(void) {
    uint8 opcode;
    int mode = 0;
    
#ifdef __GNUC__
    static void *jumptable[256] = {
        &&OP_00,        &&OP_01,        &&OP_02,        &&OP_03,        &&OP_04,        &&OP_05,        &&OP_06,        &&OP_07,
        &&OP_08,        &&OP_09,        &&OP_0A,        &&OP_0B,        &&OP_0C,        &&OP_0D,        &&OP_0E,        &&OP_0F,
        &&OP_10,        &&OP_11,        &&OP_12,        &&OP_13,        &&OP_14,        &&OP_15,        &&OP_16,        &&OP_17,
        &&OP_18,        &&OP_19,        &&OP_1A,        &&OP_1B,        &&OP_1C,        &&OP_1D,        &&OP_1E,        &&OP_1F,
        &&OP_20,        &&OP_21,        &&OP_22,        &&OP_23,        &&OP_24,        &&OP_25,        &&OP_26,        &&OP_27,
        &&OP_28,        &&OP_29,        &&OP_2A,        &&OP_2B,        &&OP_2C,        &&OP_2D,        &&OP_2E,        &&OP_2F,
        &&OP_30,        &&OP_31,        &&OP_32,        &&OP_33,        &&OP_34,        &&OP_35,        &&OP_36,        &&OP_37,
        &&OP_38,        &&OP_39,        &&OP_3A,        &&OP_3B,        &&OP_3C,        &&OP_3D,        &&OP_3E,        &&OP_3F,
        &&OP_40,        &&OP_41,        &&OP_42,        &&OP_43,        &&OP_44,        &&OP_45,        &&OP_46,        &&OP_47,
        &&OP_48,        &&OP_49,        &&OP_4A,        &&OP_4B,        &&OP_4C,        &&OP_4D,        &&OP_4E,        &&OP_4F,
        &&OP_50,        &&OP_51,        &&OP_52,        &&OP_53,        &&OP_54,        &&OP_55,        &&OP_56,        &&OP_57,
        &&OP_58,        &&OP_59,        &&OP_5A,        &&OP_5B,        &&OP_5C,        &&OP_5D,        &&OP_5E,        &&OP_5F,
        &&OP_60,        &&OP_61,        &&OP_62,        &&OP_63,        &&OP_64,        &&OP_65,        &&OP_66,        &&OP_67,
        &&OP_68,        &&OP_69,        &&OP_6A,        &&OP_6B,        &&OP_6C,        &&OP_6D,        &&OP_6E,        &&OP_6F,
        &&OP_70,        &&OP_71,        &&OP_72,        &&OP_73,        &&OP_74,        &&OP_75,        &&OP_76,        &&OP_77,
        &&OP_78,        &&OP_79,        &&OP_7A,        &&OP_7B,        &&OP_7C,        &&OP_7D,        &&OP_7E,        &&OP_7F,
        &&OP_80,        &&OP_81,        &&OP_82,        &&OP_83,        &&OP_84,        &&OP_85,        &&OP_86,        &&OP_87,
        &&OP_88,        &&OP_89,        &&OP_8A,        &&OP_8B,        &&OP_8C,        &&OP_8D,        &&OP_8E,        &&OP_8F,
        &&OP_90,        &&OP_91,        &&OP_92,        &&OP_93,        &&OP_94,        &&OP_95,        &&OP_96,        &&OP_97,
        &&OP_98,        &&OP_99,        &&OP_9A,        &&OP_9B,        &&OP_9C,        &&OP_9D,        &&OP_9E,        &&OP_9F,
        &&OP_A0,        &&OP_A1,        &&OP_A2,        &&OP_A3,        &&OP_A4,        &&OP_A5,        &&OP_A6,        &&OP_A7,
        &&OP_A8,        &&OP_A9,        &&OP_AA,        &&OP_AB,        &&OP_AC,        &&OP_AD,        &&OP_AE,        &&OP_AF,
        &&OP_B0,        &&OP_B1,        &&OP_B2,        &&OP_B3,        &&OP_B4,        &&OP_B5,        &&OP_B6,        &&OP_B7,
        &&OP_B8,        &&OP_B9,        &&OP_BA,        &&OP_BB,        &&OP_BC,        &&OP_BD,        &&OP_BE,        &&OP_BF,
        &&OP_C0,        &&OP_C1,        &&OP_C2,        &&OP_C3,        &&OP_C4,        &&OP_C5,        &&OP_C6,        &&OP_C7,
        &&OP_C8,        &&OP_C9,        &&OP_CA,        &&OP_CB,        &&OP_CC,        &&OP_CD,        &&OP_CE,        &&OP_CF,
        &&OP_D0,        &&OP_D1,        &&OP_D2,        &&OP_D3,        &&OP_D4,        &&OP_D5,        &&OP_D6,        &&OP_D7,
        &&OP_D8,        &&OP_D9,        &&OP_DA,        &&OP_DB,        &&OP_DC,        &&OP_DD,        &&OP_DE,        &&OP_DF,
        &&OP_E0,        &&OP_E1,        &&OP_E2,        &&OP_E3,        &&OP_E4,        &&OP_E5,        &&OP_E6,        &&OP_E7,
        &&OP_E8,        &&OP_E9,        &&OP_EA,        &&OP_EB,        &&OP_EC,        &&OP_ED,        &&OP_EE,        &&OP_EF,
        &&OP_F0,        &&OP_F1,        &&OP_F2,        &&OP_F3,        &&OP_F4,        &&OP_F5,        &&OP_F6,        &&OP_F7,
        &&OP_F8,        &&OP_F9,        &&OP_FA,        &&OP_FB,        &&OP_FC,        &&OP_FD,        &&OP_FE,        &&OP_FF,
    };
#endif

    while (!Status) {
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
#if defined(DEBUG) || defined(iDEBUG)
z80_trace_push(PCX);
#endif

    mode = 0;
    DISPATCH:
        opcode = RAM_PP(PC);
        IR = (IR & 0xff00) | ((IR + 1) & 0x7f) | (IR & 0x80);
#ifdef __GNUC__
        goto *jumptable[opcode];
#else
        switch (opcode) {
            case 0x00: goto OP_00;
            case 0x01: goto OP_01;
            case 0x02: goto OP_02;
            case 0x03: goto OP_03;
            case 0x04: goto OP_04;
            case 0x05: goto OP_05;
            case 0x06: goto OP_06;
            case 0x07: goto OP_07;
            case 0x08: goto OP_08;
            case 0x09: goto OP_09;
            case 0x0A: goto OP_0A;
            case 0x0B: goto OP_0B;
            case 0x0C: goto OP_0C;
            case 0x0D: goto OP_0D;
            case 0x0E: goto OP_0E;
            case 0x0F: goto OP_0F;
            case 0x10: goto OP_10;
            case 0x11: goto OP_11;
            case 0x12: goto OP_12;
            case 0x13: goto OP_13;
            case 0x14: goto OP_14;
            case 0x15: goto OP_15;
            case 0x16: goto OP_16;
            case 0x17: goto OP_17;
            case 0x18: goto OP_18;
            case 0x19: goto OP_19;
            case 0x1A: goto OP_1A;
            case 0x1B: goto OP_1B;
            case 0x1C: goto OP_1C;
            case 0x1D: goto OP_1D;
            case 0x1E: goto OP_1E;
            case 0x1F: goto OP_1F;
            case 0x20: goto OP_20;
            case 0x21: goto OP_21;
            case 0x22: goto OP_22;
            case 0x23: goto OP_23;
            case 0x24: goto OP_24;
            case 0x25: goto OP_25;
            case 0x26: goto OP_26;
            case 0x27: goto OP_27;
            case 0x28: goto OP_28;
            case 0x29: goto OP_29;
            case 0x2A: goto OP_2A;
            case 0x2B: goto OP_2B;
            case 0x2C: goto OP_2C;
            case 0x2D: goto OP_2D;
            case 0x2E: goto OP_2E;
            case 0x2F: goto OP_2F;
            case 0x30: goto OP_30;
            case 0x31: goto OP_31;
            case 0x32: goto OP_32;
            case 0x33: goto OP_33;
            case 0x34: goto OP_34;
            case 0x35: goto OP_35;
            case 0x36: goto OP_36;
            case 0x37: goto OP_37;
            case 0x38: goto OP_38;
            case 0x39: goto OP_39;
            case 0x3A: goto OP_3A;
            case 0x3B: goto OP_3B;
            case 0x3C: goto OP_3C;
            case 0x3D: goto OP_3D;
            case 0x3E: goto OP_3E;
            case 0x3F: goto OP_3F;
            case 0x40: goto OP_40;
            case 0x41: goto OP_41;
            case 0x42: goto OP_42;
            case 0x43: goto OP_43;
            case 0x44: goto OP_44;
            case 0x45: goto OP_45;
            case 0x46: goto OP_46;
            case 0x47: goto OP_47;
            case 0x48: goto OP_48;
            case 0x49: goto OP_49;
            case 0x4A: goto OP_4A;
            case 0x4B: goto OP_4B;
            case 0x4C: goto OP_4C;
            case 0x4D: goto OP_4D;
            case 0x4E: goto OP_4E;
            case 0x4F: goto OP_4F;
            case 0x50: goto OP_50;
            case 0x51: goto OP_51;
            case 0x52: goto OP_52;
            case 0x53: goto OP_53;
            case 0x54: goto OP_54;
            case 0x55: goto OP_55;
            case 0x56: goto OP_56;
            case 0x57: goto OP_57;
            case 0x58: goto OP_58;
            case 0x59: goto OP_59;
            case 0x5A: goto OP_5A;
            case 0x5B: goto OP_5B;
            case 0x5C: goto OP_5C;
            case 0x5D: goto OP_5D;
            case 0x5E: goto OP_5E;
            case 0x5F: goto OP_5F;
            case 0x60: goto OP_60;
            case 0x61: goto OP_61;
            case 0x62: goto OP_62;
            case 0x63: goto OP_63;
            case 0x64: goto OP_64;
            case 0x65: goto OP_65;
            case 0x66: goto OP_66;
            case 0x67: goto OP_67;
            case 0x68: goto OP_68;
            case 0x69: goto OP_69;
            case 0x6A: goto OP_6A;
            case 0x6B: goto OP_6B;
            case 0x6C: goto OP_6C;
            case 0x6D: goto OP_6D;
            case 0x6E: goto OP_6E;
            case 0x6F: goto OP_6F;
            case 0x70: goto OP_70;
            case 0x71: goto OP_71;
            case 0x72: goto OP_72;
            case 0x73: goto OP_73;
            case 0x74: goto OP_74;
            case 0x75: goto OP_75;
            case 0x76: goto OP_76;
            case 0x77: goto OP_77;
            case 0x78: goto OP_78;
            case 0x79: goto OP_79;
            case 0x7A: goto OP_7A;
            case 0x7B: goto OP_7B;
            case 0x7C: goto OP_7C;
            case 0x7D: goto OP_7D;
            case 0x7E: goto OP_7E;
            case 0x7F: goto OP_7F;
            case 0x80: goto OP_80;
            case 0x81: goto OP_81;
            case 0x82: goto OP_82;
            case 0x83: goto OP_83;
            case 0x84: goto OP_84;
            case 0x85: goto OP_85;
            case 0x86: goto OP_86;
            case 0x87: goto OP_87;
            case 0x88: goto OP_88;
            case 0x89: goto OP_89;
            case 0x8A: goto OP_8A;
            case 0x8B: goto OP_8B;
            case 0x8C: goto OP_8C;
            case 0x8D: goto OP_8D;
            case 0x8E: goto OP_8E;
            case 0x8F: goto OP_8F;
            case 0x90: goto OP_90;
            case 0x91: goto OP_91;
            case 0x92: goto OP_92;
            case 0x93: goto OP_93;
            case 0x94: goto OP_94;
            case 0x95: goto OP_95;
            case 0x96: goto OP_96;
            case 0x97: goto OP_97;
            case 0x98: goto OP_98;
            case 0x99: goto OP_99;
            case 0x9A: goto OP_9A;
            case 0x9B: goto OP_9B;
            case 0x9C: goto OP_9C;
            case 0x9D: goto OP_9D;
            case 0x9E: goto OP_9E;
            case 0x9F: goto OP_9F;
            case 0xA0: goto OP_A0;
            case 0xA1: goto OP_A1;
            case 0xA2: goto OP_A2;
            case 0xA3: goto OP_A3;
            case 0xA4: goto OP_A4;
            case 0xA5: goto OP_A5;
            case 0xA6: goto OP_A6;
            case 0xA7: goto OP_A7;
            case 0xA8: goto OP_A8;
            case 0xA9: goto OP_A9;
            case 0xAA: goto OP_AA;
            case 0xAB: goto OP_AB;
            case 0xAC: goto OP_AC;
            case 0xAD: goto OP_AD;
            case 0xAE: goto OP_AE;
            case 0xAF: goto OP_AF;
            case 0xB0: goto OP_B0;
            case 0xB1: goto OP_B1;
            case 0xB2: goto OP_B2;
            case 0xB3: goto OP_B3;
            case 0xB4: goto OP_B4;
            case 0xB5: goto OP_B5;
            case 0xB6: goto OP_B6;
            case 0xB7: goto OP_B7;
            case 0xB8: goto OP_B8;
            case 0xB9: goto OP_B9;
            case 0xBA: goto OP_BA;
            case 0xBB: goto OP_BB;
            case 0xBC: goto OP_BC;
            case 0xBD: goto OP_BD;
            case 0xBE: goto OP_BE;
            case 0xBF: goto OP_BF;
            case 0xC0: goto OP_C0;
            case 0xC1: goto OP_C1;
            case 0xC2: goto OP_C2;
            case 0xC3: goto OP_C3;
            case 0xC4: goto OP_C4;
            case 0xC5: goto OP_C5;
            case 0xC6: goto OP_C6;
            case 0xC7: goto OP_C7;
            case 0xC8: goto OP_C8;
            case 0xC9: goto OP_C9;
            case 0xCA: goto OP_CA;
            case 0xCB: goto OP_CB;
            case 0xCC: goto OP_CC;
            case 0xCD: goto OP_CD;
            case 0xCE: goto OP_CE;
            case 0xCF: goto OP_CF;
            case 0xD0: goto OP_D0;
            case 0xD1: goto OP_D1;
            case 0xD2: goto OP_D2;
            case 0xD3: goto OP_D3;
            case 0xD4: goto OP_D4;
            case 0xD5: goto OP_D5;
            case 0xD6: goto OP_D6;
            case 0xD7: goto OP_D7;
            case 0xD8: goto OP_D8;
            case 0xD9: goto OP_D9;
            case 0xDA: goto OP_DA;
            case 0xDB: goto OP_DB;
            case 0xDC: goto OP_DC;
            case 0xDD: goto OP_DD;
            case 0xDE: goto OP_DE;
            case 0xDF: goto OP_DF;
            case 0xE0: goto OP_E0;
            case 0xE1: goto OP_E1;
            case 0xE2: goto OP_E2;
            case 0xE3: goto OP_E3;
            case 0xE4: goto OP_E4;
            case 0xE5: goto OP_E5;
            case 0xE6: goto OP_E6;
            case 0xE7: goto OP_E7;
            case 0xE8: goto OP_E8;
            case 0xE9: goto OP_E9;
            case 0xEA: goto OP_EA;
            case 0xEB: goto OP_EB;
            case 0xEC: goto OP_EC;
            case 0xED: goto OP_ED;
            case 0xEE: goto OP_EE;
            case 0xEF: goto OP_EF;
            case 0xF0: goto OP_F0;
            case 0xF1: goto OP_F1;
            case 0xF2: goto OP_F2;
            case 0xF3: goto OP_F3;
            case 0xF4: goto OP_F4;
            case 0xF5: goto OP_F5;
            case 0xF6: goto OP_F6;
            case 0xF7: goto OP_F7;
            case 0xF8: goto OP_F8;
            case 0xF9: goto OP_F9;
            case 0xFA: goto OP_FA;
            case 0xFB: goto OP_FB;
            case 0xFC: goto OP_FC;
            case 0xFD: goto OP_FD;
            case 0xFE: goto OP_FE;
            case 0xFF: goto OP_FF;
        }
#endif

    OP_DD:
        mode = 1;
        goto DISPATCH;
    OP_FD:
        mode = 2;
        goto DISPATCH;

    OP_00: { EXEC_OP(0x00); continue; }
    OP_01: { EXEC_OP(0x01); continue; }
    OP_02: { EXEC_OP(0x02); continue; }
    OP_03: { EXEC_OP(0x03); continue; }
    OP_04: { EXEC_OP(0x04); continue; }
    OP_05: { EXEC_OP(0x05); continue; }
    OP_06: { EXEC_OP(0x06); continue; }
    OP_07: { EXEC_OP(0x07); continue; }
    OP_08: { EXEC_OP(0x08); continue; }
    OP_09: { EXEC_OP(0x09); continue; }
    OP_0A: { EXEC_OP(0x0A); continue; }
    OP_0B: { EXEC_OP(0x0B); continue; }
    OP_0C: { EXEC_OP(0x0C); continue; }
    OP_0D: { EXEC_OP(0x0D); continue; }
    OP_0E: { EXEC_OP(0x0E); continue; }
    OP_0F: { EXEC_OP(0x0F); continue; }
    OP_10: { EXEC_OP(0x10); continue; }
    OP_11: { EXEC_OP(0x11); continue; }
    OP_12: { EXEC_OP(0x12); continue; }
    OP_13: { EXEC_OP(0x13); continue; }
    OP_14: { EXEC_OP(0x14); continue; }
    OP_15: { EXEC_OP(0x15); continue; }
    OP_16: { EXEC_OP(0x16); continue; }
    OP_17: { EXEC_OP(0x17); continue; }
    OP_18: { EXEC_OP(0x18); continue; }
    OP_19: { EXEC_OP(0x19); continue; }
    OP_1A: { EXEC_OP(0x1A); continue; }
    OP_1B: { EXEC_OP(0x1B); continue; }
    OP_1C: { EXEC_OP(0x1C); continue; }
    OP_1D: { EXEC_OP(0x1D); continue; }
    OP_1E: { EXEC_OP(0x1E); continue; }
    OP_1F: { EXEC_OP(0x1F); continue; }
    OP_20: { EXEC_OP(0x20); continue; }
    OP_21: { EXEC_OP(0x21); continue; }
    OP_22: { EXEC_OP(0x22); continue; }
    OP_23: { EXEC_OP(0x23); continue; }
    OP_24: { EXEC_OP(0x24); continue; }
    OP_25: { EXEC_OP(0x25); continue; }
    OP_26: { EXEC_OP(0x26); continue; }
    OP_27: { EXEC_OP(0x27); continue; }
    OP_28: { EXEC_OP(0x28); continue; }
    OP_29: { EXEC_OP(0x29); continue; }
    OP_2A: { EXEC_OP(0x2A); continue; }
    OP_2B: { EXEC_OP(0x2B); continue; }
    OP_2C: { EXEC_OP(0x2C); continue; }
    OP_2D: { EXEC_OP(0x2D); continue; }
    OP_2E: { EXEC_OP(0x2E); continue; }
    OP_2F: { EXEC_OP(0x2F); continue; }
    OP_30: { EXEC_OP(0x30); continue; }
    OP_31: { EXEC_OP(0x31); continue; }
    OP_32: { EXEC_OP(0x32); continue; }
    OP_33: { EXEC_OP(0x33); continue; }
    OP_34: { EXEC_OP(0x34); continue; }
    OP_35: { EXEC_OP(0x35); continue; }
    OP_36: { EXEC_OP(0x36); continue; }
    OP_37: { EXEC_OP(0x37); continue; }
    OP_38: { EXEC_OP(0x38); continue; }
    OP_39: { EXEC_OP(0x39); continue; }
    OP_3A: { EXEC_OP(0x3A); continue; }
    OP_3B: { EXEC_OP(0x3B); continue; }
    OP_3C: { EXEC_OP(0x3C); continue; }
    OP_3D: { EXEC_OP(0x3D); continue; }
    OP_3E: { EXEC_OP(0x3E); continue; }
    OP_3F: { EXEC_OP(0x3F); continue; }
    OP_40: { EXEC_OP(0x40); continue; }
    OP_41: { EXEC_OP(0x41); continue; }
    OP_42: { EXEC_OP(0x42); continue; }
    OP_43: { EXEC_OP(0x43); continue; }
    OP_44: { EXEC_OP(0x44); continue; }
    OP_45: { EXEC_OP(0x45); continue; }
    OP_46: { EXEC_OP(0x46); continue; }
    OP_47: { EXEC_OP(0x47); continue; }
    OP_48: { EXEC_OP(0x48); continue; }
    OP_49: { EXEC_OP(0x49); continue; }
    OP_4A: { EXEC_OP(0x4A); continue; }
    OP_4B: { EXEC_OP(0x4B); continue; }
    OP_4C: { EXEC_OP(0x4C); continue; }
    OP_4D: { EXEC_OP(0x4D); continue; }
    OP_4E: { EXEC_OP(0x4E); continue; }
    OP_4F: { EXEC_OP(0x4F); continue; }
    OP_50: { EXEC_OP(0x50); continue; }
    OP_51: { EXEC_OP(0x51); continue; }
    OP_52: { EXEC_OP(0x52); continue; }
    OP_53: { EXEC_OP(0x53); continue; }
    OP_54: { EXEC_OP(0x54); continue; }
    OP_55: { EXEC_OP(0x55); continue; }
    OP_56: { EXEC_OP(0x56); continue; }
    OP_57: { EXEC_OP(0x57); continue; }
    OP_58: { EXEC_OP(0x58); continue; }
    OP_59: { EXEC_OP(0x59); continue; }
    OP_5A: { EXEC_OP(0x5A); continue; }
    OP_5B: { EXEC_OP(0x5B); continue; }
    OP_5C: { EXEC_OP(0x5C); continue; }
    OP_5D: { EXEC_OP(0x5D); continue; }
    OP_5E: { EXEC_OP(0x5E); continue; }
    OP_5F: { EXEC_OP(0x5F); continue; }
    OP_60: { EXEC_OP(0x60); continue; }
    OP_61: { EXEC_OP(0x61); continue; }
    OP_62: { EXEC_OP(0x62); continue; }
    OP_63: { EXEC_OP(0x63); continue; }
    OP_64: { EXEC_OP(0x64); continue; }
    OP_65: { EXEC_OP(0x65); continue; }
    OP_66: { EXEC_OP(0x66); continue; }
    OP_67: { EXEC_OP(0x67); continue; }
    OP_68: { EXEC_OP(0x68); continue; }
    OP_69: { EXEC_OP(0x69); continue; }
    OP_6A: { EXEC_OP(0x6A); continue; }
    OP_6B: { EXEC_OP(0x6B); continue; }
    OP_6C: { EXEC_OP(0x6C); continue; }
    OP_6D: { EXEC_OP(0x6D); continue; }
    OP_6E: { EXEC_OP(0x6E); continue; }
    OP_6F: { EXEC_OP(0x6F); continue; }
    OP_70: { EXEC_OP(0x70); continue; }
    OP_71: { EXEC_OP(0x71); continue; }
    OP_72: { EXEC_OP(0x72); continue; }
    OP_73: { EXEC_OP(0x73); continue; }
    OP_74: { EXEC_OP(0x74); continue; }
    OP_75: { EXEC_OP(0x75); continue; }
    OP_76: { EXEC_OP(0x76); continue; }
    OP_77: { EXEC_OP(0x77); continue; }
    OP_78: { EXEC_OP(0x78); continue; }
    OP_79: { EXEC_OP(0x79); continue; }
    OP_7A: { EXEC_OP(0x7A); continue; }
    OP_7B: { EXEC_OP(0x7B); continue; }
    OP_7C: { EXEC_OP(0x7C); continue; }
    OP_7D: { EXEC_OP(0x7D); continue; }
    OP_7E: { EXEC_OP(0x7E); continue; }
    OP_7F: { EXEC_OP(0x7F); continue; }
    OP_80: { EXEC_OP(0x80); continue; }
    OP_81: { EXEC_OP(0x81); continue; }
    OP_82: { EXEC_OP(0x82); continue; }
    OP_83: { EXEC_OP(0x83); continue; }
    OP_84: { EXEC_OP(0x84); continue; }
    OP_85: { EXEC_OP(0x85); continue; }
    OP_86: { EXEC_OP(0x86); continue; }
    OP_87: { EXEC_OP(0x87); continue; }
    OP_88: { EXEC_OP(0x88); continue; }
    OP_89: { EXEC_OP(0x89); continue; }
    OP_8A: { EXEC_OP(0x8A); continue; }
    OP_8B: { EXEC_OP(0x8B); continue; }
    OP_8C: { EXEC_OP(0x8C); continue; }
    OP_8D: { EXEC_OP(0x8D); continue; }
    OP_8E: { EXEC_OP(0x8E); continue; }
    OP_8F: { EXEC_OP(0x8F); continue; }
    OP_90: { EXEC_OP(0x90); continue; }
    OP_91: { EXEC_OP(0x91); continue; }
    OP_92: { EXEC_OP(0x92); continue; }
    OP_93: { EXEC_OP(0x93); continue; }
    OP_94: { EXEC_OP(0x94); continue; }
    OP_95: { EXEC_OP(0x95); continue; }
    OP_96: { EXEC_OP(0x96); continue; }
    OP_97: { EXEC_OP(0x97); continue; }
    OP_98: { EXEC_OP(0x98); continue; }
    OP_99: { EXEC_OP(0x99); continue; }
    OP_9A: { EXEC_OP(0x9A); continue; }
    OP_9B: { EXEC_OP(0x9B); continue; }
    OP_9C: { EXEC_OP(0x9C); continue; }
    OP_9D: { EXEC_OP(0x9D); continue; }
    OP_9E: { EXEC_OP(0x9E); continue; }
    OP_9F: { EXEC_OP(0x9F); continue; }
    OP_A0: { EXEC_OP(0xA0); continue; }
    OP_A1: { EXEC_OP(0xA1); continue; }
    OP_A2: { EXEC_OP(0xA2); continue; }
    OP_A3: { EXEC_OP(0xA3); continue; }
    OP_A4: { EXEC_OP(0xA4); continue; }
    OP_A5: { EXEC_OP(0xA5); continue; }
    OP_A6: { EXEC_OP(0xA6); continue; }
    OP_A7: { EXEC_OP(0xA7); continue; }
    OP_A8: { EXEC_OP(0xA8); continue; }
    OP_A9: { EXEC_OP(0xA9); continue; }
    OP_AA: { EXEC_OP(0xAA); continue; }
    OP_AB: { EXEC_OP(0xAB); continue; }
    OP_AC: { EXEC_OP(0xAC); continue; }
    OP_AD: { EXEC_OP(0xAD); continue; }
    OP_AE: { EXEC_OP(0xAE); continue; }
    OP_AF: { EXEC_OP(0xAF); continue; }
    OP_B0: { EXEC_OP(0xB0); continue; }
    OP_B1: { EXEC_OP(0xB1); continue; }
    OP_B2: { EXEC_OP(0xB2); continue; }
    OP_B3: { EXEC_OP(0xB3); continue; }
    OP_B4: { EXEC_OP(0xB4); continue; }
    OP_B5: { EXEC_OP(0xB5); continue; }
    OP_B6: { EXEC_OP(0xB6); continue; }
    OP_B7: { EXEC_OP(0xB7); continue; }
    OP_B8: { EXEC_OP(0xB8); continue; }
    OP_B9: { EXEC_OP(0xB9); continue; }
    OP_BA: { EXEC_OP(0xBA); continue; }
    OP_BB: { EXEC_OP(0xBB); continue; }
    OP_BC: { EXEC_OP(0xBC); continue; }
    OP_BD: { EXEC_OP(0xBD); continue; }
    OP_BE: { EXEC_OP(0xBE); continue; }
    OP_BF: { EXEC_OP(0xBF); continue; }
    OP_C0: { EXEC_OP(0xC0); continue; }
    OP_C1: { EXEC_OP(0xC1); continue; }
    OP_C2: { EXEC_OP(0xC2); continue; }
    OP_C3: { EXEC_OP(0xC3); continue; }
    OP_C4: { EXEC_OP(0xC4); continue; }
    OP_C5: { EXEC_OP(0xC5); continue; }
    OP_C6: { EXEC_OP(0xC6); continue; }
    OP_C7: { EXEC_OP(0xC7); continue; }
    OP_C8: { EXEC_OP(0xC8); continue; }
    OP_C9: { EXEC_OP(0xC9); continue; }
    OP_CA: { EXEC_OP(0xCA); continue; }
    OP_CB: { EXEC_OP(0xCB); continue; }
    OP_CC: { EXEC_OP(0xCC); continue; }
    OP_CD: { EXEC_OP(0xCD); continue; }
    OP_CE: { EXEC_OP(0xCE); continue; }
    OP_CF: { EXEC_OP(0xCF); continue; }
    OP_D0: { EXEC_OP(0xD0); continue; }
    OP_D1: { EXEC_OP(0xD1); continue; }
    OP_D2: { EXEC_OP(0xD2); continue; }
    OP_D3: { EXEC_OP(0xD3); continue; }
    OP_D4: { EXEC_OP(0xD4); continue; }
    OP_D5: { EXEC_OP(0xD5); continue; }
    OP_D6: { EXEC_OP(0xD6); continue; }
    OP_D7: { EXEC_OP(0xD7); continue; }
    OP_D8: { EXEC_OP(0xD8); continue; }
    OP_D9: { EXEC_OP(0xD9); continue; }
    OP_DA: { EXEC_OP(0xDA); continue; }
    OP_DB: { EXEC_OP(0xDB); continue; }
    OP_DC: { EXEC_OP(0xDC); continue; }
    OP_DE: { EXEC_OP(0xDE); continue; }
    OP_DF: { EXEC_OP(0xDF); continue; }
    OP_E0: { EXEC_OP(0xE0); continue; }
    OP_E1: { EXEC_OP(0xE1); continue; }
    OP_E2: { EXEC_OP(0xE2); continue; }
    OP_E3: { EXEC_OP(0xE3); continue; }
    OP_E4: { EXEC_OP(0xE4); continue; }
    OP_E5: { EXEC_OP(0xE5); continue; }
    OP_E6: { EXEC_OP(0xE6); continue; }
    OP_E7: { EXEC_OP(0xE7); continue; }
    OP_E8: { EXEC_OP(0xE8); continue; }
    OP_E9: { EXEC_OP(0xE9); continue; }
    OP_EA: { EXEC_OP(0xEA); continue; }
    OP_EB: { EXEC_OP(0xEB); continue; }
    OP_EC: { EXEC_OP(0xEC); continue; }
    OP_ED: { EXEC_OP(0xED); continue; }
    OP_EE: { EXEC_OP(0xEE); continue; }
    OP_EF: { EXEC_OP(0xEF); continue; }
    OP_F0: { EXEC_OP(0xF0); continue; }
    OP_F1: { EXEC_OP(0xF1); continue; }
    OP_F2: { EXEC_OP(0xF2); continue; }
    OP_F3: { EXEC_OP(0xF3); continue; }
    OP_F4: { EXEC_OP(0xF4); continue; }
    OP_F5: { EXEC_OP(0xF5); continue; }
    OP_F6: { EXEC_OP(0xF6); continue; }
    OP_F7: { EXEC_OP(0xF7); continue; }
    OP_F8: { EXEC_OP(0xF8); continue; }
    OP_F9: { EXEC_OP(0xF9); continue; }
    OP_FA: { EXEC_OP(0xFA); continue; }
    OP_FB: { EXEC_OP(0xFB); continue; }
    OP_FC: { EXEC_OP(0xFC); continue; }
    OP_FE: { EXEC_OP(0xFE); continue; }
    OP_FF: { EXEC_OP(0xFF); continue; }
    }
}
#endif
