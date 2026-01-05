#ifndef PICO_H
#define PICO_H

#define LED 25
#define LEDinv 0
#define board_analog_io
#define board_digital_io

#define RP_CLK_GPIO 18
#define RP_CMD_GPIO 19
#define RP_DAT0_GPIO 20  // DAT1: GPIO21, DAT2: GPIO22, DAT3: GPIO23.
#define SDINIT SdioConfig(RP_CLK_GPIO, RP_CMD_GPIO, RP_DAT0_GPIO)
SdFat SD;
typedef File32 File;

#define BOARD "RP2040-Geek"
#define board_rp2040_geek

#endif