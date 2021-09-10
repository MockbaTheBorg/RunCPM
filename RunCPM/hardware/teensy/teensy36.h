#ifndef TEENSY_H
#define TEENSY_H

#define SDMHZ 50
#define LED 13
#define LEDinv 0
#define board_analog_io
#define board_digital_io
SdFat SD;
typedef File32 File;
#define SDINIT SdioConfig(FIFO_SDIO)
#define BOARD "Teensy 3.6"
#define board_teensy36

#endif