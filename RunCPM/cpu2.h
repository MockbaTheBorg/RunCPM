#ifndef CPU_H
#define CPU_H

/* Model 2 - Smaller code, optimized */

#define CPU_IS "Model 2"

int32 PCX; /* external view of PC                          */
int32 AF;  /* AF register                                  */
int32 BC;  /* BC register                                  */
int32 DE;  /* DE register                                  */
int32 HL;  /* HL register                                  */
int32 IX;  /* IX register                                  */
int32 IY;  /* IY register                                  */
int32 PC;  /* program counter                              */
int32 SP;  /* SP register                                  */
int32 AF1; /* alternate AF register                        */
int32 BC1; /* alternate BC register                        */
int32 DE1; /* alternate DE register                        */
int32 HL1; /* alternate HL register                        */
int32 IFF; /* Interrupt Flip Flop                          */
int32 IR;  /* Interrupt (upper) / Refresh (lower) register */
int32 Status = STATUS_RUNNING; /* Status of the CPU 0=running 1=end request 2=back to CCP */
int32 Debug = 0;
int32 Step = -1;

#ifdef iDEBUG
FILE* iLogFile;
char iLogBuffer[256];
const char* iLogTxt;
#endif

/* increase R by val (to correctly implement refresh counter) if enabled */
#ifdef DO_INCR
#define INCR(val) IR = (IR & ~0x3f) | ((IR + (val)) & 0x3f)
#else
#define INCR(val) ;
#endif

/*
	Functions needed by the soft CPU implementation
*/
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

/* Z80 Custom soft core */

#define ADDRMASK        0xffff

#define FLAG_C  1
#define FLAG_N  2
#define FLAG_P  4
#define FLAG_H  16
#define FLAG_Z  64
#define FLAG_S  128

#define SETFLAG(f,c)    (AF = (c) ? AF | FLAG_ ## f : AF & ~FLAG_ ## f)
#define TSTFLAG(f)      ((AF & FLAG_ ## f) != 0)

#define SET_PVS(s)  (((cbits >> 6) ^ (cbits >> 5)) & 4)
#define SET_PV      (SET_PVS(sum))

#define POP(x)  {                               \
    uint32 y = RAM_PP(SP);                      \
    x = y + (RAM_PP(SP) << 8);                  \
}

#define JPC(cond) {                             \
    if (cond) {                                 \
        PC = GET_WORD(PC);                      \
    } else {                                    \
		PC++;                                   \
        PC++;                                   \
    }                                           \
}

#define CALLC(cond) {                           \
    if (cond) {                                 \
        uint32 a = GET_WORD(PC);                \
        PUSH(PC + 2);                           \
        PC = a;                                 \
    } else {                                    \
		PC++;                                   \
        PC++;                                   \
    }                                           \
}

// the following tables precompute some common subexpressions
static uint8 parityTable[256];
static uint8 cbitsTable[512];
static uint16 rrcaTable[256];
static uint16 xororTable[256];
static uint8 rotateShiftTable[256];
static uint8 incZ80Table[257];
static uint8 decZ80Table[256];
static uint8 cbitsZ80Table[512];
static uint8 cbitsZ80DupTable[512];
static uint8 cbits2Z80Table[512];
static uint8 cbits2Z80DupTable[512];
static uint8 negTable[256];
static uint8 cpTable[256];

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
		decZ80Table[i] = (i & 0xa8) | (((i & 0xff) == 0) << 6) | (((i & 0xf) == 0xf) << 4) | ((i == 0x7f) << 2) | 2;
		negTable[i] = (((i & 0x0f) != 0) << 4) | ((i == 0x80) << 2) | 2 | (i != 0);
		cpTable[i] = (i & 0x80) | (((i & 0xff) == 0) << 6);
	}
	// 257 bytes tables
	for (int i = 0; i < 257; i++) {
		incZ80Table[i] = (i & 0xa8) | (((i & 0xff) == 0) << 6) | (((i & 0xf) == 0) << 4) | ((i == 0x80) << 2);
	}
	// 512 bytes tables
	for (int i = 0; i < 512; i++) {
		cbitsTable[i] = (i & 0x10) | ((i >> 8) & 1);
		cbitsZ80Table[i] = (i & 0x10) | (((i >> 6) ^ (i >> 5)) & 4) | ((i >> 8) & 1);
		cbitsZ80DupTable[i] = (i & 0x10) | (((i >> 6) ^ (i >> 5)) & 4) | ((i >> 8) & 1) | (i & 0xa8);
		cbits2Z80Table[i] = (i & 0x10) | (((i >> 6) ^ (i >> 5)) & 4) | ((i >> 8) & 1) | 2;
		cbits2Z80DupTable[i] = (i & 0x10) | (((i >> 6) ^ (i >> 5)) & 4) | ((i >> 8) & 1) | 2 | (i & 0xa8);
	}
}

#if defined(DEBUG) || defined(iDEBUG)
static const char* Mnemonics[256] =
{
	"NOP", "LD BC,#h", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,*h", "RLCA",
	"EX AF,AF'", "ADD HL,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,*h", "RRCA",
	"DJNZ @h", "LD DE,#h", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,*h", "RLA",
	"JR @h", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,*h", "RRA",
	"JR NZ,@h", "LD HL,#h", "LD (#h),HL", "INC HL", "INC H", "DEC H", "LD H,*h", "DAA",
	"JR Z,@h", "ADD HL,HL", "LD HL,(#h)", "DEC HL", "INC L", "DEC L", "LD L,*h", "CPL",
	"JR NC,@h", "LD SP,#h", "LD (#h),A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL),*h", "SCF",
	"JR C,@h", "ADD HL,SP", "LD A,(#h)", "DEC SP", "INC A", "DEC A", "LD A,*h", "CCF",
	"LD B,B", "LD B,C", "LD B,D", "LD B,E", "LD B,H", "LD B,L", "LD B,(HL)", "LD B,A",
	"LD C,B", "LD C,C", "LD C,D", "LD C,E", "LD C,H", "LD C,L", "LD C,(HL)", "LD C,A",
	"LD D,B", "LD D,C", "LD D,D", "LD D,E", "LD D,H", "LD D,L", "LD D,(HL)", "LD D,A",
	"LD E,B", "LD E,C", "LD E,D", "LD E,E", "LD E,H", "LD E,L", "LD E,(HL)", "LD E,A",
	"LD H,B", "LD H,C", "LD H,D", "LD H,E", "LD H,H", "LD H,L", "LD H,(HL)", "LD H,A",
	"LD L,B", "LD L,C", "LD L,D", "LD L,E", "LD L,H", "LD L,L", "LD L,(HL)", "LD L,A",
	"LD (HL),B", "LD (HL),C", "LD (HL),D", "LD (HL),E", "LD (HL),H", "LD (HL),L", "HALT", "LD (HL),A",
	"LD A,B", "LD A,C", "LD A,D", "LD A,E", "LD A,H", "LD A,L", "LD A,(HL)", "LD A,A",
	"ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD (HL)", "ADD A",
	"ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC (HL)", "ADC A",
	"SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB (HL)", "SUB A",
	"SBC B", "SBC C", "SBC D", "SBC E", "SBC H", "SBC L", "SBC (HL)", "SBC A",
	"AND B", "AND C", "AND D", "AND E", "AND H", "AND L", "AND (HL)", "AND A",
	"XOR B", "XOR C", "XOR D", "XOR E", "XOR H", "XOR L", "XOR (HL)", "XOR A",
	"OR B", "OR C", "OR D", "OR E", "OR H", "OR L", "OR (HL)", "OR A",
	"CP B", "CP C", "CP D", "CP E", "CP H", "CP L", "CP (HL)", "CP A",
	"RET NZ", "POP BC", "JP NZ,#h", "JP #h", "CALL NZ,#h", "PUSH BC", "ADD *h", "RST 00h",
	"RET Z", "RET", "JP Z,#h", "PFX_CB", "CALL Z,#h", "CALL #h", "ADC *h", "RST 08h",
	"RET NC", "POP DE", "JP NC,#h", "OUTA (*h)", "CALL NC,#h", "PUSH DE", "SUB *h", "RST 10h",
	"RET C", "EXX", "JP C,#h", "INA (*h)", "CALL C,#h", "PFX_DD", "SBC *h", "RST 18h",
	"RET PO", "POP HL", "JP PO,#h", "EX HL,(SP)", "CALL PO,#h", "PUSH HL", "AND *h", "RST 20h",
	"RET PE", "LD PC,HL", "JP PE,#h", "EX DE,HL", "CALL PE,#h", "PFX_ED", "XOR *h", "RST 28h",
	"RET P", "POP AF", "JP P,#h", "DI", "CALL P,#h", "PUSH AF", "OR *h", "RST 30h",
	"RET M", "LD SP,HL", "JP M,#h", "EI", "CALL M,#h", "PFX_FD", "CP *h", "RST 38h"
};

static const char* MnemonicsCB[256] =
{
	"RLC B", "RLC C", "RLC D", "RLC E", "RLC H", "RLC L", "RLC (HL)", "RLC A",
	"RRC B", "RRC C", "RRC D", "RRC E", "RRC H", "RRC L", "RRC (HL)", "RRC A",
	"RL B", "RL C", "RL D", "RL E", "RL H", "RL L", "RL (HL)", "RL A",
	"RR B", "RR C", "RR D", "RR E", "RR H", "RR L", "RR (HL)", "RR A",
	"SLA B", "SLA C", "SLA D", "SLA E", "SLA H", "SLA L", "SLA (HL)", "SLA A",
	"SRA B", "SRA C", "SRA D", "SRA E", "SRA H", "SRA L", "SRA (HL)", "SRA A",
	"SLL B", "SLL C", "SLL D", "SLL E", "SLL H", "SLL L", "SLL (HL)", "SLL A",
	"SRL B", "SRL C", "SRL D", "SRL E", "SRL H", "SRL L", "SRL (HL)", "SRL A",
	"BIT 0,B", "BIT 0,C", "BIT 0,D", "BIT 0,E", "BIT 0,H", "BIT 0,L", "BIT 0,(HL)", "BIT 0,A",
	"BIT 1,B", "BIT 1,C", "BIT 1,D", "BIT 1,E", "BIT 1,H", "BIT 1,L", "BIT 1,(HL)", "BIT 1,A",
	"BIT 2,B", "BIT 2,C", "BIT 2,D", "BIT 2,E", "BIT 2,H", "BIT 2,L", "BIT 2,(HL)", "BIT 2,A",
	"BIT 3,B", "BIT 3,C", "BIT 3,D", "BIT 3,E", "BIT 3,H", "BIT 3,L", "BIT 3,(HL)", "BIT 3,A",
	"BIT 4,B", "BIT 4,C", "BIT 4,D", "BIT 4,E", "BIT 4,H", "BIT 4,L", "BIT 4,(HL)", "BIT 4,A",
	"BIT 5,B", "BIT 5,C", "BIT 5,D", "BIT 5,E", "BIT 5,H", "BIT 5,L", "BIT 5,(HL)", "BIT 5,A",
	"BIT 6,B", "BIT 6,C", "BIT 6,D", "BIT 6,E", "BIT 6,H", "BIT 6,L", "BIT 6,(HL)", "BIT 6,A",
	"BIT 7,B", "BIT 7,C", "BIT 7,D", "BIT 7,E", "BIT 7,H", "BIT 7,L", "BIT 7,(HL)", "BIT 7,A",
	"RES 0,B", "RES 0,C", "RES 0,D", "RES 0,E", "RES 0,H", "RES 0,L", "RES 0,(HL)", "RES 0,A",
	"RES 1,B", "RES 1,C", "RES 1,D", "RES 1,E", "RES 1,H", "RES 1,L", "RES 1,(HL)", "RES 1,A",
	"RES 2,B", "RES 2,C", "RES 2,D", "RES 2,E", "RES 2,H", "RES 2,L", "RES 2,(HL)", "RES 2,A",
	"RES 3,B", "RES 3,C", "RES 3,D", "RES 3,E", "RES 3,H", "RES 3,L", "RES 3,(HL)", "RES 3,A",
	"RES 4,B", "RES 4,C", "RES 4,D", "RES 4,E", "RES 4,H", "RES 4,L", "RES 4,(HL)", "RES 4,A",
	"RES 5,B", "RES 5,C", "RES 5,D", "RES 5,E", "RES 5,H", "RES 5,L", "RES 5,(HL)", "RES 5,A",
	"RES 6,B", "RES 6,C", "RES 6,D", "RES 6,E", "RES 6,H", "RES 6,L", "RES 6,(HL)", "RES 6,A",
	"RES 7,B", "RES 7,C", "RES 7,D", "RES 7,E", "RES 7,H", "RES 7,L", "RES 7,(HL)", "RES 7,A",
	"SET 0,B", "SET 0,C", "SET 0,D", "SET 0,E", "SET 0,H", "SET 0,L", "SET 0,(HL)", "SET 0,A",
	"SET 1,B", "SET 1,C", "SET 1,D", "SET 1,E", "SET 1,H", "SET 1,L", "SET 1,(HL)", "SET 1,A",
	"SET 2,B", "SET 2,C", "SET 2,D", "SET 2,E", "SET 2,H", "SET 2,L", "SET 2,(HL)", "SET 2,A",
	"SET 3,B", "SET 3,C", "SET 3,D", "SET 3,E", "SET 3,H", "SET 3,L", "SET 3,(HL)", "SET 3,A",
	"SET 4,B", "SET 4,C", "SET 4,D", "SET 4,E", "SET 4,H", "SET 4,L", "SET 4,(HL)", "SET 4,A",
	"SET 5,B", "SET 5,C", "SET 5,D", "SET 5,E", "SET 5,H", "SET 5,L", "SET 5,(HL)", "SET 5,A",
	"SET 6,B", "SET 6,C", "SET 6,D", "SET 6,E", "SET 6,H", "SET 6,L", "SET 6,(HL)", "SET 6,A",
	"SET 7,B", "SET 7,C", "SET 7,D", "SET 7,E", "SET 7,H", "SET 7,L", "SET 7,(HL)", "SET 7,A"
};

static const char* MnemonicsED[256] =
{
	"DB EDh,00h", "DB EDh,01h", "DB EDh,02h", "DB EDh,03h",
	"DB EDh,04h", "DB EDh,05h", "DB EDh,06h", "DB EDh,07h",
	"DB EDh,08h", "DB EDh,09h", "DB EDh,0Ah", "DB EDh,0Bh",
	"DB EDh,0Ch", "DB EDh,0Dh", "DB EDh,0Eh", "DB EDh,0Fh",
	"DB EDh,10h", "DB EDh,11h", "DB EDh,12h", "DB EDh,13h",
	"DB EDh,14h", "DB EDh,15h", "DB EDh,16h", "DB EDh,17h",
	"DB EDh,18h", "DB EDh,19h", "DB EDh,1Ah", "DB EDh,1Bh",
	"DB EDh,1Ch", "DB EDh,1Dh", "DB EDh,1Eh", "DB EDh,1Fh",
	"DB EDh,20h", "DB EDh,21h", "DB EDh,22h", "DB EDh,23h",
	"DB EDh,24h", "DB EDh,25h", "DB EDh,26h", "DB EDh,27h",
	"DB EDh,28h", "DB EDh,29h", "DB EDh,2Ah", "DB EDh,2Bh",
	"DB EDh,2Ch", "DB EDh,2Dh", "DB EDh,2Eh", "DB EDh,2Fh",
	"DB EDh,30h", "DB EDh,31h", "DB EDh,32h", "DB EDh,33h",
	"DB EDh,34h", "DB EDh,35h", "DB EDh,36h", "DB EDh,37h",
	"DB EDh,38h", "DB EDh,39h", "DB EDh,3Ah", "DB EDh,3Bh",
	"DB EDh,3Ch", "DB EDh,3Dh", "DB EDh,3Eh", "DB EDh,3Fh",
	"IN B,(C)", "OUT (C),B", "SBC HL,BC", "LD (#h),BC",
	"NEG", "RETN", "IM 0", "LD I,A",
	"IN C,(C)", "OUT (C),C", "ADC HL,BC", "LD BC,(#h)",
	"DB EDh,4Ch", "RETI", "DB EDh,4Eh", "LD R,A",
	"IN D,(C)", "OUT (C),D", "SBC HL,DE", "LD (#h),DE",
	"DB EDh,54h", "DB EDh,55h", "IM 1", "LD A,I",
	"IN E,(C)", "OUT (C),E", "ADC HL,DE", "LD DE,(#h)",
	"DB EDh,5Ch", "DB EDh,5Dh", "IM 2", "LD A,R",
	"IN H,(C)", "OUT (C),H", "SBC HL,HL", "LD (#h),HL",
	"DB EDh,64h", "DB EDh,65h", "DB EDh,66h", "RRD",
	"IN L,(C)", "OUT (C),L", "ADC HL,HL", "LD HL,(#h)",
	"DB EDh,6Ch", "DB EDh,6Dh", "DB EDh,6Eh", "RLD",
	"IN F,(C)", "DB EDh,71h", "SBC HL,SP", "LD (#h),SP",
	"DB EDh,74h", "DB EDh,75h", "DB EDh,76h", "DB EDh,77h",
	"IN A,(C)", "OUT (C),A", "ADC HL,SP", "LD SP,(#h)",
	"DB EDh,7Ch", "DB EDh,7Dh", "DB EDh,7Eh", "DB EDh,7Fh",
	"DB EDh,80h", "DB EDh,81h", "DB EDh,82h", "DB EDh,83h",
	"DB EDh,84h", "DB EDh,85h", "DB EDh,86h", "DB EDh,87h",
	"DB EDh,88h", "DB EDh,89h", "DB EDh,8Ah", "DB EDh,8Bh",
	"DB EDh,8Ch", "DB EDh,8Dh", "DB EDh,8Eh", "DB EDh,8Fh",
	"DB EDh,90h", "DB EDh,91h", "DB EDh,92h", "DB EDh,93h",
	"DB EDh,94h", "DB EDh,95h", "DB EDh,96h", "DB EDh,97h",
	"DB EDh,98h", "DB EDh,99h", "DB EDh,9Ah", "DB EDh,9Bh",
	"DB EDh,9Ch", "DB EDh,9Dh", "DB EDh,9Eh", "DB EDh,9Fh",
	"LDI", "CPI", "INI", "OUTI",
	"DB EDh,A4h", "DB EDh,A5h", "DB EDh,A6h", "DB EDh,A7h",
	"LDD", "CPD", "IND", "OUTD",
	"DB EDh,ACh", "DB EDh,ADh", "DB EDh,AEh", "DB EDh,AFh",
	"LDIR", "CPIR", "INIR", "OTIR",
	"DB EDh,B4h", "DB EDh,B5h", "DB EDh,B6h", "DB EDh,B7h",
	"LDDR", "CPDR", "INDR", "OTDR",
	"DB EDh,BCh", "DB EDh,BDh", "DB EDh,BEh", "DB EDh,BFh",
	"DB EDh,C0h", "DB EDh,C1h", "DB EDh,C2h", "DB EDh,C3h",
	"DB EDh,C4h", "DB EDh,C5h", "DB EDh,C6h", "DB EDh,C7h",
	"DB EDh,C8h", "DB EDh,C9h", "DB EDh,CAh", "DB EDh,CBh",
	"DB EDh,CCh", "DB EDh,CDh", "DB EDh,CEh", "DB EDh,CFh",
	"DB EDh,D0h", "DB EDh,D1h", "DB EDh,D2h", "DB EDh,D3h",
	"DB EDh,D4h", "DB EDh,D5h", "DB EDh,D6h", "DB EDh,D7h",
	"DB EDh,D8h", "DB EDh,D9h", "DB EDh,DAh", "DB EDh,DBh",
	"DB EDh,DCh", "DB EDh,DDh", "DB EDh,DEh", "DB EDh,DFh",
	"DB EDh,E0h", "DB EDh,E1h", "DB EDh,E2h", "DB EDh,E3h",
	"DB EDh,E4h", "DB EDh,E5h", "DB EDh,E6h", "DB EDh,E7h",
	"DB EDh,E8h", "DB EDh,E9h", "DB EDh,EAh", "DB EDh,EBh",
	"DB EDh,ECh", "DB EDh,EDh", "DB EDh,EEh", "DB EDh,EFh",
	"DB EDh,F0h", "DB EDh,F1h", "DB EDh,F2h", "DB EDh,F3h",
	"DB EDh,F4h", "DB EDh,F5h", "DB EDh,F6h", "DB EDh,F7h",
	"DB EDh,F8h", "DB EDh,F9h", "DB EDh,FAh", "DB EDh,FBh",
	"DB EDh,FCh", "DB EDh,FDh", "DB EDh,FEh", "DB EDh,FFh"
};

static const char* MnemonicsXX[256] =
{
	"NOP", "LD BC,#h", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,*h", "RLCA",
	"EX AF,AF'", "ADD I%,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,*h", "RRCA",
	"DJNZ @h", "LD DE,#h", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,*h", "RLA",
	"JR @h", "ADD I%,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,*h", "RRA",
	"JR NZ,@h", "LD I%,#h", "LD (#h),I%", "INC I%", "INC I%h", "DEC I%h", "LD I%h,*h", "DAA",
	"JR Z,@h", "ADD I%,I%", "LD I%,(#h)", "DEC I%", "INC I%l", "DEC I%l", "LD I%l,*h", "CPL",
	"JR NC,@h", "LD SP,#h", "LD (#h),A", "INC SP", "INC (I%+^h)", "DEC (I%+^h)", "LD (I%+^h),*h", "SCF",
	"JR C,@h", "ADD I%,SP", "LD A,(#h)", "DEC SP", "INC A", "DEC A", "LD A,*h", "CCF",
	"LD B,B", "LD B,C", "LD B,D", "LD B,E", "LD B,I%h", "LD B,I%l", "LD B,(I%+^h)", "LD B,A",
	"LD C,B", "LD C,C", "LD C,D", "LD C,E", "LD C,I%h", "LD C,I%l", "LD C,(I%+^h)", "LD C,A",
	"LD D,B", "LD D,C", "LD D,D", "LD D,E", "LD D,I%h", "LD D,I%l", "LD D,(I%+^h)", "LD D,A",
	"LD E,B", "LD E,C", "LD E,D", "LD E,E", "LD E,I%h", "LD E,I%l", "LD E,(I%+^h)", "LD E,A",
	"LD I%h,B", "LD I%h,C", "LD I%h,D", "LD I%h,E", "LD I%h,I%h", "LD I%h,I%l", "LD H,(I%+^h)", "LD I%h,A",
	"LD I%l,B", "LD I%l,C", "LD I%l,D", "LD I%l,E", "LD I%l,I%h", "LD I%l,I%l", "LD L,(I%+^h)", "LD I%l,A",
	"LD (I%+^h),B", "LD (I%+^h),C", "LD (I%+^h),D", "LD (I%+^h),E", "LD (I%+^h),H", "LD (I%+^h),L", "HALT", "LD (I%+^h),A",
	"LD A,B", "LD A,C", "LD A,D", "LD A,E", "LD A,I%h", "LD A,I%l", "LD A,(I%+^h)", "LD A,A",
	"ADD B", "ADD C", "ADD D", "ADD E", "ADD I%h", "ADD I%l", "ADD (I%+^h)", "ADD A",
	"ADC B", "ADC C", "ADC D", "ADC E", "ADC I%h", "ADC I%l", "ADC (I%+^h)", "ADC,A",
	"SUB B", "SUB C", "SUB D", "SUB E", "SUB I%h", "SUB I%l", "SUB (I%+^h)", "SUB A",
	"SBC B", "SBC C", "SBC D", "SBC E", "SBC I%h", "SBC I%l", "SBC (I%+^h)", "SBC A",
	"AND B", "AND C", "AND D", "AND E", "AND I%h", "AND I%l", "AND (I%+^h)", "AND A",
	"XOR B", "XOR C", "XOR D", "XOR E", "XOR I%h", "XOR I%l", "XOR (I%+^h)", "XOR A",
	"OR B", "OR C", "OR D", "OR E", "OR I%h", "OR I%l", "OR (I%+^h)", "OR A",
	"CP B", "CP C", "CP D", "CP E", "CP I%h", "CP I%l", "CP (I%+^h)", "CP A",
	"RET NZ", "POP BC", "JP NZ,#h", "JP #h", "CALL NZ,#h", "PUSH BC", "ADD *h", "RST 00h",
	"RET Z", "RET", "JP Z,#h", "PFX_CB", "CALL Z,#h", "CALL #h", "ADC *h", "RST 08h",
	"RET NC", "POP DE", "JP NC,#h", "OUTA (*h)", "CALL NC,#h", "PUSH DE", "SUB *h", "RST 10h",
	"RET C", "EXX", "JP C,#h", "INA (*h)", "CALL C,#h", "PFX_DD", "SBC *h", "RST 18h",
	"RET PO", "POP I%", "JP PO,#h", "EX I%,(SP)", "CALL PO,#h", "PUSH I%", "AND *h", "RST 20h",
	"RET PE", "LD PC,I%", "JP PE,#h", "EX DE,I%", "CALL PE,#h", "PFX_ED", "XOR *h", "RST 28h",
	"RET P", "POP AF", "JP P,#h", "DI", "CALL P,#h", "PUSH AF", "OR *h", "RST 30h",
	"RET M", "LD SP,I%", "JP M,#h", "EI", "CALL M,#h", "PFX_FD", "CP *h", "RST 38h"
};

static const char* MnemonicsXCB[256] =
{
	"RLC B", "RLC C", "RLC D", "RLC E", "RLC H", "RLC L", "RLC (I%@h)", "RLC A",
	"RRC B", "RRC C", "RRC D", "RRC E", "RRC H", "RRC L", "RRC (I%@h)", "RRC A",
	"RL B", "RL C", "RL D", "RL E", "RL H", "RL L", "RL (I%@h)", "RL A",
	"RR B", "RR C", "RR D", "RR E", "RR H", "RR L", "RR (I%@h)", "RR A",
	"SLA B", "SLA C", "SLA D", "SLA E", "SLA H", "SLA L", "SLA (I%@h)", "SLA A",
	"SRA B", "SRA C", "SRA D", "SRA E", "SRA H", "SRA L", "SRA (I%@h)", "SRA A",
	"SLL B", "SLL C", "SLL D", "SLL E", "SLL H", "SLL L", "SLL (I%@h)", "SLL A",
	"SRL B", "SRL C", "SRL D", "SRL E", "SRL H", "SRL L", "SRL (I%@h)", "SRL A",
	"BIT 0,B", "BIT 0,C", "BIT 0,D", "BIT 0,E", "BIT 0,H", "BIT 0,L", "BIT 0,(I%@h)", "BIT 0,A",
	"BIT 1,B", "BIT 1,C", "BIT 1,D", "BIT 1,E", "BIT 1,H", "BIT 1,L", "BIT 1,(I%@h)", "BIT 1,A",
	"BIT 2,B", "BIT 2,C", "BIT 2,D", "BIT 2,E", "BIT 2,H", "BIT 2,L", "BIT 2,(I%@h)", "BIT 2,A",
	"BIT 3,B", "BIT 3,C", "BIT 3,D", "BIT 3,E", "BIT 3,H", "BIT 3,L", "BIT 3,(I%@h)", "BIT 3,A",
	"BIT 4,B", "BIT 4,C", "BIT 4,D", "BIT 4,E", "BIT 4,H", "BIT 4,L", "BIT 4,(I%@h)", "BIT 4,A",
	"BIT 5,B", "BIT 5,C", "BIT 5,D", "BIT 5,E", "BIT 5,H", "BIT 5,L", "BIT 5,(I%@h)", "BIT 5,A",
	"BIT 6,B", "BIT 6,C", "BIT 6,D", "BIT 6,E", "BIT 6,H", "BIT 6,L", "BIT 6,(I%@h)", "BIT 6,A",
	"BIT 7,B", "BIT 7,C", "BIT 7,D", "BIT 7,E", "BIT 7,H", "BIT 7,L", "BIT 7,(I%@h)", "BIT 7,A",
	"RES 0,B", "RES 0,C", "RES 0,D", "RES 0,E", "RES 0,H", "RES 0,L", "RES 0,(I%@h)", "RES 0,A",
	"RES 1,B", "RES 1,C", "RES 1,D", "RES 1,E", "RES 1,H", "RES 1,L", "RES 1,(I%@h)", "RES 1,A",
	"RES 2,B", "RES 2,C", "RES 2,D", "RES 2,E", "RES 2,H", "RES 2,L", "RES 2,(I%@h)", "RES 2,A",
	"RES 3,B", "RES 3,C", "RES 3,D", "RES 3,E", "RES 3,H", "RES 3,L", "RES 3,(I%@h)", "RES 3,A",
	"RES 4,B", "RES 4,C", "RES 4,D", "RES 4,E", "RES 4,H", "RES 4,L", "RES 4,(I%@h)", "RES 4,A",
	"RES 5,B", "RES 5,C", "RES 5,D", "RES 5,E", "RES 5,H", "RES 5,L", "RES 5,(I%@h)", "RES 5,A",
	"RES 6,B", "RES 6,C", "RES 6,D", "RES 6,E", "RES 6,H", "RES 6,L", "RES 6,(I%@h)", "RES 6,A",
	"RES 7,B", "RES 7,C", "RES 7,D", "RES 7,E", "RES 7,H", "RES 7,L", "RES 7,(I%@h)", "RES 7,A",
	"SET 0,B", "SET 0,C", "SET 0,D", "SET 0,E", "SET 0,H", "SET 0,L", "SET 0,(I%@h)", "SET 0,A",
	"SET 1,B", "SET 1,C", "SET 1,D", "SET 1,E", "SET 1,H", "SET 1,L", "SET 1,(I%@h)", "SET 1,A",
	"SET 2,B", "SET 2,C", "SET 2,D", "SET 2,E", "SET 2,H", "SET 2,L", "SET 2,(I%@h)", "SET 2,A",
	"SET 3,B", "SET 3,C", "SET 3,D", "SET 3,E", "SET 3,H", "SET 3,L", "SET 3,(I%@h)", "SET 3,A",
	"SET 4,B", "SET 4,C", "SET 4,D", "SET 4,E", "SET 4,H", "SET 4,L", "SET 4,(I%@h)", "SET 4,A",
	"SET 5,B", "SET 5,C", "SET 5,D", "SET 5,E", "SET 5,H", "SET 5,L", "SET 5,(I%@h)", "SET 5,A",
	"SET 6,B", "SET 6,C", "SET 6,D", "SET 6,E", "SET 6,H", "SET 6,L", "SET 6,(I%@h)", "SET 6,A",
	"SET 7,B", "SET 7,C", "SET 7,D", "SET 7,E", "SET 7,H", "SET 7,L", "SET 7,(I%@h)", "SET 7,A"
};

static const char* CPMCalls[41] =
{
	"System Reset", "Console Input", "Console Output", "Reader Input", "Punch Output", "List Output", "Direct I/O", "Get IOByte",
	"Set IOByte", "Print String", "Read Buffered", "Console Status", "Get Version", "Reset Disk", "Select Disk", "Open File",
	"Close File", "Search First", "Search Next", "Delete File", "Read Sequential", "Write Sequential", "Make File", "Rename File",
	"Get Login Vector", "Get Current Disk", "Set DMA Address", "Get Alloc", "Write Protect Disk", "Get R/O Vector", "Set File Attr", "Get Disk Params",
	"Get/Set User", "Read Random", "Write Random", "Get File Size", "Set Random Record", "Reset Drive", "N/A", "N/A", "Write Random 0 fill"
};

int32 Watch = -1;
#endif

/* Memory management    */
static uint8 GET_BYTE(uint16 a) {
	return _RamRead(a);
}

static void PUT_BYTE(uint16 a, uint8 v) {
	_RamWrite(a, v);
}

static uint16 GET_WORD(uint16 a) {
	return _RamRead(a) | (_RamRead(a + 1) << 8);
}

static void PUT_WORD(uint16 a, uint32 v) {
	_RamWrite(a, v);
	_RamWrite(++a, v >> 8);
}

#define RAM_MM(a)   GET_BYTE(a--)
#define RAM_PP(a)   GET_BYTE(a++)

#define PUT_BYTE_PP(a,v) PUT_BYTE(a++, v)
#define PUT_BYTE_MM(a,v) PUT_BYTE(a--, v)
#define MM_PUT_BYTE(a,v) PUT_BYTE(--a, v)

#define PUSH(x) do {            \
	MM_PUT_BYTE(SP, (x) >> 8);  \
	MM_PUT_BYTE(SP, x);         \
} while (0)

/*  Macros for the IN/OUT instructions INI/INIR/IND/INDR/OUTI/OTIR/OUTD/OTDR

Pre condition
temp == value of register B at entry of the instruction
acu == value of transferred byte (IN or OUT)
Post condition
F is set correctly

Use INOUTFLAGS_ZERO(x) for INIR/INDR/OTIR/OTDR where
x == (C + 1) & 0xff for INIR
x == L              for OTIR and OTDR
x == (C - 1) & 0xff for INDR
Use INOUTFLAGS_NONZERO(x) for INI/IND/OUTI/OUTD where
x == (C + 1) & 0xff for INI
x == L              for OUTI and OUTD
x == (C - 1) & 0xff for IND
*/
#define INOUTFLAGS(syxz, x)                                             \
    AF = (AF & 0xff00) | (syxz) |               /* SF, YF, XF, ZF   */  \
        ((acu & 0x80) >> 6) |                           /* NF       */  \
        ((acu + (x)) > 0xff ? (FLAG_C | FLAG_H) : 0) |  /* CF, HF   */  \
        parityTable[((acu + (x)) & 7) ^ temp]           /* PF       */

#define INOUTFLAGS_ZERO(x)      INOUTFLAGS(FLAG_Z, x)
#define INOUTFLAGS_NONZERO(x)                                           \
    INOUTFLAGS((HIGH_REGISTER(BC) & 0xa8) | ((HIGH_REGISTER(BC) == 0) << 6), x)

static inline void Z80reset(void) {
	PC = 0;
	IFF = 0;
	IR = 0;
	Status = STATUS_RUNNING;
	Debug = 0;
	Step = -1;

	#ifndef preTables
		initTables();
	#endif
}

#ifdef DEBUG
#include "debug.h"
#endif

static inline void Z80run(void) {
	uint32 temp = 0;
	uint32 acu;
	uint32 sum;
	uint32 cbits;
	uint32 op = 0;
	uint32 adr;

	/* main instruction fetch/decode loop */
	while (!Status) {	/* loop until Status != 0 */

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
	INCR(1); /* Add one M1 cycle to refresh counter */

	/* push instruction into trace (before it is executed) */
#if defined(DEBUG) || defined(iDEBUG)
	z80_trace_push(PCX);
#endif

#ifdef iDEBUG
		iLogFile = fopen("iDump.log", "a");
		switch (RAM[PCX & 0xffff]) {
		case 0xCB: iLogTxt = MnemonicsCB[RAM[(PCX & 0xffff) + 1]]; break;
		case 0xED: iLogTxt = MnemonicsED[RAM[(PCX & 0xffff) + 1]]; break;
		case 0xDD:
		case 0xFD:
			if (RAM[PCX & 0xffff] == 0xCB) {
				iLogTxt = MnemonicsXCB[RAM[(PCX & 0xffff) + 1]]; break;
			} else {
				iLogTxt = MnemonicsXX[RAM[(PCX & 0xffff) + 1]]; break;
			}
		default: iLogTxt = Mnemonics[RAM[PCX & 0xffff]];
		}
		sprintf(iLogBuffer, "0x%04x : 0x%02x = %s\n", PCX, RAM[PCX & 0xffff], iLogTxt);
		fputs(iLogBuffer, iLogFile);
		fclose(iLogFile);
#endif

		switch (RAM_PP(PC)) {

		case 0x00:      /* NOP */
			break;

		case 0x01:      /* LD BC,nnnn */
			BC = GET_WORD(PC++);
			++PC;
			break;

		case 0x02:      /* LD (BC),A */
			PUT_BYTE(BC, HIGH_REGISTER(AF));
			break;

		case 0x03:      /* INC BC */
			++BC;
			break;

		case 0x04:      /* INC B */
			BC += 0x100;
			temp = HIGH_REGISTER(BC);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x05:      /* DEC B */
			BC -= 0x100;
			temp = HIGH_REGISTER(BC);
			AF = (AF & ~0xfe) | decZ80Table[temp];
			break;

		case 0x06:      /* LD B,nn */
			SET_HIGH_REGISTER(BC, RAM_PP(PC));
			break;

		case 0x07:      /* RLCA */
			AF = ((AF >> 7) & 0x0128) | ((AF << 1) & ~0x1ff) |
				(AF & 0xc4) | ((AF >> 15) & 1);
			break;

		case 0x08:      /* EX AF,AF' */
		    AF ^= AF1;
    		AF1 ^= AF;
    		AF ^= AF1;
			break;

		case 0x09:      /* ADD HL,BC */
			HL &= ADDRMASK;
			BC &= ADDRMASK;
			sum = HL + BC;
			AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(HL ^ BC ^ sum) >> 8];
			HL = sum;
			break;

		case 0x0a:      /* LD A,(BC) */
			SET_HIGH_REGISTER(AF, GET_BYTE(BC));
			break;

		case 0x0b:      /* DEC BC */
			--BC;
			break;

		case 0x0c:      /* INC C */
			temp = LOW_REGISTER(BC) + 1;
			SET_LOW_REGISTER(BC, temp);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x0d:      /* DEC C */
			temp = LOW_REGISTER(BC) - 1;
			SET_LOW_REGISTER(BC, temp);
			AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
			break;

		case 0x0e:      /* LD C,nn */
			SET_LOW_REGISTER(BC, RAM_PP(PC));
			break;

		case 0x0f:      /* RRCA */
			AF = (AF & 0xc4) | rrcaTable[HIGH_REGISTER(AF)];
			break;

		case 0x10:      /* DJNZ dd */
			if ((BC -= 0x100) & 0xff00)
				PC += (int8)GET_BYTE(PC) + 1;
			else
				++PC;
			break;

		case 0x11:      /* LD DE,nnnn */
			DE = GET_WORD(PC++);
			++PC;
			break;

		case 0x12:      /* LD (DE),A */
			PUT_BYTE(DE, HIGH_REGISTER(AF));
			break;

		case 0x13:      /* INC DE */
			++DE;
			break;

		case 0x14:      /* INC D */
			DE += 0x100;
			temp = HIGH_REGISTER(DE);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x15:      /* DEC D */
			DE -= 0x100;
			temp = HIGH_REGISTER(DE);
			AF = (AF & ~0xfe) | decZ80Table[temp];
			break;

		case 0x16:      /* LD D,nn */
			SET_HIGH_REGISTER(DE, RAM_PP(PC));
			break;

		case 0x17:      /* RLA */
			AF = ((AF << 8) & 0x0100) | ((AF >> 7) & 0x28) | ((AF << 1) & ~0x01ff) |
				(AF & 0xc4) | ((AF >> 15) & 1);
			break;

		case 0x18:      /* JR dd */
			PC += (int8)GET_BYTE(PC) + 1;
			break;

		case 0x19:      /* ADD HL,DE */
			HL &= ADDRMASK;
			DE &= ADDRMASK;
			sum = HL + DE;
			AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(HL ^ DE ^ sum) >> 8];
			HL = sum;
			break;

		case 0x1a:      /* LD A,(DE) */
			SET_HIGH_REGISTER(AF, GET_BYTE(DE));
			break;

		case 0x1b:      /* DEC DE */
			--DE;
			break;

		case 0x1c:      /* INC E */
			temp = LOW_REGISTER(DE) + 1;
			SET_LOW_REGISTER(DE, temp);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x1d:      /* DEC E */
			temp = LOW_REGISTER(DE) - 1;
			SET_LOW_REGISTER(DE, temp);
			AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
			break;

		case 0x1e:      /* LD E,nn */
			SET_LOW_REGISTER(DE, RAM_PP(PC));
			break;

		case 0x1f:      /* RRA */
			AF = ((AF & 1) << 15) | (AF & 0xc4) | (rrcaTable[HIGH_REGISTER(AF)] & 0x7fff);
			break;

		case 0x20:      /* JR NZ,dd */
			if (TSTFLAG(Z))
				++PC;
			else
				PC += (int8)GET_BYTE(PC) + 1;
			break;

		case 0x21:      /* LD HL,nnnn */
			HL = GET_WORD(PC++);
			++PC;
			break;

		case 0x22:      /* LD (nnnn),HL */
			PUT_WORD(GET_WORD(PC++), HL);
			++PC;
			break;

		case 0x23:      /* INC HL */
			++HL;
			break;

		case 0x24:      /* INC H */
			HL += 0x100;
			temp = HIGH_REGISTER(HL);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x25:      /* DEC H */
			HL -= 0x100;
			temp = HIGH_REGISTER(HL);
			AF = (AF & ~0xfe) | decZ80Table[temp];
			break;

		case 0x26:      /* LD H,nn */
			SET_HIGH_REGISTER(HL, RAM_PP(PC));
			break;

		case 0x27:      /* DAA */
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

		case 0x28:      /* JR Z,dd */
			if (TSTFLAG(Z))
				PC += (int8)GET_BYTE(PC) + 1;
			else
				++PC;
			break;

		case 0x29:      /* ADD HL,HL */
			HL &= ADDRMASK;
			sum = HL + HL;
			AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[sum >> 8];
			HL = sum;
			break;

		case 0x2a:      /* LD HL,(nnnn) */
			HL = GET_WORD(GET_WORD(PC++));
			++PC;
			break;

		case 0x2b:      /* DEC HL */
			--HL;
			break;

		case 0x2c:      /* INC L */
			temp = LOW_REGISTER(HL) + 1;
			SET_LOW_REGISTER(HL, temp);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x2d:      /* DEC L */
			temp = LOW_REGISTER(HL) - 1;
			SET_LOW_REGISTER(HL, temp);
			AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
			break;

		case 0x2e:      /* LD L,nn */
			SET_LOW_REGISTER(HL, RAM_PP(PC));
			break;

		case 0x2f:      /* CPL */
			AF = (~AF & ~0xff) | (AF & 0xc5) | ((~AF >> 8) & 0x28) | 0x12;
			break;

		case 0x30:      /* JR NC,dd */
			if (TSTFLAG(C))
				++PC;
			else
				PC += (int8)GET_BYTE(PC) + 1;
			break;

		case 0x31:      /* LD SP,nnnn */
			SP = GET_WORD(PC++);
			++PC;
			break;

		case 0x32:      /* LD (nnnn),A */
			PUT_BYTE(GET_WORD(PC++), HIGH_REGISTER(AF));
			++PC;
			break;

		case 0x33:      /* INC SP */
			++SP;
			break;

		case 0x34:      /* INC (HL) */
			temp = GET_BYTE(HL) + 1;
			PUT_BYTE(HL, temp);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x35:      /* DEC (HL) */
			temp = GET_BYTE(HL) - 1;
			PUT_BYTE(HL, temp);
			AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
			break;

		case 0x36:      /* LD (HL),nn */
			PUT_BYTE(HL, RAM_PP(PC));
			break;

		case 0x37:      /* SCF */
			AF = (AF & ~0x3b) | ((AF >> 8) & 0x28) | 1;
			break;

		case 0x38:      /* JR C,dd */
			if (TSTFLAG(C))
				PC += (int8)GET_BYTE(PC) + 1;
			else
				++PC;
			break;

		case 0x39:      /* ADD HL,SP */
			HL &= ADDRMASK;
			SP &= ADDRMASK;
			sum = HL + SP;
			AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(HL ^ SP ^ sum) >> 8];
			HL = sum;
			break;

		case 0x3a:      /* LD A,(nnnn) */
			SET_HIGH_REGISTER(AF, GET_BYTE(GET_WORD(PC++)));
			++PC;
			break;

		case 0x3b:      /* DEC SP */
			--SP;
			break;

		case 0x3c:      /* INC A */
			AF += 0x100;
			temp = HIGH_REGISTER(AF);
			AF = (AF & ~0xfe) | incZ80Table[temp];
			break;

		case 0x3d:      /* DEC A */
			AF -= 0x100;
			temp = HIGH_REGISTER(AF);
			AF = (AF & ~0xfe) | decZ80Table[temp];
			break;

		case 0x3e:      /* LD A,nn */
			SET_HIGH_REGISTER(AF, RAM_PP(PC));
			break;

		case 0x3f:      /* CCF */
			AF = (AF & ~0x3b) | ((AF >> 8) & 0x28) | ((AF & 1) << 4) | (~AF & 1);
			break;

		case 0x40:      /* LD B,B */
			break;

		case 0x41:      /* LD B,C */
			BC = (BC & 0xff) | ((BC & 0xff) << 8);
			break;

		case 0x42:      /* LD B,D */
			BC = (BC & 0xff) | (DE & ~0xff);
			break;

		case 0x43:      /* LD B,E */
			BC = (BC & 0xff) | ((DE & 0xff) << 8);
			break;

		case 0x44:      /* LD B,H */
			BC = (BC & 0xff) | (HL & ~0xff);
			break;

		case 0x45:      /* LD B,L */
			BC = (BC & 0xff) | ((HL & 0xff) << 8);
			break;

		case 0x46:      /* LD B,(HL) */
			SET_HIGH_REGISTER(BC, GET_BYTE(HL));
			break;

		case 0x47:      /* LD B,A */
			BC = (BC & 0xff) | (AF & ~0xff);
			break;

		case 0x48:      /* LD C,B */
			BC = (BC & ~0xff) | ((BC >> 8) & 0xff);
			break;

		case 0x49:      /* LD C,C */
			break;

		case 0x4a:      /* LD C,D */
			BC = (BC & ~0xff) | ((DE >> 8) & 0xff);
			break;

		case 0x4b:      /* LD C,E */
			BC = (BC & ~0xff) | (DE & 0xff);
			break;

		case 0x4c:      /* LD C,H */
			BC = (BC & ~0xff) | ((HL >> 8) & 0xff);
			break;

		case 0x4d:      /* LD C,L */
			BC = (BC & ~0xff) | (HL & 0xff);
			break;

		case 0x4e:      /* LD C,(HL) */
			SET_LOW_REGISTER(BC, GET_BYTE(HL));
			break;

		case 0x4f:      /* LD C,A */
			BC = (BC & ~0xff) | ((AF >> 8) & 0xff);
			break;

		case 0x50:      /* LD D,B */
			DE = (DE & 0xff) | (BC & ~0xff);
			break;

		case 0x51:      /* LD D,C */
			DE = (DE & 0xff) | ((BC & 0xff) << 8);
			break;

		case 0x52:      /* LD D,D */
			break;

		case 0x53:      /* LD D,E */
			DE = (DE & 0xff) | ((DE & 0xff) << 8);
			break;

		case 0x54:      /* LD D,H */
			DE = (DE & 0xff) | (HL & ~0xff);
			break;

		case 0x55:      /* LD D,L */
			DE = (DE & 0xff) | ((HL & 0xff) << 8);
			break;

		case 0x56:      /* LD D,(HL) */
			SET_HIGH_REGISTER(DE, GET_BYTE(HL));
			break;

		case 0x57:      /* LD D,A */
			DE = (DE & 0xff) | (AF & ~0xff);
			break;

		case 0x58:      /* LD E,B */
			DE = (DE & ~0xff) | ((BC >> 8) & 0xff);
			break;

		case 0x59:      /* LD E,C */
			DE = (DE & ~0xff) | (BC & 0xff);
			break;

		case 0x5a:      /* LD E,D */
			DE = (DE & ~0xff) | ((DE >> 8) & 0xff);
			break;

		case 0x5b:      /* LD E,E */
			break;

		case 0x5c:      /* LD E,H */
			DE = (DE & ~0xff) | ((HL >> 8) & 0xff);
			break;

		case 0x5d:      /* LD E,L */
			DE = (DE & ~0xff) | (HL & 0xff);
			break;

		case 0x5e:      /* LD E,(HL) */
			SET_LOW_REGISTER(DE, GET_BYTE(HL));
			break;

		case 0x5f:      /* LD E,A */
			DE = (DE & ~0xff) | ((AF >> 8) & 0xff);
			break;

		case 0x60:      /* LD H,B */
			HL = (HL & 0xff) | (BC & ~0xff);
			break;

		case 0x61:      /* LD H,C */
			HL = (HL & 0xff) | ((BC & 0xff) << 8);
			break;

		case 0x62:      /* LD H,D */
			HL = (HL & 0xff) | (DE & ~0xff);
			break;

		case 0x63:      /* LD H,E */
			HL = (HL & 0xff) | ((DE & 0xff) << 8);
			break;

		case 0x64:      /* LD H,H */
			break;

		case 0x65:      /* LD H,L */
			HL = (HL & 0xff) | ((HL & 0xff) << 8);
			break;

		case 0x66:      /* LD H,(HL) */
			SET_HIGH_REGISTER(HL, GET_BYTE(HL));
			break;

		case 0x67:      /* LD H,A */
			HL = (HL & 0xff) | (AF & ~0xff);
			break;

		case 0x68:      /* LD L,B */
			HL = (HL & ~0xff) | ((BC >> 8) & 0xff);
			break;

		case 0x69:      /* LD L,C */
			HL = (HL & ~0xff) | (BC & 0xff);
			break;

		case 0x6a:      /* LD L,D */
			HL = (HL & ~0xff) | ((DE >> 8) & 0xff);
			break;

		case 0x6b:      /* LD L,E */
			HL = (HL & ~0xff) | (DE & 0xff);
			break;

		case 0x6c:      /* LD L,H */
			HL = (HL & ~0xff) | ((HL >> 8) & 0xff);
			break;

		case 0x6d:      /* LD L,L */
			break;

		case 0x6e:      /* LD L,(HL) */
			SET_LOW_REGISTER(HL, GET_BYTE(HL));
			break;

		case 0x6f:      /* LD L,A */
			HL = (HL & ~0xff) | ((AF >> 8) & 0xff);
			break;

		case 0x70:      /* LD (HL),B */
			PUT_BYTE(HL, HIGH_REGISTER(BC));
			break;

		case 0x71:      /* LD (HL),C */
			PUT_BYTE(HL, LOW_REGISTER(BC));
			break;

		case 0x72:      /* LD (HL),D */
			PUT_BYTE(HL, HIGH_REGISTER(DE));
			break;

		case 0x73:      /* LD (HL),E */
			PUT_BYTE(HL, LOW_REGISTER(DE));
			break;

		case 0x74:      /* LD (HL),H */
			PUT_BYTE(HL, HIGH_REGISTER(HL));
			break;

		case 0x75:      /* LD (HL),L */
			PUT_BYTE(HL, LOW_REGISTER(HL));
			break;

		case 0x76:      /* HALT */
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
			--PC;
			Status = STATUS_EXIT;
			break;

		case 0x77:      /* LD (HL),A */
			PUT_BYTE(HL, HIGH_REGISTER(AF));
			break;

		case 0x78:      /* LD A,B */
			AF = (AF & 0xff) | (BC & ~0xff);
			break;

		case 0x79:      /* LD A,C */
			AF = (AF & 0xff) | ((BC & 0xff) << 8);
			break;

		case 0x7a:      /* LD A,D */
			AF = (AF & 0xff) | (DE & ~0xff);
			break;

		case 0x7b:      /* LD A,E */
			AF = (AF & 0xff) | ((DE & 0xff) << 8);
			break;

		case 0x7c:      /* LD A,H */
			AF = (AF & 0xff) | (HL & ~0xff);
			break;

		case 0x7d:      /* LD A,L */
			AF = (AF & 0xff) | ((HL & 0xff) << 8);
			break;

		case 0x7e:      /* LD A,(HL) */
			SET_HIGH_REGISTER(AF, GET_BYTE(HL));
			break;

		case 0x7f:      /* LD A,A */
			break;

		case 0x80:      /* ADD A,B */
			temp = HIGH_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsTable[cbits] | (SET_PV);
			break;

		case 0x81:      /* ADD A,C */
			temp = LOW_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x82:      /* ADD A,D */
			temp = HIGH_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x83:      /* ADD A,E */
			temp = LOW_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x84:      /* ADD A,H */
			temp = HIGH_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x85:      /* ADD A,L */
			temp = LOW_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x86:      /* ADD A,(HL) */
			temp = GET_BYTE(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x87:      /* ADD A,A */
			cbits = 2 * HIGH_REGISTER(AF);
			AF = (xororTable[cbits & 0xff] & ~4) | cbitsTable[cbits] | (SET_PVS(cbits));
			break;

		case 0x88:      /* ADC A,B */
			temp = HIGH_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x89:      /* ADC A,C */
			temp = LOW_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x8a:      /* ADC A,D */
			temp = HIGH_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x8b:      /* ADC A,E */
			temp = LOW_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x8c:      /* ADC A,H */
			temp = HIGH_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x8d:      /* ADC A,L */
			temp = LOW_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x8e:      /* ADC A,(HL) */
			temp = GET_BYTE(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0x8f:      /* ADC A,A */
			cbits = 2 * HIGH_REGISTER(AF) + TSTFLAG(C);
			AF = (xororTable[cbits & 0xff] & ~4) | cbitsZ80DupTable[cbits & 0x1ff];
			break;

		case 0x90:      /* SUB B */
			temp = HIGH_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x91:      /* SUB C */
			temp = LOW_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x92:      /* SUB D */
			temp = HIGH_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x93:      /* SUB E */
			temp = LOW_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x94:      /* SUB H */
			temp = HIGH_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x95:      /* SUB L */
			temp = LOW_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x96:      /* SUB (HL) */
			temp = GET_BYTE(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x97:      /* SUB A */
			AF = 0x42;
			break;

		case 0x98:      /* SBC A,B */
			temp = HIGH_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x99:      /* SBC A,C */
			temp = LOW_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x9a:      /* SBC A,D */
			temp = HIGH_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x9b:      /* SBC A,E */
			temp = LOW_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x9c:      /* SBC A,H */
			temp = HIGH_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x9d:      /* SBC A,L */
			temp = LOW_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x9e:      /* SBC A,(HL) */
			temp = GET_BYTE(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0x9f:      /* SBC A,A */
			cbits = -TSTFLAG(C);
			AF = (xororTable[cbits & 0xff] & ~4) | cbits2Z80DupTable[cbits & 0x1ff];
			break;

		case 0xa0:      /* AND B */
			AF = (xororTable[((AF & BC) >> 8) & 0xff] | 0x10);
			break;

		case 0xa1:      /* AND C */
			AF = (xororTable[((AF >> 8)& BC) & 0xff] | 0x10);
			break;

		case 0xa2:      /* AND D */
			AF = (xororTable[((AF & DE) >> 8) & 0xff] | 0x10);
			break;

		case 0xa3:      /* AND E */
			AF = (xororTable[((AF >> 8)& DE) & 0xff] | 0x10);
			break;

		case 0xa4:      /* AND H */
			AF = (xororTable[((AF & HL) >> 8) & 0xff] | 0x10);
			break;

		case 0xa5:      /* AND L */
			AF = (xororTable[((AF >> 8)& HL) & 0xff] | 0x10);
			break;

		case 0xa6:      /* AND (HL) */
			AF = (xororTable[((AF >> 8)& GET_BYTE(HL)) & 0xff] | 0x10);
			break;

		case 0xa7:      /* AND A */
			AF = (xororTable[(AF >> 8) & 0xff] | 0x10);
			break;

		case 0xa8:      /* XOR B */
			AF = xororTable[((AF ^ BC) >> 8) & 0xff];
			break;

		case 0xa9:      /* XOR C */
			AF = xororTable[((AF >> 8) ^ BC) & 0xff];
			break;

		case 0xaa:      /* XOR D */
			AF = xororTable[((AF ^ DE) >> 8) & 0xff];
			break;

		case 0xab:      /* XOR E */
			AF = xororTable[((AF >> 8) ^ DE) & 0xff];
			break;

		case 0xac:      /* XOR H */
			AF = xororTable[((AF ^ HL) >> 8) & 0xff];
			break;

		case 0xad:      /* XOR L */
			AF = xororTable[((AF >> 8) ^ HL) & 0xff];
			break;

		case 0xae:      /* XOR (HL) */
			AF = xororTable[((AF >> 8) ^ GET_BYTE(HL)) & 0xff];
			break;

		case 0xaf:      /* XOR A */
			AF = 0x44;
			break;

		case 0xb0:      /* OR B */
			AF = xororTable[((AF | BC) >> 8) & 0xff];
			break;

		case 0xb1:      /* OR C */
			AF = xororTable[((AF >> 8) | BC) & 0xff];
			break;

		case 0xb2:      /* OR D */
			AF = xororTable[((AF | DE) >> 8) & 0xff];
			break;

		case 0xb3:      /* OR E */
			AF = xororTable[((AF >> 8) | DE) & 0xff];
			break;

		case 0xb4:      /* OR H */
			AF = xororTable[((AF | HL) >> 8) & 0xff];
			break;

		case 0xb5:      /* OR L */
			AF = xororTable[((AF >> 8) | HL) & 0xff];
			break;

		case 0xb6:      /* OR (HL) */
			AF = xororTable[((AF >> 8) | GET_BYTE(HL)) & 0xff];
			break;

		case 0xb7:      /* OR A */
			AF = xororTable[(AF >> 8) & 0xff];
			break;

		case 0xb8:      /* CP B */
			temp = HIGH_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xb9:      /* CP C */
			temp = LOW_REGISTER(BC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xba:      /* CP D */
			temp = HIGH_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xbb:      /* CP E */
			temp = LOW_REGISTER(DE);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xbc:      /* CP H */
			temp = HIGH_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xbd:      /* CP L */
			temp = LOW_REGISTER(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xbe:      /* CP (HL) */
			temp = GET_BYTE(HL);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xbf:      /* CP A */
			SET_LOW_REGISTER(AF, (HIGH_REGISTER(AF) & 0x28) | 0x42);
			break;

		case 0xc0:      /* RET NZ */
			if (!(TSTFLAG(Z)))
				POP(PC);
			break;

		case 0xc1:      /* POP BC */
			POP(BC);
			break;

		case 0xc2:      /* JP NZ,nnnn */
			JPC(!TSTFLAG(Z));
			break;

		case 0xc3:      /* JP nnnn */
			JPC(1);
			break;

		case 0xc4:      /* CALL NZ,nnnn */
			CALLC(!TSTFLAG(Z));
			break;

		case 0xc5:      /* PUSH BC */
			PUSH(BC);
			break;

		case 0xc6:      /* ADD A,nn */
			temp = RAM_PP(PC);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0xc7:      /* RST 0 */
			PUSH(PC);
			PC = 0;
			break;

		case 0xc8:      /* RET Z */
			if (TSTFLAG(Z))
				POP(PC);
			break;

		case 0xc9:      /* RET */
			POP(PC);
			break;

		case 0xca:      /* JP Z,nnnn */
			JPC(TSTFLAG(Z));
			break;

		case 0xcb:      /* CB prefix */
			INCR(1); /* Add one M1 cycle to refresh counter */
			adr = HL;
			switch ((op = GET_BYTE(PC)) & 7) {

			case 0:
				acu = HIGH_REGISTER(BC);
				break;

			case 1:
				acu = LOW_REGISTER(BC);
				break;

			case 2:
				acu = HIGH_REGISTER(DE);
				break;

			case 3:
				acu = LOW_REGISTER(DE);
				break;

			case 4:
				acu = HIGH_REGISTER(HL);
				break;

			case 5:
				acu = LOW_REGISTER(HL);
				break;

			case 6:
				acu = GET_BYTE(adr);
				break;

			default:
				acu = HIGH_REGISTER(AF);
				break;
			}
			++PC;
			switch (op & 0xc0) {

			case 0x00:  /* shift/rotate */
				switch (op & 0x38) {

					case 0x00:/* RLC */
						temp = (acu << 1) | (acu >> 7);
						cbits = temp & 1;
						break;

					case 0x08:/* RRC */
						temp = (acu >> 1) | (acu << 7);
						cbits = temp & 0x80;
						break;

					case 0x10:/* RL */
						temp = (acu << 1) | TSTFLAG(C);
						cbits = acu & 0x80;
						break;

					case 0x18:/* RR */
						temp = (acu >> 1) | (TSTFLAG(C) << 7);
						cbits = acu & 1;
						break;

					case 0x20:/* SLA */
						temp = acu << 1;
						cbits = acu & 0x80;
						break;

					case 0x28:/* SRA */
						temp = (acu >> 1) | (acu & 0x80);
						cbits = acu & 1;
						break;

					case 0x30:/* SLIA */
						temp = (acu << 1) | 1;
						cbits = acu & 0x80;
						break;

					case 0x38:/* SRL */
						temp = acu >> 1;
						cbits = acu & 1;
						break;

					default:
						temp = acu;
						cbits = 0;
					}
					AF = (AF & ~0xff) | rotateShiftTable[temp & 0xff] | !!cbits;
				break;

			case 0x40:  /* BIT */
				if (acu & (1 << ((op >> 3) & 7)))
					AF = (AF & ~0xfe) | 0x10 | (((op & 0x38) == 0x38) << 7);
				else
					AF = (AF & ~0xfe) | 0x54;
				if ((op & 7) != 6)
					AF |= (acu & 0x28);
				temp = acu;
				break;

			case 0x80:  /* RES */
				temp = acu & ~(1 << ((op >> 3) & 7));
				break;

			case 0xc0:  /* SET */
				temp = acu | (1 << ((op >> 3) & 7));
				break;
			}
			switch (op & 7) {

			case 0:
				SET_HIGH_REGISTER(BC, temp);
				break;

			case 1:
				SET_LOW_REGISTER(BC, temp);
				break;

			case 2:
				SET_HIGH_REGISTER(DE, temp);
				break;

			case 3:
				SET_LOW_REGISTER(DE, temp);
				break;

			case 4:
				SET_HIGH_REGISTER(HL, temp);
				break;

			case 5:
				SET_LOW_REGISTER(HL, temp);
				break;

			case 6:
				PUT_BYTE(adr, temp);
				break;

			default:
				SET_HIGH_REGISTER(AF, temp);
				break;
			}
			break;

		case 0xcc:      /* CALL Z,nnnn */
			CALLC(TSTFLAG(Z));
			break;

		case 0xcd:      /* CALL nnnn */
			CALLC(1);
			break;

		case 0xce:      /* ADC A,nn */
			temp = RAM_PP(PC);
			acu = HIGH_REGISTER(AF);
			sum = acu + temp + TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[cbits & 0x1ff];
			break;

		case 0xcf:      /* RST 8 */
			PUSH(PC);
			PC = 8;
#ifdef INT_HANDOFF
			_Bios();
			POP(PC);
#endif
			break;

		case 0xd0:      /* RET NC */
			if (!(TSTFLAG(C)))
				POP(PC);
			break;

		case 0xd1:      /* POP DE */
			POP(DE);
			break;

		case 0xd2:      /* JP NC,nnnn */
			JPC(!TSTFLAG(C));
			break;

		case 0xd3:      /* OUT (nn),A */
			cpu_out(RAM_PP(PC), HIGH_REGISTER(AF));
			break;

		case 0xd4:      /* CALL NC,nnnn */
			CALLC(!TSTFLAG(C));
			break;

		case 0xd5:      /* PUSH DE */
			PUSH(DE);
			break;

		case 0xd6:      /* SUB nn */
			temp = RAM_PP(PC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0xd7:      /* RST 10H */
			PUSH(PC);
			PC = 0x10;
#ifdef INT_HANDOFF
			_Bdos();
			POP(PC);
#endif
			break;

		case 0xd8:      /* RET C */
			if (TSTFLAG(C))
				POP(PC);
			break;

		case 0xd9:      /* EXX */
			BC ^= BC1;
			BC1 ^= BC;
			BC ^= BC1;
			DE ^= DE1;
			DE1 ^= DE;
			DE ^= DE1;
			HL ^= HL1;
			HL1 ^= HL;
			HL ^= HL1;
			break;

		case 0xda:      /* JP C,nnnn */
			JPC(TSTFLAG(C));
			break;

		case 0xdb:      /* IN A,(nn) */
			SET_HIGH_REGISTER(AF, cpu_in(RAM_PP(PC)));
			break;

		case 0xdc:      /* CALL C,nnnn */
			CALLC(TSTFLAG(C));
			break;

		case 0xdd:      /* DD prefix */
			INCR(1); /* Add one M1 cycle to refresh counter */
			switch (RAM_PP(PC)) {

			case 0x09:      /* ADD IX,BC */
				IX &= ADDRMASK;
				BC &= ADDRMASK;
				sum = IX + BC;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(IX ^ BC ^ sum) >> 8];
				IX = sum;
				break;

			case 0x19:      /* ADD IX,DE */
				IX &= ADDRMASK;
				DE &= ADDRMASK;
				sum = IX + DE;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(IX ^ DE ^ sum) >> 8];
				IX = sum;
				break;

			case 0x21:      /* LD IX,nnnn */
				IX = GET_WORD(PC++);
				++PC;
				break;

			case 0x22:      /* LD (nnnn),IX */
				PUT_WORD(GET_WORD(PC++), IX);
				++PC;
				break;

			case 0x23:      /* INC IX */
				++IX;
				break;

			case 0x24:      /* INC IXH */
				IX += 0x100;
				AF = (AF & ~0xfe) | incZ80Table[HIGH_REGISTER(IX)];
				break;

			case 0x25:      /* DEC IXH */
				IX -= 0x100;
				AF = (AF & ~0xfe) | decZ80Table[HIGH_REGISTER(IX)];
				break;

			case 0x26:      /* LD IXH,nn */
				SET_HIGH_REGISTER(IX, RAM_PP(PC));
				break;

			case 0x29:      /* ADD IX,IX */
				IX &= ADDRMASK;
				sum = IX + IX;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[sum >> 8];
				IX = sum;
				break;

			case 0x2a:      /* LD IX,(nnnn) */
				IX = GET_WORD(GET_WORD(PC++));
				++PC;
				break;

			case 0x2b:      /* DEC IX */
				--IX;
				break;

			case 0x2c:      /* INC IXL */
				temp = LOW_REGISTER(IX) + 1;
				SET_LOW_REGISTER(IX, temp);
				AF = (AF & ~0xfe) | incZ80Table[temp];
				break;

			case 0x2d:      /* DEC IXL */
				temp = LOW_REGISTER(IX) - 1;
				SET_LOW_REGISTER(IX, temp);
				AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
				break;

			case 0x2e:      /* LD IXL,nn */
				SET_LOW_REGISTER(IX, RAM_PP(PC));
				break;

			case 0x34:      /* INC (IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr) + 1;
				PUT_BYTE(adr, temp);
				AF = (AF & ~0xfe) | incZ80Table[temp];
				break;

			case 0x35:      /* DEC (IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr) - 1;
				PUT_BYTE(adr, temp);
				AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
				break;

			case 0x36:      /* LD (IX+dd),nn */
				adr = IX + (int8)RAM_PP(PC);
				PUT_BYTE(adr, RAM_PP(PC));
				break;

			case 0x39:      /* ADD IX,SP */
				IX &= ADDRMASK;
				SP &= ADDRMASK;
				sum = IX + SP;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(IX ^ SP ^ sum) >> 8];
				IX = sum;
				break;

			case 0x44:      /* LD B,IXH */
				SET_HIGH_REGISTER(BC, HIGH_REGISTER(IX));
				break;

			case 0x45:      /* LD B,IXL */
				SET_HIGH_REGISTER(BC, LOW_REGISTER(IX));
				break;

			case 0x46:      /* LD B,(IX+dd) */
				SET_HIGH_REGISTER(BC, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x4c:      /* LD C,IXH */
				SET_LOW_REGISTER(BC, HIGH_REGISTER(IX));
				break;

			case 0x4d:      /* LD C,IXL */
				SET_LOW_REGISTER(BC, LOW_REGISTER(IX));
				break;

			case 0x4e:      /* LD C,(IX+dd) */
				SET_LOW_REGISTER(BC, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x54:      /* LD D,IXH */
				SET_HIGH_REGISTER(DE, HIGH_REGISTER(IX));
				break;

			case 0x55:      /* LD D,IXL */
				SET_HIGH_REGISTER(DE, LOW_REGISTER(IX));
				break;

			case 0x56:      /* LD D,(IX+dd) */
				SET_HIGH_REGISTER(DE, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x5c:      /* LD E,IXH */
				SET_LOW_REGISTER(DE, HIGH_REGISTER(IX));
				break;

			case 0x5d:      /* LD E,IXL */
				SET_LOW_REGISTER(DE, LOW_REGISTER(IX));
				break;

			case 0x5e:      /* LD E,(IX+dd) */
				SET_LOW_REGISTER(DE, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x60:      /* LD IXH,B */
				SET_HIGH_REGISTER(IX, HIGH_REGISTER(BC));
				break;

			case 0x61:      /* LD IXH,C */
				SET_HIGH_REGISTER(IX, LOW_REGISTER(BC));
				break;

			case 0x62:      /* LD IXH,D */
				SET_HIGH_REGISTER(IX, HIGH_REGISTER(DE));
				break;

			case 0x63:      /* LD IXH,E */
				SET_HIGH_REGISTER(IX, LOW_REGISTER(DE));
				break;

			case 0x64:      /* LD IXH,IXH */
				break;

			case 0x65:      /* LD IXH,IXL */
				SET_HIGH_REGISTER(IX, LOW_REGISTER(IX));
				break;

			case 0x66:      /* LD H,(IX+dd) */
				SET_HIGH_REGISTER(HL, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x67:      /* LD IXH,A */
				SET_HIGH_REGISTER(IX, HIGH_REGISTER(AF));
				break;

			case 0x68:      /* LD IXL,B */
				SET_LOW_REGISTER(IX, HIGH_REGISTER(BC));
				break;

			case 0x69:      /* LD IXL,C */
				SET_LOW_REGISTER(IX, LOW_REGISTER(BC));
				break;

			case 0x6a:      /* LD IXL,D */
				SET_LOW_REGISTER(IX, HIGH_REGISTER(DE));
				break;

			case 0x6b:      /* LD IXL,E */
				SET_LOW_REGISTER(IX, LOW_REGISTER(DE));
				break;

			case 0x6c:      /* LD IXL,IXH */
				SET_LOW_REGISTER(IX, HIGH_REGISTER(IX));
				break;

			case 0x6d:      /* LD IXL,IXL */
				break;

			case 0x6e:      /* LD L,(IX+dd) */
				SET_LOW_REGISTER(HL, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x6f:      /* LD IXL,A */
				SET_LOW_REGISTER(IX, HIGH_REGISTER(AF));
				break;

			case 0x70:      /* LD (IX+dd),B */
				PUT_BYTE(IX + (int8)RAM_PP(PC), HIGH_REGISTER(BC));
				break;

			case 0x71:      /* LD (IX+dd),C */
				PUT_BYTE(IX + (int8)RAM_PP(PC), LOW_REGISTER(BC));
				break;

			case 0x72:      /* LD (IX+dd),D */
				PUT_BYTE(IX + (int8)RAM_PP(PC), HIGH_REGISTER(DE));
				break;

			case 0x73:      /* LD (IX+dd),E */
				PUT_BYTE(IX + (int8)RAM_PP(PC), LOW_REGISTER(DE));
				break;

			case 0x74:      /* LD (IX+dd),H */
				PUT_BYTE(IX + (int8)RAM_PP(PC), HIGH_REGISTER(HL));
				break;

			case 0x75:      /* LD (IX+dd),L */
				PUT_BYTE(IX + (int8)RAM_PP(PC), LOW_REGISTER(HL));
				break;

			case 0x77:      /* LD (IX+dd),A */
				PUT_BYTE(IX + (int8)RAM_PP(PC), HIGH_REGISTER(AF));
				break;

			case 0x7c:      /* LD A,IXH */
				SET_HIGH_REGISTER(AF, HIGH_REGISTER(IX));
				break;

			case 0x7d:      /* LD A,IXL */
				SET_HIGH_REGISTER(AF, LOW_REGISTER(IX));
				break;

			case 0x7e:      /* LD A,(IX+dd) */
				SET_HIGH_REGISTER(AF, GET_BYTE(IX + (int8)RAM_PP(PC)));
				break;

			case 0x84:      /* ADD A,IXH */
				temp = HIGH_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x85:      /* ADD A,IXL */
				temp = LOW_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x86:      /* ADD A,(IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x8c:      /* ADC A,IXH */
				temp = HIGH_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp + TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x8d:      /* ADC A,IXL */
				temp = LOW_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp + TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x8e:      /* ADC A,(IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp + TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x96:      /* SUB (IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0x94:      /* SUB IXH */
				SETFLAG(C, 0);/* fall through, a bit less efficient but smaller code */

			case 0x9c:      /* SBC A,IXH */
				temp = HIGH_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp - TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0x95:      /* SUB IXL */
				SETFLAG(C, 0);/* fall through, a bit less efficient but smaller code */

			case 0x9d:      /* SBC A,IXL */
				temp = LOW_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp - TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0x9e:      /* SBC A,(IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp - TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xa4:      /* AND IXH */
				AF = (xororTable[((AF & IX) >> 8) & 0xff] | 0x10);
				break;

			case 0xa5:      /* AND IXL */
				AF = (xororTable[((AF >> 8)& IX) & 0xff] | 0x10);
				break;

			case 0xa6:      /* AND (IX+dd) */
				AF = (xororTable[((AF >> 8)& GET_BYTE(IX + (int8)RAM_PP(PC))) & 0xff] | 0x10);
				break;

			case 0xac:      /* XOR IXH */
				AF = xororTable[((AF ^ IX) >> 8) & 0xff];
				break;

			case 0xad:      /* XOR IXL */
				AF = xororTable[((AF >> 8) ^ IX) & 0xff];
				break;

			case 0xae:      /* XOR (IX+dd) */
				AF = xororTable[((AF >> 8) ^ GET_BYTE(IX + (int8)RAM_PP(PC))) & 0xff];
				break;

			case 0xb4:      /* OR IXH */
				AF = xororTable[((AF | IX) >> 8) & 0xff];
				break;

			case 0xb5:      /* OR IXL */
				AF = xororTable[((AF >> 8) | IX) & 0xff];
				break;

			case 0xb6:      /* OR (IX+dd) */
				AF = xororTable[((AF >> 8) | GET_BYTE(IX + (int8)RAM_PP(PC))) & 0xff];
				break;

			case 0xbc:      /* CP IXH */
				temp = HIGH_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
					cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xbd:      /* CP IXL */
				temp = LOW_REGISTER(IX);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
					cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xbe:      /* CP (IX+dd) */
				adr = IX + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
					cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xcb:      /* CB prefix */
				adr = IX + (int8)RAM_PP(PC);
				switch ((op = GET_BYTE(PC)) & 7) {

				case 0:
					acu = HIGH_REGISTER(BC);
					break;

				case 1:
					acu = LOW_REGISTER(BC);
					break;

				case 2:
					acu = HIGH_REGISTER(DE);
					break;

				case 3:
					acu = LOW_REGISTER(DE);
					break;

				case 4:
					acu = HIGH_REGISTER(HL);
					break;

				case 5:
					acu = LOW_REGISTER(HL);
					break;

				case 6:
					acu = GET_BYTE(adr);
					break;

				default:
					acu = HIGH_REGISTER(AF);
					break;
				}
				++PC;
				switch (op & 0xc0) {

				case 0x00:  /* shift/rotate */
					switch (op & 0x38) {

						case 0x00:/* RLC */
							temp = (acu << 1) | (acu >> 7);
							cbits = temp & 1;
							break;

						case 0x08:/* RRC */
							temp = (acu >> 1) | (acu << 7);
							cbits = temp & 0x80;
							break;

						case 0x10:/* RL */
							temp = (acu << 1) | TSTFLAG(C);
							cbits = acu & 0x80;
							break;

						case 0x18:/* RR */
							temp = (acu >> 1) | (TSTFLAG(C) << 7);
							cbits = acu & 1;
							break;

						case 0x20:/* SLA */
							temp = acu << 1;
							cbits = acu & 0x80;
							break;

						case 0x28:/* SRA */
							temp = (acu >> 1) | (acu & 0x80);
							cbits = acu & 1;
							break;

						case 0x30:/* SLIA */
							temp = (acu << 1) | 1;
							cbits = acu & 0x80;
							break;

						case 0x38:/* SRL */
							temp = acu >> 1;
							cbits = acu & 1;
							break;

						default:
							temp = acu;
							cbits = 0;
					}
					AF = (AF & ~0xff) | rotateShiftTable[temp & 0xff] | !!cbits;
					break;

				case 0x40:  /* BIT */
					if (acu & (1 << ((op >> 3) & 7)))
						AF = (AF & ~0xfe) | 0x10 | (((op & 0x38) == 0x38) << 7);
					else
						AF = (AF & ~0xfe) | 0x54;
					if ((op & 7) != 6)
						AF |= (acu & 0x28);
					temp = acu;
					break;

				case 0x80:  /* RES */
					temp = acu & ~(1 << ((op >> 3) & 7));
					break;

				case 0xc0:  /* SET */
					temp = acu | (1 << ((op >> 3) & 7));
					break;
				}
				switch (op & 7) {

				case 0:
					SET_HIGH_REGISTER(BC, temp);
					break;

				case 1:
					SET_LOW_REGISTER(BC, temp);
					break;

				case 2:
					SET_HIGH_REGISTER(DE, temp);
					break;

				case 3:
					SET_LOW_REGISTER(DE, temp);
					break;

				case 4:
					SET_HIGH_REGISTER(HL, temp);
					break;

				case 5:
					SET_LOW_REGISTER(HL, temp);
					break;

				case 6:
					PUT_BYTE(adr, temp);
					break;

				default:
					SET_HIGH_REGISTER(AF, temp);
					break;
				}
				break;

			case 0xe1:      /* POP IX */
				POP(IX);
				break;

			case 0xe3:      /* EX (SP),IX */
				temp = IX;
				POP(IX);
				PUSH(temp);
				break;

			case 0xe5:      /* PUSH IX */
				PUSH(IX);
				break;

			case 0xe9:      /* JP (IX) */
				PC = IX;
				break;

			case 0xf9:      /* LD SP,IX */
				SP = IX;
				break;

			default:                /* ignore DD */
				--PC;
			}
			break;

		case 0xde:          /* SBC A,nn */
			temp = RAM_PP(PC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp - TSTFLAG(C);
			cbits = acu ^ temp ^ sum;
			AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[cbits & 0x1ff];
			break;

		case 0xdf:      /* RST 18H */
			PUSH(PC);
			PC = 0x18;
			break;

		case 0xe0:      /* RET PO */
			if (!(TSTFLAG(P)))
				POP(PC);
			break;

		case 0xe1:      /* POP HL */
			POP(HL);
			break;

		case 0xe2:      /* JP PO,nnnn */
			JPC(!TSTFLAG(P));
			break;

		case 0xe3:      /* EX (SP),HL */
			temp = HL;
			POP(HL);
			PUSH(temp);
			break;

		case 0xe4:      /* CALL PO,nnnn */
			CALLC(!TSTFLAG(P));
			break;

		case 0xe5:      /* PUSH HL */
			PUSH(HL);
			break;

		case 0xe6:      /* AND nn */
			AF = (xororTable[((AF >> 8)& RAM_PP(PC)) & 0xff] | 0x10);
			break;

		case 0xe7:      /* RST 20H */
			PUSH(PC);
			PC = 0x20;
			break;

		case 0xe8:      /* RET PE */
			if (TSTFLAG(P))
				POP(PC);
			break;

		case 0xe9:      /* JP (HL) */
			PC = HL;
			break;

		case 0xea:      /* JP PE,nnnn */
			JPC(TSTFLAG(P));
			break;

		case 0xeb:      /* EX DE,HL */
			HL ^= DE;
			DE ^= HL;
			HL ^= DE;
			break;

		case 0xec:      /* CALL PE,nnnn */
			CALLC(TSTFLAG(P));
			break;

		case 0xed:      /* ED prefix */
			INCR(1); /* Add one M1 cycle to refresh counter */
			switch (RAM_PP(PC)) {

			case 0x40:      /* IN B,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_HIGH_REGISTER(BC, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x41:      /* OUT (C),B */
				cpu_out(LOW_REGISTER(BC), HIGH_REGISTER(BC));
				break;

			case 0x42:      /* SBC HL,BC */
				HL &= ADDRMASK;
				BC &= ADDRMASK;
				sum = HL - BC - TSTFLAG(C);
				AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
					cbits2Z80Table[((HL ^ BC ^ sum) >> 8) & 0x1ff];
				HL = sum;
				break;

			case 0x43:      /* LD (nnnn),BC */
				PUT_WORD(GET_WORD(PC++), BC);
				++PC;
				break;

			case 0x44:      /* NEG */

			case 0x4C:      /* NEG, unofficial */

			case 0x54:      /* NEG, unofficial */

			case 0x5C:      /* NEG, unofficial */

			case 0x64:      /* NEG, unofficial */

			case 0x6C:      /* NEG, unofficial */

			case 0x74:      /* NEG, unofficial */

			case 0x7C:      /* NEG, unofficial */
				temp = HIGH_REGISTER(AF);
				AF = ((~(AF & 0xff00) + 1) & 0xff00); /* AF = (-(AF & 0xff00) & 0xff00); */
				AF |= ((AF >> 8) & 0xa8) | (((AF & 0xff00) == 0) << 6) | negTable[temp];
				break;

			case 0x45:      /* RETN */

			case 0x55:      /* RETN, unofficial */

			case 0x5D:      /* RETN, unofficial */

			case 0x65:      /* RETN, unofficial */

			case 0x6D:      /* RETN, unofficial */

			case 0x75:      /* RETN, unofficial */

			case 0x7D:      /* RETN, unofficial */
				IFF |= IFF >> 1;
				POP(PC);
				break;

			case 0x46:      /* IM 0 */
							/* interrupt mode 0 */
				break;

			case 0x47:      /* LD I,A */
				IR = (IR & 0xff) | (AF & ~0xff);
				break;

			case 0x48:      /* IN C,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_LOW_REGISTER(BC, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x49:      /* OUT (C),C */
				cpu_out(LOW_REGISTER(BC), LOW_REGISTER(BC));
				break;

			case 0x4a:      /* ADC HL,BC */
				HL &= ADDRMASK;
				BC &= ADDRMASK;
				sum = HL + BC + TSTFLAG(C);
				AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
					cbitsZ80Table[(HL ^ BC ^ sum) >> 8];
				HL = sum;
				break;

			case 0x4b:      /* LD BC,(nnnn) */
				BC = GET_WORD(GET_WORD(PC++));
				++PC;
				break;

			case 0x4d:      /* RETI */
				IFF |= IFF >> 1;
				POP(PC);
				break;

			case 0x4f:      /* LD R,A */
				IR = (IR & ~0xff) | ((AF >> 8) & 0xff);
				break;

			case 0x50:      /* IN D,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_HIGH_REGISTER(DE, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x51:      /* OUT (C),D */
				cpu_out(LOW_REGISTER(BC), HIGH_REGISTER(DE));
				break;

			case 0x52:      /* SBC HL,DE */
				HL &= ADDRMASK;
				DE &= ADDRMASK;
				sum = HL - DE - TSTFLAG(C);
				AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
					cbits2Z80Table[((HL ^ DE ^ sum) >> 8) & 0x1ff];
				HL = sum;
				break;

			case 0x53:      /* LD (nnnn),DE */
				PUT_WORD(GET_WORD(PC++), DE);
				++PC;
				break;

			case 0x56:      /* IM 1 */
							/* interrupt mode 1 */
				break;

			case 0x57:      /* LD A,I */
				AF = (AF & 0x29) | (IR & ~0xff) | ((IR >> 8) & 0x80) | (((IR & ~0xff) == 0) << 6) | ((IFF & 2) << 1);
				break;

			case 0x58:      /* IN E,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_LOW_REGISTER(DE, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x59:      /* OUT (C),E */
				cpu_out(LOW_REGISTER(BC), LOW_REGISTER(DE));
				break;

			case 0x5a:      /* ADC HL,DE */
				HL &= ADDRMASK;
				DE &= ADDRMASK;
				sum = HL + DE + TSTFLAG(C);
				AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
					cbitsZ80Table[(HL ^ DE ^ sum) >> 8];
				HL = sum;
				break;

			case 0x5b:      /* LD DE,(nnnn) */
				DE = GET_WORD(GET_WORD(PC++));
				++PC;
				break;

			case 0x5e:      /* IM 2 */
							/* interrupt mode 2 */
				break;

			case 0x5f:      /* LD A,R */
				AF = (AF & 0x29) | ((IR & 0xff) << 8) | (IR & 0x80) |
					(((IR & 0xff) == 0) << 6) | ((IFF & 2) << 1);
				break;

			case 0x60:      /* IN H,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_HIGH_REGISTER(HL, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x61:      /* OUT (C),H */
				cpu_out(LOW_REGISTER(BC), HIGH_REGISTER(HL));
				break;

			case 0x62:      /* SBC HL,HL */
				HL &= ADDRMASK;
				sum = HL - HL - TSTFLAG(C);
				AF = (AF & ~0xff) | (((sum & ADDRMASK) == 0) << 6) |
					cbits2Z80DupTable[(sum >> 8) & 0x1ff];
				HL = sum;
				break;

			case 0x63:      /* LD (nnnn),HL */
				PUT_WORD(GET_WORD(PC++), HL);
				++PC;
				break;

			case 0x67:      /* RRD */
				temp = GET_BYTE(HL);
				acu = HIGH_REGISTER(AF);
				PUT_BYTE(HL, HIGH_DIGIT(temp) | (LOW_DIGIT(acu) << 4));
				AF = xororTable[(acu & 0xf0) | LOW_DIGIT(temp)] | (AF & 1);
				break;

			case 0x68:      /* IN L,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_LOW_REGISTER(HL, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x69:      /* OUT (C),L */
				cpu_out(LOW_REGISTER(BC), LOW_REGISTER(HL));
				break;

			case 0x6a:      /* ADC HL,HL */
				HL &= ADDRMASK;
				sum = HL + HL + TSTFLAG(C);
				AF = (AF & ~0xff) | (((sum & ADDRMASK) == 0) << 6) |
					cbitsZ80DupTable[sum >> 8];
				HL = sum;
				break;

			case 0x6b:      /* LD HL,(nnnn) */
				HL = GET_WORD(GET_WORD(PC++));
				++PC;
				break;

			case 0x6f:      /* RLD */
				temp = GET_BYTE(HL);
				acu = HIGH_REGISTER(AF);
				PUT_BYTE(HL, (LOW_DIGIT(temp) << 4) | LOW_DIGIT(acu));
				AF = xororTable[(acu & 0xf0) | HIGH_DIGIT(temp)] | (AF & 1);
				break;

			case 0x70:      /* IN (C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_LOW_REGISTER(temp, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x71:      /* OUT (C),0 */
				cpu_out(LOW_REGISTER(BC), 0);
				break;

			case 0x72:      /* SBC HL,SP */
				HL &= ADDRMASK;
				SP &= ADDRMASK;
				sum = HL - SP - TSTFLAG(C);
				AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
					cbits2Z80Table[((HL ^ SP ^ sum) >> 8) & 0x1ff];
				HL = sum;
				break;

			case 0x73:      /* LD (nnnn),SP */
				PUT_WORD(GET_WORD(PC++), SP);
				++PC;
				break;

			case 0x78:      /* IN A,(C) */
				temp = cpu_in(LOW_REGISTER(BC));
				SET_HIGH_REGISTER(AF, temp);
				AF = (AF & ~0xfe) | rotateShiftTable[temp & 0xff];
				break;

			case 0x79:      /* OUT (C),A */
				cpu_out(LOW_REGISTER(BC), HIGH_REGISTER(AF));
				break;

			case 0x7a:      /* ADC HL,SP */
				HL &= ADDRMASK;
				SP &= ADDRMASK;
				sum = HL + SP + TSTFLAG(C);
				AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) | (((sum & ADDRMASK) == 0) << 6) |
					cbitsZ80Table[(HL ^ SP ^ sum) >> 8];
				HL = sum;
				break;

			case 0x7b:      /* LD SP,(nnnn) */
				SP = GET_WORD(GET_WORD(PC++));
				++PC;
				break;

			case 0xa0:      /* LDI */
				acu = RAM_PP(HL);
				PUT_BYTE_PP(DE, acu);
				acu += HIGH_REGISTER(AF);
				AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) |
					(((--BC & ADDRMASK) != 0) << 2);
				break;

			case 0xa1:      /* CPI */
				acu = HIGH_REGISTER(AF);
				temp = RAM_PP(HL);
				sum = acu - temp;
				cbits = acu ^ temp ^ sum;
				AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
					(((sum - ((cbits & 16) >> 4)) & 2) << 4) | (cbits & 16) |
					((sum - ((cbits >> 4) & 1)) & 8) |
					((--BC & ADDRMASK) != 0) << 2 | 2;
				if ((sum & 15) == 8 && (cbits & 16) != 0)
					AF &= ~8;
				break;

				/*  SF, ZF, YF, XF flags are affected by decreasing register B, as in DEC B.
				NF flag A is copy of bit 7 of the value read from or written to an I/O port.
				INI/INIR/IND/INDR use the C flag in stead of the L register. There is a
				catch though, because not the value of C is used, but C + 1 if it's INI/INIR or
				C - 1 if it's IND/INDR. So, first of all INI/INIR:
				HF and CF Both set if ((HL) + ((C + 1) & 255) > 255)
				PF The parity of (((HL) + ((C + 1) & 255)) & 7) xor B)                      */
			case 0xa2:      /* INI */
				acu = cpu_in(LOW_REGISTER(BC));
				PUT_BYTE(HL, acu);
				++HL;
				temp = HIGH_REGISTER(BC);
				BC -= 0x100;
				INOUTFLAGS_NONZERO((LOW_REGISTER(BC) + 1) & 0xff);
				break;

				/*  SF, ZF, YF, XF flags are affected by decreasing register B, as in DEC B.
				NF flag A is copy of bit 7 of the value read from or written to an I/O port.
				And now the for OUTI/OTIR/OUTD/OTDR instructions. Take state of the L
				after the increment or decrement of HL; add the value written to the I/O port
				to; call that k for now. If k > 255, then the CF and HF flags are set. The PF
				flags is set like the parity of k bitwise and'ed with 7, bitwise xor'ed with B.
				HF and CF Both set if ((HL) + L > 255)
				PF The parity of ((((HL) + L) & 7) xor B)                                       */
			case 0xa3:      /* OUTI */
				acu = GET_BYTE(HL);
				cpu_out(LOW_REGISTER(BC), acu);
				++HL;
				temp = HIGH_REGISTER(BC);
				BC -= 0x100;
				INOUTFLAGS_NONZERO(LOW_REGISTER(HL));
				break;

			case 0xa8:      /* LDD */
				acu = RAM_MM(HL);
				PUT_BYTE_MM(DE, acu);
				acu += HIGH_REGISTER(AF);
				AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) |
					(((--BC & ADDRMASK) != 0) << 2);
				break;

			case 0xa9:      /* CPD */
				acu = HIGH_REGISTER(AF);
				temp = RAM_MM(HL);
				sum = acu - temp;
				cbits = acu ^ temp ^ sum;
				AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
					(((sum - ((cbits & 16) >> 4)) & 2) << 4) | (cbits & 16) |
					((sum - ((cbits >> 4) & 1)) & 8) |
					((--BC & ADDRMASK) != 0) << 2 | 2;
				if ((sum & 15) == 8 && (cbits & 16) != 0)
					AF &= ~8;
				break;

				/*  SF, ZF, YF, XF flags are affected by decreasing register B, as in DEC B.
				NF flag A is copy of bit 7 of the value read from or written to an I/O port.
				INI/INIR/IND/INDR use the C flag in stead of the L register. There is a
				catch though, because not the value of C is used, but C + 1 if it's INI/INIR or
				C - 1 if it's IND/INDR. And last IND/INDR:
				HF and CF Both set if ((HL) + ((C - 1) & 255) > 255)
				PF The parity of (((HL) + ((C - 1) & 255)) & 7) xor B)                      */
			case 0xaa:      /* IND */
				acu = cpu_in(LOW_REGISTER(BC));
				PUT_BYTE(HL, acu);
				--HL;
				temp = HIGH_REGISTER(BC);
				BC -= 0x100;
				INOUTFLAGS_NONZERO((LOW_REGISTER(BC) - 1) & 0xff);
				break;

			case 0xab:      /* OUTD */
				acu = GET_BYTE(HL);
				cpu_out(LOW_REGISTER(BC), acu);
				--HL;
				temp = HIGH_REGISTER(BC);
				BC -= 0x100;
				INOUTFLAGS_NONZERO(LOW_REGISTER(HL));
				break;

			case 0xb0:      /* LDIR */
				BC &= ADDRMASK;
				if (BC == 0)
					BC = 0x10000;
				do {
					INCR(2); /* Add two M1 cycles to refresh counter */
					acu = RAM_PP(HL);
					PUT_BYTE_PP(DE, acu);
				} while (--BC);
				acu += HIGH_REGISTER(AF);
				AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4);
				break;

			case 0xb1:      /* CPIR */
				acu = HIGH_REGISTER(AF);
				BC &= ADDRMASK;
				if (BC == 0)
					BC = 0x10000;
				do {
					INCR(1); /* Add one M1 cycle to refresh counter */
					temp = RAM_PP(HL);
					op = --BC != 0;
					sum = acu - temp;
				} while (op && sum != 0);
				cbits = acu ^ temp ^ sum;
				AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
					(((sum - ((cbits & 16) >> 4)) & 2) << 4) |
					(cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |
					op << 2 | 2;
				if ((sum & 15) == 8 && (cbits & 16) != 0)
					AF &= ~8;
				break;

			case 0xb2:      /* INIR */
				temp = HIGH_REGISTER(BC);
				if (temp == 0)
					temp = 0x100;
				do {
					INCR(1); /* Add one M1 cycle to refresh counter */
					acu = cpu_in(LOW_REGISTER(BC));
					PUT_BYTE(HL, acu);
					++HL;
				} while (--temp);
				temp = HIGH_REGISTER(BC);
				SET_HIGH_REGISTER(BC, 0);
				INOUTFLAGS_ZERO((LOW_REGISTER(BC) + 1) & 0xff);
				break;

			case 0xb3:      /* OTIR */
				temp = HIGH_REGISTER(BC);
				if (temp == 0)
					temp = 0x100;
				do {
					INCR(1); /* Add one M1 cycle to refresh counter */
					acu = GET_BYTE(HL);
					cpu_out(LOW_REGISTER(BC), acu);
					++HL;
				} while (--temp);
				temp = HIGH_REGISTER(BC);
				SET_HIGH_REGISTER(BC, 0);
				INOUTFLAGS_ZERO(LOW_REGISTER(HL));
				break;

			case 0xb8:      /* LDDR */
				BC &= ADDRMASK;
				if (BC == 0)
					BC = 0x10000;
				do {
					INCR(2); /* Add two M1 cycles to refresh counter */
					acu = RAM_MM(HL);
					PUT_BYTE_MM(DE, acu);
				} while (--BC);
				acu += HIGH_REGISTER(AF);
				AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4);
				break;

			case 0xb9:      /* CPDR */
				acu = HIGH_REGISTER(AF);
				BC &= ADDRMASK;
				if (BC == 0)
					BC = 0x10000;
				do {
					INCR(1); /* Add one M1 cycle to refresh counter */
					temp = RAM_MM(HL);
					op = --BC != 0;
					sum = acu - temp;
				} while (op && sum != 0);
				cbits = acu ^ temp ^ sum;
				AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
					(((sum - ((cbits & 16) >> 4)) & 2) << 4) |
					(cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |
					op << 2 | 2;
				if ((sum & 15) == 8 && (cbits & 16) != 0)
					AF &= ~8;
				break;

			case 0xba:      /* INDR */
				temp = HIGH_REGISTER(BC);
				if (temp == 0)
					temp = 0x100;
				do {
					INCR(1); /* Add one M1 cycle to refresh counter */
					acu = cpu_in(LOW_REGISTER(BC));
					PUT_BYTE(HL, acu);
					--HL;
				} while (--temp);
				temp = HIGH_REGISTER(BC);
				SET_HIGH_REGISTER(BC, 0);
				INOUTFLAGS_ZERO((LOW_REGISTER(BC) - 1) & 0xff);
				break;

			case 0xbb:      /* OTDR */
				temp = HIGH_REGISTER(BC);
				if (temp == 0)
					temp = 0x100;
				do {
					INCR(1); /* Add one M1 cycle to refresh counter */
					acu = GET_BYTE(HL);
					cpu_out(LOW_REGISTER(BC), acu);
					--HL;
				} while (--temp);
				temp = HIGH_REGISTER(BC);
				SET_HIGH_REGISTER(BC, 0);
				INOUTFLAGS_ZERO(LOW_REGISTER(HL));
				break;

			default:    /* ignore ED and following byte */
				break;
			}
			break;

		case 0xee:      /* XOR nn */
			AF = xororTable[((AF >> 8) ^ RAM_PP(PC)) & 0xff];
			break;

		case 0xef:      /* RST 28H */
			PUSH(PC);
			PC = 0x28;
			break;

		case 0xf0:      /* RET P */
			if (!(TSTFLAG(S)))
				POP(PC);
			break;

		case 0xf1:      /* POP AF */
			POP(AF);
			break;

		case 0xf2:      /* JP P,nnnn */
			JPC(!TSTFLAG(S));
			break;

		case 0xf3:      /* DI */
			IFF = 0;
			break;

		case 0xf4:      /* CALL P,nnnn */
			CALLC(!TSTFLAG(S));
			break;

		case 0xf5:      /* PUSH AF */
			PUSH(AF);
			break;

		case 0xf6:      /* OR nn */
			AF = xororTable[((AF >> 8) | RAM_PP(PC)) & 0xff];
			break;

		case 0xf7:      /* RST 30H */
			PUSH(PC);
			PC = 0x30;
			break;

		case 0xf8:      /* RET M */
			if (TSTFLAG(S))
				POP(PC);
			break;

		case 0xf9:      /* LD SP,HL */
			SP = HL;
			break;

		case 0xfa:      /* JP M,nnnn */
			JPC(TSTFLAG(S));
			break;

		case 0xfb:      /* EI */
			IFF = 3;
			break;

		case 0xfc:      /* CALL M,nnnn */
			CALLC(TSTFLAG(S));
			break;

		case 0xfd:      /* FD prefix */
			INCR(1); /* Add one M1 cycle to refresh counter */
			switch (RAM_PP(PC)) {

			case 0x09:      /* ADD IY,BC */
				IY &= ADDRMASK;
				BC &= ADDRMASK;
				sum = IY + BC;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(IY ^ BC ^ sum) >> 8];
				IY = sum;
				break;

			case 0x19:      /* ADD IY,DE */
				IY &= ADDRMASK;
				DE &= ADDRMASK;
				sum = IY + DE;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(IY ^ DE ^ sum) >> 8];
				IY = sum;
				break;

			case 0x21:      /* LD IY,nnnn */
				IY = GET_WORD(PC++);
				++PC;
				break;

			case 0x22:      /* LD (nnnn),IY */
				temp = GET_WORD(PC++);
				PUT_WORD(temp, IY);
				++PC;
				break;

			case 0x23:      /* INC IY */
				++IY;
				break;

			case 0x24:      /* INC IYH */
				IY += 0x100;
				AF = (AF & ~0xfe) | incZ80Table[HIGH_REGISTER(IY)];
				break;

			case 0x25:      /* DEC IYH */
				IY -= 0x100;
				AF = (AF & ~0xfe) | decZ80Table[HIGH_REGISTER(IY)];
				break;

			case 0x26:      /* LD IYH,nn */
				SET_HIGH_REGISTER(IY, RAM_PP(PC));
				break;

			case 0x29:      /* ADD IY,IY */
				IY &= ADDRMASK;
				sum = IY + IY;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[sum >> 8];
				IY = sum;
				break;

			case 0x2a:      /* LD IY,(nnnn) */
				IY = GET_WORD(GET_WORD(PC++));
				++PC;
				break;

			case 0x2b:      /* DEC IY */
				--IY;
				break;

			case 0x2c:      /* INC IYL */
				temp = LOW_REGISTER(IY) + 1;
				SET_LOW_REGISTER(IY, temp);
				AF = (AF & ~0xfe) | incZ80Table[temp];
				break;

			case 0x2d:      /* DEC IYL */
				temp = LOW_REGISTER(IY) - 1;
				SET_LOW_REGISTER(IY, temp);
				AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
				break;

			case 0x2e:      /* LD IYL,nn */
				SET_LOW_REGISTER(IY, RAM_PP(PC));
				break;

			case 0x34:      /* INC (IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr) + 1;
				PUT_BYTE(adr, temp);
				AF = (AF & ~0xfe) | incZ80Table[temp];
				break;

			case 0x35:      /* DEC (IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr) - 1;
				PUT_BYTE(adr, temp);
				AF = (AF & ~0xfe) | decZ80Table[temp & 0xff];
				break;

			case 0x36:      /* LD (IY+dd),nn */
				adr = IY + (int8)RAM_PP(PC);
				PUT_BYTE(adr, RAM_PP(PC));
				break;

			case 0x39:      /* ADD IY,SP */
				IY &= ADDRMASK;
				SP &= ADDRMASK;
				sum = IY + SP;
				AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) | cbitsTable[(IY ^ SP ^ sum) >> 8];
				IY = sum;
				break;

			case 0x44:      /* LD B,IYH */
				SET_HIGH_REGISTER(BC, HIGH_REGISTER(IY));
				break;

			case 0x45:      /* LD B,IYL */
				SET_HIGH_REGISTER(BC, LOW_REGISTER(IY));
				break;

			case 0x46:      /* LD B,(IY+dd) */
				SET_HIGH_REGISTER(BC, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x4c:      /* LD C,IYH */
				SET_LOW_REGISTER(BC, HIGH_REGISTER(IY));
				break;

			case 0x4d:      /* LD C,IYL */
				SET_LOW_REGISTER(BC, LOW_REGISTER(IY));
				break;

			case 0x4e:      /* LD C,(IY+dd) */
				SET_LOW_REGISTER(BC, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x54:      /* LD D,IYH */
				SET_HIGH_REGISTER(DE, HIGH_REGISTER(IY));
				break;

			case 0x55:      /* LD D,IYL */
				SET_HIGH_REGISTER(DE, LOW_REGISTER(IY));
				break;

			case 0x56:      /* LD D,(IY+dd) */
				SET_HIGH_REGISTER(DE, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x5c:      /* LD E,IYH */
				SET_LOW_REGISTER(DE, HIGH_REGISTER(IY));
				break;

			case 0x5d:      /* LD E,IYL */
				SET_LOW_REGISTER(DE, LOW_REGISTER(IY));
				break;

			case 0x5e:      /* LD E,(IY+dd) */
				SET_LOW_REGISTER(DE, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x60:      /* LD IYH,B */
				SET_HIGH_REGISTER(IY, HIGH_REGISTER(BC));
				break;

			case 0x61:      /* LD IYH,C */
				SET_HIGH_REGISTER(IY, LOW_REGISTER(BC));
				break;

			case 0x62:      /* LD IYH,D */
				SET_HIGH_REGISTER(IY, HIGH_REGISTER(DE));
				break;

			case 0x63:      /* LD IYH,E */
				SET_HIGH_REGISTER(IY, LOW_REGISTER(DE));
				break;

			case 0x64:      /* LD IYH,IYH */
				break;

			case 0x65:      /* LD IYH,IYL */
				SET_HIGH_REGISTER(IY, LOW_REGISTER(IY));
				break;

			case 0x66:      /* LD H,(IY+dd) */
				SET_HIGH_REGISTER(HL, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x67:      /* LD IYH,A */
				SET_HIGH_REGISTER(IY, HIGH_REGISTER(AF));
				break;

			case 0x68:      /* LD IYL,B */
				SET_LOW_REGISTER(IY, HIGH_REGISTER(BC));
				break;

			case 0x69:      /* LD IYL,C */
				SET_LOW_REGISTER(IY, LOW_REGISTER(BC));
				break;

			case 0x6a:      /* LD IYL,D */
				SET_LOW_REGISTER(IY, HIGH_REGISTER(DE));
				break;

			case 0x6b:      /* LD IYL,E */
				SET_LOW_REGISTER(IY, LOW_REGISTER(DE));
				break;

			case 0x6c:      /* LD IYL,IYH */
				SET_LOW_REGISTER(IY, HIGH_REGISTER(IY));
				break;

			case 0x6d:      /* LD IYL,IYL */
				break;

			case 0x6e:      /* LD L,(IY+dd) */
				SET_LOW_REGISTER(HL, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x6f:      /* LD IYL,A */
				SET_LOW_REGISTER(IY, HIGH_REGISTER(AF));
				break;

			case 0x70:      /* LD (IY+dd),B */
				PUT_BYTE(IY + (int8)RAM_PP(PC), HIGH_REGISTER(BC));
				break;

			case 0x71:      /* LD (IY+dd),C */
				PUT_BYTE(IY + (int8)RAM_PP(PC), LOW_REGISTER(BC));
				break;

			case 0x72:      /* LD (IY+dd),D */
				PUT_BYTE(IY + (int8)RAM_PP(PC), HIGH_REGISTER(DE));
				break;

			case 0x73:      /* LD (IY+dd),E */
				PUT_BYTE(IY + (int8)RAM_PP(PC), LOW_REGISTER(DE));
				break;

			case 0x74:      /* LD (IY+dd),H */
				PUT_BYTE(IY + (int8)RAM_PP(PC), HIGH_REGISTER(HL));
				break;

			case 0x75:      /* LD (IY+dd),L */
				PUT_BYTE(IY + (int8)RAM_PP(PC), LOW_REGISTER(HL));
				break;

			case 0x77:      /* LD (IY+dd),A */
				PUT_BYTE(IY + (int8)RAM_PP(PC), HIGH_REGISTER(AF));
				break;

			case 0x7c:      /* LD A,IYH */
				SET_HIGH_REGISTER(AF, HIGH_REGISTER(IY));
				break;

			case 0x7d:      /* LD A,IYL */
				SET_HIGH_REGISTER(AF, LOW_REGISTER(IY));
				break;

			case 0x7e:      /* LD A,(IY+dd) */
				SET_HIGH_REGISTER(AF, GET_BYTE(IY + (int8)RAM_PP(PC)));
				break;

			case 0x84:      /* ADD A,IYH */
				temp = HIGH_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x85:      /* ADD A,IYL */
				temp = LOW_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x86:      /* ADD A,(IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x8c:      /* ADC A,IYH */
				temp = HIGH_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp + TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x8d:      /* ADC A,IYL */
				temp = LOW_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp + TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x8e:      /* ADC A,(IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu + temp + TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbitsZ80Table[acu ^ temp ^ sum];
				break;

			case 0x96:      /* SUB (IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0x94:      /* SUB IYH */
				SETFLAG(C, 0);/* fall through, a bit less efficient but smaller code */

			case 0x9c:      /* SBC A,IYH */
				temp = HIGH_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp - TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0x95:      /* SUB IYL */
				SETFLAG(C, 0);/* fall through, a bit less efficient but smaller code */

			case 0x9d:      /* SBC A,IYL */
				temp = LOW_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp - TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0x9e:      /* SBC A,(IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp - TSTFLAG(C);
				AF = (xororTable[sum & 0xff] & ~4) | cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xa4:      /* AND IYH */
				AF = (xororTable[((AF & IY) >> 8) & 0xff] | 0x10);
				break;

			case 0xa5:      /* AND IYL */
				AF = (xororTable[((AF >> 8)& IY) & 0xff] | 0x10);
				break;

			case 0xa6:      /* AND (IY+dd) */
				AF = (xororTable[((AF >> 8)& GET_BYTE(IY + (int8)RAM_PP(PC))) & 0xff] | 0x10);
				break;

			case 0xac:      /* XOR IYH */
				AF = xororTable[((AF ^ IY) >> 8) & 0xff];
				break;

			case 0xad:      /* XOR IYL */
				AF = xororTable[((AF >> 8) ^ IY) & 0xff];
				break;

			case 0xae:      /* XOR (IY+dd) */
				AF = xororTable[((AF >> 8) ^ GET_BYTE(IY + (int8)RAM_PP(PC))) & 0xff];
				break;

			case 0xb4:      /* OR IYH */
				AF = xororTable[((AF | IY) >> 8) & 0xff];
				break;

			case 0xb5:      /* OR IYL */
				AF = xororTable[((AF >> 8) | IY) & 0xff];
				break;

			case 0xb6:      /* OR (IY+dd) */
				AF = xororTable[((AF >> 8) | GET_BYTE(IY + (int8)RAM_PP(PC))) & 0xff];
				break;

			case 0xbc:      /* CP IYH */
				temp = HIGH_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
					cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xbd:      /* CP IYL */
				temp = LOW_REGISTER(IY);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
					cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xbe:      /* CP (IY+dd) */
				adr = IY + (int8)RAM_PP(PC);
				temp = GET_BYTE(adr);
				acu = HIGH_REGISTER(AF);
				sum = acu - temp;
				AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
					cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
				break;

			case 0xcb:      /* CB prefix */
				adr = IY + (int8)RAM_PP(PC);
				switch ((op = GET_BYTE(PC)) & 7) {

				case 0:
					acu = HIGH_REGISTER(BC);
					break;

				case 1:
					acu = LOW_REGISTER(BC);
					break;

				case 2:
					acu = HIGH_REGISTER(DE);
					break;

				case 3:
					acu = LOW_REGISTER(DE);
					break;

				case 4:
					acu = HIGH_REGISTER(HL);
					break;

				case 5:
					acu = LOW_REGISTER(HL);
					break;

				case 6:
					acu = GET_BYTE(adr);
					break;

				default:
					acu = HIGH_REGISTER(AF);
					break;
				}
				++PC;
				switch (op & 0xc0) {

				case 0x00:  /* shift/rotate */
					switch (op & 0x38) {

						case 0x00:/* RLC */
							temp = (acu << 1) | (acu >> 7);
							cbits = temp & 1;
							break;
	
						case 0x08:/* RRC */
							temp = (acu >> 1) | (acu << 7);
							cbits = temp & 0x80;
							break;
	
						case 0x10:/* RL */
							temp = (acu << 1) | TSTFLAG(C);
							cbits = acu & 0x80;
							break;
	
						case 0x18:/* RR */
							temp = (acu >> 1) | (TSTFLAG(C) << 7);
							cbits = acu & 1;
							break;
	
						case 0x20:/* SLA */
							temp = acu << 1;
							cbits = acu & 0x80;
							break;
	
						case 0x28:/* SRA */
							temp = (acu >> 1) | (acu & 0x80);
							cbits = acu & 1;
							break;
	
						case 0x30:/* SLIA */
							temp = (acu << 1) | 1;
							cbits = acu & 0x80;
							break;
	
						case 0x38:/* SRL */
							temp = acu >> 1;
							cbits = acu & 1;
							break;
	
						default:
							temp = acu;
							cbits = 0;
					}
					AF = (AF & ~0xff) | rotateShiftTable[temp & 0xff] | !!cbits;
					break;

				case 0x40:  /* BIT */
					if (acu & (1 << ((op >> 3) & 7)))
						AF = (AF & ~0xfe) | 0x10 | (((op & 0x38) == 0x38) << 7);
					else
						AF = (AF & ~0xfe) | 0x54;
					if ((op & 7) != 6)
						AF |= (acu & 0x28);
					temp = acu;
					break;

				case 0x80:  /* RES */
					temp = acu & ~(1 << ((op >> 3) & 7));
					break;

				case 0xc0:  /* SET */
					temp = acu | (1 << ((op >> 3) & 7));
					break;
				}
				switch (op & 7) {

				case 0:
					SET_HIGH_REGISTER(BC, temp);
					break;

				case 1:
					SET_LOW_REGISTER(BC, temp);
					break;

				case 2:
					SET_HIGH_REGISTER(DE, temp);
					break;

				case 3:
					SET_LOW_REGISTER(DE, temp);
					break;

				case 4:
					SET_HIGH_REGISTER(HL, temp);
					break;

				case 5:
					SET_LOW_REGISTER(HL, temp);
					break;

				case 6:
					PUT_BYTE(adr, temp);
					break;

				default:
					SET_HIGH_REGISTER(AF, temp);
					break;
				}
				break;

			case 0xe1:      /* POP IY */
				POP(IY);
				break;

			case 0xe3:      /* EX (SP),IY */
				temp = IY;
				POP(IY);
				PUSH(temp);
				break;

			case 0xe5:      /* PUSH IY */
				PUSH(IY);
				break;

			case 0xe9:      /* JP (IY) */
				PC = IY;
				break;

			case 0xf9:      /* LD SP,IY */
				SP = IY;
				break;

			default:            /* ignore FD */
				--PC;
			}
			break;

		case 0xfe:      /* CP nn */
			temp = RAM_PP(PC);
			acu = HIGH_REGISTER(AF);
			sum = acu - temp;
			AF = (AF & ~0xff) | cpTable[sum & 0xff] | (temp & 0x28) |
				cbits2Z80Table[(acu ^ temp ^ sum) & 0x1ff];
			break;

		case 0xff:      /* RST 38H */
			PUSH(PC);
			PC = 0x38;
		}
	}
}


#endif
