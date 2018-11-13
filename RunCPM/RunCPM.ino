#include "globals.h"

#define STM32 // Must be manually activated for STM32 support

#include <SPI.h>

// LED related definitions
#if defined STM32
  #include <STM32SD.h>
  #define LED PD13
  #define LEDinv 0
  #define SDINIT
  #define O_CREAT FA_CREATE_NEW
  #define O_READ FA_READ
  #define O_WRITE FA_WRITE
  #define O_RDONLY FA_READ
  #define O_APPEND FA_OPEN_APPEND
  #define O_RDWR FA_READ | FA_WRITE
#elif defined ESP32
  #include <mySD.h>
  #define LED 22 // TTGO_T1=22 LOLIN32_Pro=5(inverted) DOIT_Esp32=2 ESP32-PICO-KIT=no led
  #define LEDinv 0 // 0=normal 1=inverted
  #define SDINIT 13,15,2,14 // Some boards use 26,14,12,27
#else
  #include <SD.h>
  #define LED 13
  #define LEDinv 0
  #define SDINIT SDcs
#endif

// Delays for LED blinking
#define sDELAY 50
#define DELAY 100

// Pin for the SD chip select signal
#ifdef ARDUINO_SAM_DUE
	#define SDcs 4
#endif
#ifdef CORE_TEENSY
  #define SDcs BUILTIN_SDCARD
#endif

#include "abstraction_arduino.h"

// ESP32 specific BDOS call routines
#ifdef ESP32
#include "esp32.h"
#endif

// Serial port speed
#define SERIALSPD 9600

// AUX: device configuration
#ifdef USE_AUX
File aux_dev;
int aux_open = FALSE;
#endif

// PRT: device configuration
#ifdef USE_PRINTER
File printer_dev;
int printer_open = FALSE;
#endif

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
	Serial.begin(SERIALSPD);
	while (!Serial) {	// Wait until serial is connected
		digitalWrite(LED, HIGH^LEDinv);
		delay(sDELAY);
		digitalWrite(LED, LOW^LEDinv);
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

  if (SD.begin(SDINIT)) {
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
#ifdef USE_AUX
				if (aux_dev)
					_sys_fflush(aux_dev);
#endif
#ifdef USE_PRINTER
				if (printer_dev)
					_sys_fflush(printer_dev);
#endif
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
	digitalWrite(LED, HIGH^LEDinv);
	delay(DELAY);
	digitalWrite(LED, LOW^LEDinv);
	delay(DELAY);
	digitalWrite(LED, HIGH^LEDinv);
	delay(DELAY);
	digitalWrite(LED, LOW^LEDinv);
	delay(DELAY * 4);
}
