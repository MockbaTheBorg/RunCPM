#ifndef GLOBALS_H
#define GLOBALS_H

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
#define VERSION	"2.8"
#define VersionBCD 0x28

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/* Definition of which CCP to use: INTERNAL, DR or ZCPR (must define only one) */
//#define CCP_INTERNAL	// If this is defined, CCP will be internal
//#define CCP_DR
#define CCP_CCPZ
//#define CCP_ZCPR2
//#define CCP_ZCPR3
//#define CCP_Z80

/* Definition of the CCP memory information */
//
#ifdef CCP_INTERNAL
#define CCPname		"INTERNAL v1.3"		// Will use the CCP from ccp.h
#define VersionCCP	0x13
#define BatchFCB	(tmpFCB + 36)
#define CCPaddr		(BDOSjmppage-0x0800)
#endif
//
#ifdef CCP_DR
#define CCPname		"CCP-DR." STR(SIZEK) "K"
#define VersionCCP	0x00
#define BatchFCB	(CCPaddr + 0x7AC)	// Position of the $$$.SUB fcb
#define CCPaddr		(BDOSjmppage-0x0800)
#endif
//
#ifdef CCP_CCPZ
#define CCPname		"CCP-CCPZ." STR(SIZEK) "K"
#define VersionCCP	0x01
#define BatchFCB	(CCPaddr + 0x7A)	// Position of the $$$.SUB fcb
#define CCPaddr		(BDOSjmppage-0x0800)
#endif
//
#ifdef CCP_ZCPR2
#define CCPname		"CCP-ZCP2." STR(SIZEK) "K"
#define VersionCCP	0x02
#define BatchFCB	(CCPaddr + 0x5E)	// Position of the $$$.SUB fcb
#define CCPaddr		(BDOSjmppage-0x0800)
#endif
//
#ifdef CCP_ZCPR3
#define CCPname		"CCP-ZCP3." STR(SIZEK) "K"
#define VersionCCP	0x03
#define BatchFCB	(CCPaddr + 0x5E)	// Position of the $$$.SUB fcb
#define CCPaddr		(BDOSjmppage-0x1000)
#endif
//
#ifdef CCP_Z80
#define CCPname		"CCP-Z80." STR(SIZEK) "K"
#define VersionCCP	0x04
#define BatchFCB	(CCPaddr + 0x79E)	// Position of the $$$.SUB fcb
#define CCPaddr		(BDOSjmppage-0x0800)
#endif
//
#define CCPHEAD		"\r\nRunCPM Version " VERSION " (CP/M 2.2 " STR(SIZEK) "K)\r\n"

//#define HASLUA		// Will enable Lua scripting (BDOS call 254)
						// Should be passed externally per-platform with -DHASLUA

/* Definition for CP/M 2.2 user number support */

#define USER_SUPPORT	// If this is defined, CP/M user support is added. RunCPM will ignore the contents of the /A, /B folders and instead
						// look for /A/0 /A/1 and so on, as well for the other drive letters.
						// User numbers are 0-9, then A-F for users 10-15. On case sensitive file-systems the usercodes A-F folders must be uppercase. 
						// This preliminary feature should emulate the CP/M user.

#define BATCHA			// If this is defined, the $$$.SUB will be looked for on drive A:
//#define BATCH0		// If this is defined, the $$$.SUB will be looked for on user area 0
						// The default behavior of DRI's CP/M 2.2 was to have $$$.SUB created on the current drive/user while looking for it
						// on drive A: current user, which made it complicated to run SUBMITs when not logged to drive A: user 0

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

#define WORD16(x)	((x) & 0xffff)

/* CP/M memory definitions */
#define RAM_FAST	// If this is defined, all RAM function calls become direct access (see below)
					// This saves about 2K on the Arduino code and should bring speed imperovements

#define SIZEK 64	// Can be 60 for CP/M 2.2 compatibility or more, up to 64 for extra memory
					// Can be set to less than 60, but this would require rebuilding the CCP
					// For SIZEK<60 CCP ORG = (SIZEK * 1024) - 0x0C00

#define RAMSIZE SIZEK * 1024

#ifdef RAM_FAST		// Makes all function calls to memory access into direct RAM access (less calls / less code)
uint8 RAM[RAMSIZE];
#define _RamSysAddr(a) &RAM[a]
#define _RamRead(a) RAM[a]
#define _RamRead16(a) ((RAM[(a & 0xffff) + 1] << 8) | RAM[a & 0xffff])
#define _RamWrite(a, v) RAM[a] = v
#define _RamWrite16(a, v) RAM[a] = (v) & 0xff; RAM[a + 1] = (v) >> 8
#endif

//// Size of the allocated pages (Minimum size = 1 page = 256 bytes)
#define BIOSpage			(RAMSIZE - 256)
#define BIOSjmppage			(BIOSpage - 256)
#define BDOSpage			(BIOSjmppage - 256)
#define BDOSjmppage			(BDOSpage - 256)

#define DPBaddr (BIOSpage + 64)	// Address of the Disk Parameters Block (Hardcoded in BIOS)

#define SCBaddr (BDOSpage + 16)	// Address of the System Control Block
#define tmpFCB  (BDOSpage + 64)	// Address of the temporary FCB

/* Definition of global variables */
static uint8	filename[17];		// Current filename in host filesystem format
static uint8	newname[17];		// New filename in host filesystem format
static uint8	fcbname[13];		// Current filename in CP/M format
static uint8	pattern[13];		// File matching pattern in CP/M format
static uint16	dmaAddr = 0x0080;	// Current dmaAddr
static uint8	oDrive = 0;			// Old selected drive
static uint8	cDrive = 0;			// Currently selected drive
static uint8	userCode = 0;		// Current user code
static uint16	roVector = 0;
static uint16	loginVector = 0;

#define tohex(x)	(x < 10 ? x + 48 : x + 87)

/* Definition of externs to prevent precedence compilation errors */
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef RAM_FAST
	extern uint8* _RamSysAddr(uint16 address);
	extern void _RamWrite(uint16 address, uint8 value);
#endif
	extern void _RamFill(uint16 address, int size, uint8 value);

	extern void _Bdos(void);
	extern void _Bios(void);

	extern void _HostnameToFCB(uint16 fcbaddr, uint8 *filename);
	extern void _HostnameToFCBname(uint8 *from, uint8 *to);
	extern uint8 match(uint8 *fcbname, uint8 *pattern);

	extern void _puts(const char *str);

#ifdef __cplusplus
}
#endif

#endif
