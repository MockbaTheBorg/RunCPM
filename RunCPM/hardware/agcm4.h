#ifndef AGCM4_H
#define AGCM4_H

#define USE_SDIO 0
SdFat SD;
#define SDINIT SDCARD_SS_PIN, SD_SCK_MHZ(SDMHZ)
#define SDMHZ 50
#define LED 13
#define LEDinv 0
#define BOARD "ADAFRUIT GRAND CENTRAL M4"
#define board_agcm4

#endif