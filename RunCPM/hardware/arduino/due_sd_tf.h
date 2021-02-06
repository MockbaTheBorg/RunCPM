#ifndef DUE_H
#define DUE_H

// Arduino Due with Hiletgo Stackable SD/TF Shield
#define SDMHZ 40
#define LED 13
#define LEDinv 0
#define board_analog_io
#define board_digital_io
SdFat SD;
#define SDINIT SdSpiConfig(4, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ))
#define BOARD "Arduino Due"
#define board_due

#endif