#include "globals.h"

#include <SPI.h>

#define SDFAT_FILE_TYPE 1 // Uncomment for Due and Teensy

#include <SdFat.h>  // One SD library to rule them all - Greinman SdFat from Library Manager

// Board definitions go into the "hardware" folder, if you use a board different than the
// Arduino DUE, choose/change a file from there and reference that file here
#include "hardware/arduino/due_sd_tf.h"

// Delays for LED blinking
#define sDELAY 50
#define DELAY 100

#include "abstraction_arduino.h"

// Serial port speed
#define SERIALSPD 9600

// PUN: device configuration
#ifdef USE_PUN
File pun_dev;
int pun_open = FALSE;
#endif

// LST: device configuration
#ifdef USE_LST
File lst_dev;
int lst_open = FALSE;
#endif

#include "ram.h"
#include "console.h"
#include CPU
#include "disk.h"
#include "host.h"
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
  _puts("      Built " __DATE__ " - " __TIME__ "\r\n");
  _puts("--------------------------------------------\r\n");
  _puts("CPU is ");
  _puts(CPU_IS);
  _puts("\r\n");
  Z80estimateClock();
	_puts("BIOS at 0x");
	_puthex16(BIOSjmppage);
	_puts(" - ");
	_puts("BDOS at 0x");
	_puthex16(BDOSjmppage);
	_puts("\r\n");
	_puts("CCP " CCPname " at 0x");
	_puthex16(CCPaddr);
	_puts("\r\n");

#if defined board_esp32
  _puts("Initializing SPI.\r\n");
  SPI.begin(SPIINIT);
#endif
  _puts("Initializing SD card.\r\n");
  if (SD.begin(SDINIT)) {
    if (VersionCCP >= 0x10 || SD.exists(CCPname)) {
#ifdef ABDOS
      _PatchBIOS();
#endif
      while (true) {
        _puts(CCPHEAD);
        _PatchCPM();
	Status = STATUS_RUNNING;
#ifdef CCP_INTERNAL
        _ccp();
#else
        if (!_RamLoad((uint8 *)CCPname, CCPaddr ,0)) {
          _puts("Unable to load the CCP.\r\nCPU halted.\r\n");
          break;
        }
     		// Loads an autoexec file if it exists and this is the first boot
		    // The file contents are loaded at ccpAddr+8 up to 126 bytes then the size loaded is stored at ccpAddr+7
		    if (firstBoot) {
			    if (_sys_exists((uint8*)AUTOEXEC)) {
				    uint16 cmd = CCPaddr + 8;
				    uint8 bytesread = (uint8)_RamLoad((uint8*)AUTOEXEC, cmd, 125);
				    uint8 blen = 0;
				    while (blen < bytesread && _RamRead(cmd + blen) > 31)
				    	blen++;
				    _RamWrite(cmd + blen, 0x00);
				    _RamWrite(--cmd, blen);
			    }
			    if (BOOTONLY)
				    firstBoot = FALSE;
		    }
        Z80reset();
        SET_LOW_REGISTER(BC, _RamRead(DSKByte));
        PC = CCPaddr;
        Z80run();
#endif
        if (Status == STATUS_EXIT)
#ifdef DEBUG
	#ifdef DEBUGONHALT
    			Debug = 1;
		    	Z80debug();
	#endif
#endif
          break;
#ifdef USE_PUN
        if (pun_dev)
          _sys_fflush(pun_dev);
#endif
#ifdef USE_LST
        if (lst_dev)
          _sys_fflush(lst_dev);
#endif
      }
    } else {
      _puts("Unable to load CP/M CCP.\r\nCPU halted.\r\n");
    }
  } else {
    _puts("Unable to initialize SD card.\r\nCPU halted.\r\n");
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
