#include "globals.h"

#include <SPI.h>
#include <SdFat.h>  // One SD library to rule them all - Greinman SdFat from Library Manager

// SDCard/LED related definitions
#if defined _STM32_DEF_ // STM32 boards
  const uint8_t SOFT_MISO_PIN = PC8;
  const uint8_t SOFT_MOSI_PIN = PD2;
  const uint8_t SOFT_SCK_PIN  = PC12;
  const uint8_t SD_CHIP_SELECT_PIN = PC11;
  SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD; // (fixme) Not sure if this is the best method of accessing the SD
  #define SDINIT SD_CHIP_SELECT_PIN
  #define LED PD13
  #define LEDinv 0
#elif defined ESP32 // ESP32 boards
  SdFat SD;
  #define SDINIT 14,2,15,13 // Some boards use 27,12,14,26
  #define SDMHZ 20 // It it fails, try 10
  #define LED 22 // TTGO_T1=22 LOLIN32_Pro=5(inverted) DOIT_Esp32=2 ESP32-PICO-KIT=no led
  #define LEDinv 0 // 0=normal 1=inverted
#elif defined CORE_TEENSY // Teensy 3.5 and 3.6
  SdFatSdio SD;
  #define SDINIT
  #define LED 13
  #define LEDinv 0
#else // Arduino DUE
  SdFat SD;
  #define SDINIT 4
  #define LED 13
  #define LEDinv 0
#endif

// Delays for LED blinking
#define sDELAY 50
#define DELAY 100

#include "abstraction_arduino.h"

#ifdef ESP32        // ESP32 specific CP/M BDOS call routines
  #include "esp32.h"
#endif
#ifdef _STM32_DEF_  // STM32 specific CP/M BDOS call routines
  #include "stm32.h"
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
	_puts("CCP: " CCPname "    CCP Address: 0x");
	_puthex16(CCPaddr);
	_puts("\r\n");

#ifdef ESP32
  SPI.begin(SDINIT);
  if (SD.begin(SS, SD_SCK_MHZ(SDMHZ))) {
#else
  if (SD.begin(SDINIT)) {
#endif
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
