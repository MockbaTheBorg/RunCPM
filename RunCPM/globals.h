/* Some definitions needed globally */
#ifdef __MINGW32__
#include <ctype.h>
#endif

//#define DEBUG
//#define DEBUGLOG	// Writes extensive call trace information to RunCPM.log

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

#ifdef __cplusplus
extern "C"
{
#endif
	extern void Z80reset(void);
	extern void Z80run(void);

	extern uint8 _RamRead(uint16 address);
	extern void _RamWrite(uint16 address, uint8 value);

	extern void _Bdos(void);
	extern void _Bios(void);

	extern void cpu_out(const uint32 Port, const uint32 Value);
	extern uint32 cpu_in(const uint32 Port);

#ifndef _WIN32
	extern uint8 _getch(void);
#endif
	extern void _putcon(uint8 ch);
	extern void _puts(const char* str);
	extern void _puthex8(uint8 c);
	extern void _puthex16(uint16 w);

	extern uint16	dmaAddr;
#ifdef __cplusplus
}
#endif

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

