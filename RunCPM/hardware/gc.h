// ADAFruit Grand Central
#ifndef DUE_H
#define DUE_H

SdFat SD;
// The GC uses SPI1. CS pin is SDCARD_SS_PIN
#define SDINIT SDCARD_SS_PIN, SD_SCK_MHZ(SDMHZ)
#define SDMHZ 50
#define LED 13
#define LEDinv 0
#define BOARD "ADAFRUIT GRAND CENTRAL"
#define board_gc
#define board_analog_io
#define board_digital_io

#endif