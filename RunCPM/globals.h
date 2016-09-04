/* Some definitions needed globally */
#ifdef __MINGW32__
#include <ctype.h>
#endif

/* Definitions for file/console based debugging */
//#define DEBUG
//#define DEBUGLOG	// Writes extensive call trace information to RunCPM.log
//#define CONSOLELOG	// Writes debug information to console instead of file
//#define LOGONLY 22	// If defined will log only this BDOS (or BIOS) function
#define LogName "RunCPM.log"

/* RunCPM version for the greeting header */
#define VERSION	"2.5"
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/* Definition of which CCP to use, DR or ZCPR (define only one) */
#define CCP_DR
//#define CCP_ZCPR

/* Definition of the CCP memory information */
#ifdef CCP_DR
#define CCPname		"CCP-DR.BIN"
#define CCPaddr		((SIZEK*1024)-0x0C00)
#define BatchFCB	CCPaddr + 0x7AC	// Position of the $$$.SUB fcb
#define PatchCCP	CCPaddr + 0x1FA	// This patches DR's CCP for BDOS real location
#else
#define CCPname		"CCP-ZCPR.BIN"
#define CCPaddr		((SIZEK*1024)-0x0C00)
#define BatchFCB	CCPaddr + 0x5E	// Position of the $$$.SUB fcb
#endif

/* Some environment and type definitions */
#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

typedef signed char     int8;
typedef signed short    int16;
typedef signed int      int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

#define LOW_DIGIT(x)            ((x) & 0xf)
#define HIGH_DIGIT(x)           (((x) >> 4) & 0xf)
#define LOW_REGISTER(x)         ((x) & 0xff)
#define HIGH_REGISTER(x)        (((x) >> 8) & 0xff)

#define SET_LOW_REGISTER(x, v)  x = (((x) & 0xff00) | ((v) & 0xff))
#define SET_HIGH_REGISTER(x, v) x = (((x) & 0xff) | (((v) & 0xff) << 8))

/* Definition of externs to prevent precedence compilation errors */
#ifdef __cplusplus
extern "C"
{
#endif
	extern void Z80reset(void);
	extern void Z80run(void);

	extern uint8* _RamSysAddr(uint16 address);
	extern uint8 _RamRead(uint16 address);
	extern void _RamWrite(uint16 address, uint8 value);
	extern void _RamWrite16(uint16 address, uint16 value);
	extern void _RamFill(uint16 address, int size, uint8 value);

	extern void _Bdos(void);
	extern void _Bios(void);

	extern void cpu_out(const uint32 Port, const uint32 Value);
	extern uint32 cpu_in(const uint32 Port);

	extern uint8 _FCBtoHostname(uint16 fcbaddr, uint8 *filename);
	extern void _HostnameToFCB(uint16 fcbaddr, uint8 *filename);
	extern void _HostnameToFCBname(uint8 *from, uint8 *to);
	extern uint8 match(uint8 *fcbname, uint8 *pattern);

#ifndef _WIN32
	extern uint8 _getche(void);
	extern void _putch(uint8 ch);
#endif
	extern void _putcon(uint8 ch);
	extern void _puts(const char *str);
	extern void _puthex8(uint8 c);
	extern void _puthex16(uint16 w);
#ifdef __cplusplus
}
#endif

/* Externs to allow access to the CPU registers and flags */
extern int32 PCX; /* external view of PC                          */
extern int32 AF;  /* AF register                                  */
extern int32 BC;  /* BC register                                  */
extern int32 DE;  /* DE register                                  */
extern int32 HL;  /* HL register                                  */
extern int32 IX;  /* IX register                                  */
extern int32 IY;  /* IY register                                  */
extern int32 PC;  /* program counter                              */
extern int32 SP;  /* SP register                                  */
extern int32 AF1; /* alternate AF register                        */
extern int32 BC1; /* alternate BC register                        */
extern int32 DE1; /* alternate DE register                        */
extern int32 HL1; /* alternate HL register                        */
extern int32 IFF; /* Interrupt Flip Flop                          */
extern int32 IR;  /* Interrupt (upper) / Refresh (lower) register */
extern int32 Status; /* CPU Status : 0-Running 1-Stop request     */
extern int32 Debug; /* Debug Status : 0-Normal 1-Debugging        */
extern int32 Break; /* Breakpoint                                 */

/* CP/M memory definitions */
#define SIZEK 64	// Can be 60 for CP/M 2.2 compatibility or more, up to 64 for extra memory
					// Can be set to less than 60, but this would require rebuilding the CCP
					// For SIZEK<60 CCP ORG = (SIZEK * 1024) - 0x0C00

#define RAMSIZE SIZEK * 1024

// Size of the allocated pages (Minimum size = 1 page = 256 bytes)
#define BIOSpage		RAMSIZE - 256
#define BDOSpage		BIOSpage - 256
#define BIOSjmppage		BDOSpage - 256
#define BDOSjmppage		BIOSjmppage - 256

#define DPBaddr BIOSpage + 64	// Address of the Disk Parameters Block (Hardcoded in BIOS)

#define SCBaddr BDOSpage + 16	// Address of the System Control Block
#define tmpFCB  BDOSpage + 64	// Address of the temporary FCB

/* Definition of global variables */
static uint8	filename[15];		// Current filename in host filesystem format
static uint8	newname[15];		// New filename in host filesystem format
static uint8	fcbname[13];		// Current filename in CP/M format
static uint8	pattern[13];		// File matching pattern in CP/M format
static uint16	dmaAddr = 0x0080;
static uint16	roVector = 0;
static uint16	loginVector = 0;
