#ifndef DEBUG_H
#define DEBUG_H

/* Mnemonic tables for Z80 disassembly - shared by all CPU models */
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

#endif /* defined(DEBUG) || defined(iDEBUG) */

void watchprint(uint16 pos) {
    uint8 I, J;
    _puts("\r\n");
    _puts("  Watch : ");
    _puthex16(Watch);
    _puts(" = ");
    _puthex8(_RamRead(Watch));
    _putcon(':');
    _puthex8(_RamRead(Watch + 1));
    _puts(" / ");
    for (J = 0, I = _RamRead(Watch); J < 8; ++J, I <<= 1)
        _putcon(I & 0x80 ? '1' : '0');
    _putcon(':');
    for (J = 0, I = _RamRead(Watch + 1); J < 8; ++J, I <<= 1)
        _putcon(I & 0x80 ? '1' : '0');
}

void memdump(uint16 pos) {
    uint16 h = pos;
    uint16 c = pos;
    uint8 l, i;
    uint8 ch = pos & 0xff;

    _puts("       ");
    for (i = 0; i < 16; ++i) {
        _puthex8(ch++ & 0x0f);
        _puts(" ");
    }
    _puts("\r\n");
    _puts("       ");
    for (i = 0; i < 16; ++i)
        _puts("---");
    _puts("\r\n");
    for (l = 0; l < 16; ++l) {
        _puthex16(h);
        _puts(" : ");
        for (i = 0; i < 16; ++i) {
            _puthex8(_RamRead(h++));
            _puts(" ");
        }
        for (i = 0; i < 16; ++i) {
            ch = _RamRead(c++);
            _putcon(ch > 31 && ch < 127 ? ch : '.');
        }
        _puts("\r\n");
    }
}

static int read_hex8(uint8 *out) {
    unsigned int v = 0;
    int count = 0;
    int ch;

    /* Read characters until newline/carriage return */
    while (1) {
        ch = _getcon();
        /* End on CR or LF */
        if (ch == '\r' || ch == '\n')
            break;

        /* Backspace handling (BS=8, DEL=127) */
        if (ch == 8 || ch == 127) {
            if (count > 0) {
                /* erase last hex digit visually (basic backspace handling) */
                _putch('\b');
                _putch(' ');
                _putch('\b');
                v >>= 4;
                --count;
            }
            continue;
        }

        /* Accept 0-9 */
        if (ch >= '0' && ch <= '9') {
            if (count < 2) {
                v = (v << 4) | (unsigned int)(ch - '0');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept a-f */
        if (ch >= 'a' && ch <= 'f') {
            if (count < 2) {
                v = (v << 4) | (unsigned int)(10 + ch - 'a');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept A-F */
        if (ch >= 'A' && ch <= 'F') {
            if (count < 2) {
                v = (v << 4) | (unsigned int)(10 + ch - 'A');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Ignore 'x'/'X' to allow typing "0x..." */
        if (ch == 'x' || ch == 'X')
            continue;

        /* Ignore any other characters */
    }

    /* move to next line visually */
    _putch('\r');
    _putch('\n');

    if (count == 0)
        return 0; /* no digits entered */

    *out = (uint8)(v & 0xFFu);
    return 1;
}

static int read_hex16(uint16 *out) {
    unsigned int v = 0;
    int count = 0;
    int ch;

    /* Read characters until newline/carriage return */
    while (1) {
        ch = _getcon();
        /* End on CR or LF */
        if (ch == '\r' || ch == '\n')
            break;

        /* Backspace handling (BS=8, DEL=127) */
        if (ch == 8 || ch == 127) {
            if (count > 0) {
                /* erase last hex digit visually (basic backspace handling) */
                _putch('\b');
                _putch(' ');
                _putch('\b');
                v >>= 4;
                --count;
            }
            continue;
        }

        /* Accept 0-9 */
        if (ch >= '0' && ch <= '9') {
            if (count < 4) {
                v = (v << 4) | (unsigned int)(ch - '0');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept a-f */
        if (ch >= 'a' && ch <= 'f') {
            if (count < 4) {
                v = (v << 4) | (unsigned int)(10 + ch - 'a');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept A-F */
        if (ch >= 'A' && ch <= 'F') {
            if (count < 4) {
                v = (v << 4) | (unsigned int)(10 + ch - 'A');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Ignore 'x'/'X' to allow typing "0x..." */
        if (ch == 'x' || ch == 'X')
            continue;

        /* Ignore any other characters */
    }

    /* move to next line visually */
    _putch('\r');
    _putch('\n');

    if (count == 0)
        return 0; /* no digits entered */

    *out = (uint16)(v & 0xFFFFu);
    return 1;
}

/* Read opcode prefixes from memory at 'pos', advance pos to the
   first operand byte and return the mnemonic pointer. It also returns the
   initial byte-count (prefixes + opcode bytes consumed so far) via *countp
   and an optional prefix character ('X'/'Y' or 0) via *prefixp.

   Inputs:
         posp  - pointer to the position (will be advanced to operand start)
   Outputs:
         returns const char* mnemonic string (one of Mnemonics*, MnemonicsCB, ...)
         *countp set to initial consumed bytes (1 or more)
         *prefixp set to 'X' or 'Y' or 0 if applicable
*/
static const char *GetMnemonicAt(uint16 *posp, uint8 *countp, char *prefixp) {
    uint16 pos = *posp;
    uint8 ch = _RamRead(pos);
    uint8 count = 1;
    char C = 0;
    const char *txt;

    switch (ch) {
    case 0xCB:
        ++pos;
        txt = MnemonicsCB[_RamRead(pos++)];
        count++;
        break;
    case 0xED:
        ++pos;
        txt = MnemonicsED[_RamRead(pos++)];
        count++;
        break;
    case 0xDD:
        ++pos;
        C = 'X';
        if (_RamRead(pos) != 0xCB) {
            txt = MnemonicsXX[_RamRead(pos++)];
            ++count;
        } else {
            ++pos;
            txt = MnemonicsXCB[_RamRead(pos++)];
            count += 2;
        }
        break;
    case 0xFD:
        ++pos;
        C = 'Y';
        if (_RamRead(pos) != 0xCB) {
            txt = MnemonicsXX[_RamRead(pos++)];
            ++count;
        } else {
            ++pos;
            txt = MnemonicsXCB[_RamRead(pos++)];
            count += 2;
        }
        break;
    default:
        /* Normal opcode */
        txt = Mnemonics[_RamRead(pos++)];
        break;
    }

    *posp = pos;     /* advanced to first operand byte (if any) */
    *countp = count; /* consumed opcode/prefix bytes so far */
    if (prefixp)
        *prefixp = C;
    return txt;
}

/* InstructionLength - compute the number of bytes used by the instruction at pos */
static uint8 InstructionLength(uint16 pos) {
    uint8 count = 0;
    uint8 initial;
    char C = 0;
    const char *txt;

    /* Get mnemonic and advance pos to the operand area. initial counts prefixes/opcode */
    txt = GetMnemonicAt(&pos, &initial, &C);
    count = initial;

    /* Walk the mnemonic-format string to count operand bytes */
    while (*txt != 0) {
        switch (*txt) {
        case '*': /* one immediate byte */
        case '^': /* one immediate byte */
            txt += 2;
            ++count;
            ++pos;
            break;
        case '#': /* two-byte immediate (word) */
            txt += 2;
            count += 2;
            pos += 2;
            break;
        case '@': /* relative displacement (one byte) */
            txt += 2;
            ++count;
            ++pos;
            break;
        case '%': /* prefix placeholder in mnemonic text, no operand */
            ++txt;
            break;
        default:
            ++txt;
        }
    }

    return count;
}

/* TextLength - compute the length of the text representation of the instruction at pos */
static uint8 TextLength(uint16 pos) {
    uint8 len = 0;
    const char *txt = GetMnemonicAt(&pos, &len, NULL);
    len = 0;
    while (*txt != 0) {
        switch (*txt) {
        case '*':
        case '^':
            txt += 2;
            len += 2; // 1 byte + 2 hex digits
            break;
        case '#':
            txt += 2;
            len += 4; // 2 bytes + 2 hex digits
            break;
        case '@':
            txt += 2;
            len += 4; // 1 byte + 2 hex digits
            break;
        case '%':
            ++txt;
            ++len;
            break;
        default:
            ++txt;
            ++len;
        }
    }
    return len;
}

/* Disassemble instruction at given address */
uint8 Disasm(uint16 pos) {
    /* New Disasm: print full opcode byte column, then mnemonic. */
    const char *txt;
    uint8 Cflag = 0;
    uint8 len = InstructionLength(pos);
    uint16 op_pos = pos;
    uint8 initial = 0;

    /* Print opcode bytes (up to len) */
    for (uint8 i = 0; i < len; ++i) {
        _puthex8(_RamRead((pos + i) & 0xffff));
        _putch(' ');
    }

    /* pad bytes area to fixed column (use 3 chars per byte, target 12 chars) */
    int bytes_width = (int)len * 3;
    int target = 12; /* enough for up to 4 bytes */
    for (int s = bytes_width; s < target; ++s)
        _putch(' ');

    /* Get mnemonic template (advances a temporary pos to operand start) */
    txt = GetMnemonicAt(&op_pos, &initial, (char *)&Cflag);

    /* Now print mnemonic with formatted operands. When we encounter operand markers
       we consume bytes from op_pos (which points to first operand byte). */
    while (*txt != 0) {
        switch (*txt) {
        case '*':
        case '^': {
            /* single byte immediate */
            txt += 2;
            uint8 v = _RamRead(op_pos++);
            _puthex8(v);
            break;
        }
        case '#': {
            /* word immediate: print as 16-bit hex (little-endian) */
            txt += 2;
            uint8 lo = _RamRead(op_pos);
            uint8 hi = _RamRead(op_pos + 1);
            uint16 w = (uint16)lo | ((uint16)hi << 8);
            op_pos += 2;
            _puthex16(w);
            break;
        }
        case '@': {
            /* relative displacement - show target address */
            char jr = (char)_RamRead(op_pos++);
            uint16 target = (op_pos + jr) & 0xffff;
            txt += 2;
            _puthex16(target);
            break;
        }
        case '%':
            _putch((char)Cflag);
            ++txt;
            break;
        default:
            _putch(*txt);
            ++txt;
        }
    }

    return (len);
}

/* --- Simple instruction trace buffer and exec breakpoints ---
   Implemented inline here to avoid changing platform Makefiles.
   Trace records last N executed instructions (pc, bytes, len, reg snapshot).
*/
#define TRACE_CAPACITY 512
typedef struct {
    uint16 pc;
    uint8 len;
    uint8 bytes[8];
    int32 AF, BC, DE, HL, IX, IY, SP;
} trace_entry_t;

static trace_entry_t trace_buf[TRACE_CAPACITY];
static uint32 trace_pos = 0; /* next write index */
static int trace_enabled = 1;

static void __attribute__((unused)) z80_trace_push(uint16 pc) {
    if (!trace_enabled)
        return;
    trace_entry_t *e = &trace_buf[trace_pos++ % TRACE_CAPACITY];
    uint8 len = InstructionLength(pc);
    if (len > 8)
        len = 8;
    e->pc = pc;
    e->len = len;
    for (uint8 i = 0; i < len; ++i) {
        e->bytes[i] = _RamRead((pc + i) & 0xffff);
    }
    e->AF = AF;
    e->BC = BC;
    e->DE = DE;
    e->HL = HL;
    e->IX = IX;
    e->IY = IY;
    e->SP = SP;
}

void z80_print_flags(uint16 AF) {
    static const char Flags[9] = "SZ5H3PNC";
    uint8 J, I;
    _puts(" [");
    for (J = 0, I = LOW_REGISTER(AF); J < 8; ++J, I <<= 1)
        _putcon(I & 0x80 ? Flags[J] : '.');
    _puts("]");
}

static void z80_trace_dump(void) {
    uint32 start = trace_pos;
    uint32 i;
    uint8 len;
    _puts("\r\n--- Trace dump (most recent last) ---\r\n");
    for (i = 0; i < TRACE_CAPACITY; ++i) {
        trace_entry_t *e = &trace_buf[(start + i) % TRACE_CAPACITY];
        /* skip empty entries (pc==0 and len==0) */
        if (e->len == 0 && e->pc == 0)
            continue;
        _puthex16(e->pc);
        _puts(": ");
        /* print disassembly for this address */
        Disasm(e->pc);
        len = TextLength(e->pc);
        /* pad to fixed column (target 16 chars) */
        int target = 16;
        for (int s = len; s < target; ++s)
            _putch(' ');
        _puts("BC:");
        _puthex16(e->BC);
        _puts(" DE:");
        _puthex16(e->DE);
        _puts(" HL:");
        _puthex16(e->HL);
        _puts(" AF:");
        _puthex16(e->AF);
        z80_print_flags(e->AF);
        _puts(" SP:");
        _puthex16(e->SP);
        _puts("\r\n");
    }
    _puts("--- end trace ---\r\n");
}

/* Exec breakpoints: small fixed-size list. Only the bp_addrs[] list is used. */
#define MAX_BREAKPOINTS 32
static uint16 bp_addrs[MAX_BREAKPOINTS];
static int bp_count = 0;

static int z80_add_breakpoint(uint16 addr) {
    /* prevent duplicates */
    for (int i = 0; i < bp_count; ++i)
        if (bp_addrs[i] == addr)
            return -2;
    if (bp_count >= MAX_BREAKPOINTS)
        return -1;
    bp_addrs[bp_count++] = addr;
    return 0;
}

static void z80_clear_breakpoints(void) {
    bp_count = 0;
}

static int z80_remove_breakpoint(uint16 addr) {
    for (int i = 0; i < bp_count; ++i) {
        if (bp_addrs[i] == addr) {
            /* shift remaining */
            for (int j = i; j + 1 < bp_count; ++j)
                bp_addrs[j] = bp_addrs[j + 1];
            --bp_count;
            return 0;
        }
    }
    return -1; /* not found */
}

static int __attribute__((unused)) z80_check_breakpoints_on_exec(uint16 pc) {
    for (int i = 0; i < bp_count; ++i)
        if (bp_addrs[i] == pc)
            return 1;
    return 0;
}

void Z80debug(void) {
    uint8 ch = 0;
    uint16 pos, l;
    uint8 I;
    uint16 bpoint; /* changed from unsigned int to 16-bit */
    uint8 loop = TRUE;
    int res = 0; /* use a signed int for result checks */

    _puts("\r\nDebug Mode - Press '?' for help");

    while (loop && Debug) {
        pos = PC;
        _puts("\r\n");
        _puts("BC:");
        _puthex16(BC);
        _puts(" DE:");
        _puthex16(DE);
        _puts(" HL:");
        _puthex16(HL);
        _puts(" AF:");
        _puthex16(AF);
        _puts(" :");
        z80_print_flags(AF);
        _puts("\r\n");
        _puts("IX:");
        _puthex16(IX);
        _puts(" IY:");
        _puthex16(IY);
        _puts(" SP:");
        _puthex16(SP);
        _puts(" PC:");
        _puthex16(PC);
        _puts(" : ");

        Disasm(pos);

        if (PC == 0x0005) {
            if (LOW_REGISTER(BC) > 40) {
                _puts(" (Unknown)");
            } else {
                _puts(" (");
                _puts(CPMCalls[LOW_REGISTER(BC)]);
                _puts(")");
            }
        }

        if (Watch != -1) {
            watchprint(Watch);
        }

        _puts("\r\n");
        _puts("Command|? : ");
        ch = _getcon();
        if (ch > 21 && ch < 127)
            _putch(ch);
        switch (ch) {
        case 't':
            /* Trace to next instruction */
            loop = FALSE;
            break;
        case 'c':
            /* Continue execution */
            loop = FALSE;
            _puts("\r\n");
            Debug = 0;
            break;
        case 'b':
            /* Dump memory pointed by (BC) */
            _puts("\r\n");
            memdump(BC);
            break;
        case 'd':
            /* Dump memory pointed by (DE) */
            _puts("\r\n");
            memdump(DE);
            break;
        case 'h':
            /* Dump memory pointed by (HL) */
            _puts("\r\n");
            memdump(HL);
            break;
        case 'p':
            /* Dump memory page pointed by (PC) */
            _puts("\r\n");
            memdump(PC & 0xFF00);
            break;
        case 's':
            /* Dump memory page pointed by (SP) */
            _puts("\r\n");
            memdump(SP & 0xFF00);
            break;
        case 'x':
            /* Dump memory page pointed by (IX) */
            _puts("\r\n");
            memdump(IX & 0xFF00);
            break;
        case 'y':
            /* Dump memory page pointed by (IY) */
            _puts("\r\n");
            memdump(IY & 0xFF00);
            break;
        case 'a':
            /* Dump memory pointed by dmaAddr */
            _puts("\r\n");
            memdump(dmaAddr);
            break;
        case 'l':
            /* Disassemble from current PC */
            _puts("\r\n");
            I = 16;
            l = pos;
            while (I > 0) {
                _puthex16(l);
                _puts(" : ");
                l += Disasm(l);
                _puts("\r\n");
                --I;
            }
            break;
        case 'A':
            /* Add breakpoint at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                if (z80_add_breakpoint(bpoint) == 0) {
                    _puts("Breakpoint added: ");
                    _puthex16(bpoint);
                    _puts("\r\n");
                } else {
                    _puts("Breakpoint list full\r\n");
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'B':
            /* List breakpoints set via 'A' */
            _puts(" Breakpoints:\r\n");
            if (bp_count == 0) {
                _puts("  (none)\r\n");
            } else {
                for (int i = 0; i < bp_count; ++i) {
                    uint16 a = bp_addrs[i];
                    _puthex16(a);
                    _puts(" : ");
                    Disasm(a);
                    _puts("\r\n");
                }
            }
            break;
        case 'C':
            /* Clear all breakpoints */
            z80_clear_breakpoints();
            _puts(" Breakpoints cleared\r\n");
            break;
        case 'D':
            /* Dump memory at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res)
                memdump(bpoint);
            else
                _puts("Invalid address\r\n");
            break;
        case 'E':
            /* Erase breakpoint at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                if (z80_remove_breakpoint(bpoint) == 0) {
                    _puts("Breakpoint removed: ");
                    _puthex16(bpoint);
                    _puts("\r\n");
                } else {
                    _puts("Breakpoint not found\r\n");
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'J':
            /* Jump - set PC to address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                PC = bpoint;
                _puts("PC set to ");
                _puthex16(PC);
                _puts("\r\n");
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'L':
            /* Disassemble at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                I = 16;
                l = bpoint;
                while (I > 0) {
                    _puthex16(l);
                    _puts(" : ");
                    l += Disasm(l);
                    _puts("\r\n");
                    --I;
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'M':
            /* Modify memory byte at address until Enter is pressed */
            _puts("\r\n Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                uint16 addr = bpoint;
                uint8 val;
                while (1) {
                    _puthex16(addr);
                    _puts(" : ");
                    val = _RamRead(addr);
                    _puthex8(val);
                    _puts(" -> ");
                    res = read_hex8(&val);
                    if (res) {
                        _RamWrite(addr, val);
                        addr = (addr + 1) & 0xFFFF;
                    } else {
                        break; /* exit on no input */
                    }
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'R':
            /* Dump recent trace */
            z80_trace_dump();
            break;
        case 'T':
            /* Step over a call */
            loop = FALSE;
            Step = pos + InstructionLength(pos);
            Debug = 0;
            break;
        case 'U':
            /* Unwatch - clear any byte/word watch */
            Watch = -1;
            _puts("\r\nWatch cleared\r\n");
            break;
        case 'W':
            /* Watch - set a byte/word watch */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                Watch = bpoint;
                _puts("Watch set to ");
                _puthex16(Watch);
                _puts("\r\n");
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'X':
            /* Exit RunCPM */
            _puts("\r\nExiting...\r\n");
            Debug = 0;
            Status = 1;
            break;
        case '?':
            /* Help */
            _puts("\r\n");
            _puts("Lowercase commands:\r\n");
            _puts("  t - traces to the next instruction\r\n");
            _puts("  c - Continue execution\r\n");
            _puts("  b - Dumps memory pointed by (BC)\r\n");
            _puts("  d - Dumps memory pointed by (DE)\r\n");
            _puts("  h - Dumps memory pointed by (HL)\r\n");
            _puts("  p - Dumps the page (PC) points to\r\n");
            _puts("  s - Dumps the page (SP) points to\r\n");
            _puts("  x - Dumps the page (IX) points to\r\n");
            _puts("  y - Dumps the page (IY) points to\r\n");
            _puts("  a - Dumps memory pointed by dmaAddr\r\n");
            _puts("  l - Disassembles from current PC\r\n");
            _puts("Uppercase commands:\r\n");
            _puts("  A - Add breakpoint at address\r\n");
            _puts("  B - List breakpoints\r\n");
            _puts("  C - Clear all breakpoints\r\n");
            _puts("  D - Dumps memory at address\r\n");
            _puts("  E - Erase breakpoint at address\r\n");
            _puts("  J - Jumps to address (sets PC)\r\n");
            _puts("  L - Disassembles at address\r\n");
            _puts("  M - Modify memory at address\r\n");
            _puts("  R - Dump recent trace\r\n");
            _puts("  T - Steps over a call\r\n");
            _puts("  U - Clears the byte/word watch\r\n");
            _puts("  W - Sets a byte/word watch\r\n");
            _puts("  X - Exit RunCPM\r\n");
            break;
        default:
            _puts(" ???\r\n");
        }
    }
}

#endif // ifndef DEBUG_H