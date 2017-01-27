#include "globals.h"

#include <SPI.h>
#include <SD.h>

#define LED 13
#define sDELAY 50
#define DELAY 100

// Pin for the SD chip select signal
#ifdef CORE_TEENSY
	#define SDcs BUILTIN_SDCARD
#else
	#define SDcs 4
#endif

#include "abstraction_arduino.h"
#include "ram.h"
#include "console.h"
#include "cpu.h"
#include "disk.h"
#include "cpm.h"
#ifdef CCP_INTERNAL
#include "ccp.h"
#endif

void setup(void) {
	pinMode(LED, OUTPUT);
	digitalWrite(LED, LOW);
	Serial.begin(9600);
	while (!Serial) {	// Wait until serial is connected
		digitalWrite(LED, HIGH);
		delay(sDELAY);
		digitalWrite(LED, LOW);
		delay(sDELAY);
	}
#ifdef DEBUGLOG
	_sys_deletefile((uint8 *)LogName);
#endif

	_clrscr();
	_puts("CP/M 2.2 Emulator v" VERSION " by Marcelo Dantas\r\n");
	_puts("Arduino read/write support by Krzysztof Klis\r\n");
	_puts("      Build " __DATE__ " - " __TIME__ "\r\n");
	_puts("--------------------------------------------\r\n");
	_puts("CCP: " CCPname "  CCP Address: 0x");
	_puthex16(CCPaddr);
	_puts("\r\n");

	if (SD.begin(SDcs)) {
#ifdef CCP_INTERNAL
		while(true)
		{
			_PatchCPM();
			Status = 0;
			_ccp();
			if (Status == 1)
				break;
		}
#else
		if (SD.exists(CCPname)) {
			while (true) {
				_puts(CCPHEAD);
				if (_RamLoad((char *)CCPname, CCPaddr)) {
					_PatchCPM();
					Z80reset();
					SET_LOW_REGISTER(BC, _RamRead(0x0004));
					PC = CCPaddr;
					Z80run();
					if (Status == 1)
						break;
				} else {
					_puts("Unable to load the CCP. CPU halted.\r\n");
					break;
				}
			}
		} else {
			_puts("Unable to load CP/M CCP. CPU halted.\r\n");
		}
#endif
	} else {
		_puts("Unable to initialize SD card. CPU halted.\r\n");
	}
}

void loop(void) {
	digitalWrite(LED, HIGH);
	delay(DELAY);
	digitalWrite(LED, LOW);
	delay(DELAY);
	digitalWrite(LED, HIGH);
	delay(DELAY);
	digitalWrite(LED, LOW);
	delay(DELAY * 4);
}
