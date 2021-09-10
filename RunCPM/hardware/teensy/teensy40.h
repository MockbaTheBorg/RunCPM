#ifndef TEENSY_H
#define TEENSY_H

#define SDMHZ 50
#define LED 13
#define LEDinv 0
#define board_analog_io
#define board_digital_io
SdFat SD;
typedef File32 File;
#define SDINIT SdSpiConfig(SS, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ))
#define BOARD "Teensy 4.0"
#define board_teensy40

#endif