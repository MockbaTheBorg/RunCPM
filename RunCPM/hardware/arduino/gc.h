// ADAFruit Grand Central
#ifndef GC_H
#define GC_H

SdFat SD;
// The GC uses SPI1. CS pin is SDCARD_SS_PIN
#define SDMHZ 50
#define SDINIT SDCARD_SS_PIN, SD_SCK_MHZ(SDMHZ)
#define LED 13
#define LEDinv 0
#define BOARD "AdaFruit Grand Central"
#define board_gc
#define board_analog_io
#define board_digital_io

#endif