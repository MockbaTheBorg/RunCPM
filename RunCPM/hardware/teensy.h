#ifndef TEENSY_H
#define TEENSY_H

#define SDMHZ 50
#define LED 13
#define LEDinv 0
#define board_analog_io
#define board_digital_io
#if defined(__MK64FX512__)
  SdFat32 SD;
  typedef File32 File;
  #define SDINIT SdioConfig(FIFO_SDIO)
  #define BOARD "TEENSY 3.5"
  #define board_teensy35
#elif defined(__MK66FX1M0__)
  SdFat32 SD;
  typedef File32 File;
  #define SDINIT SdioConfig(FIFO_SDIO)
  #define BOARD "TEENSY 3.6"
  #define board_teensy36
#elif defined(__IMXRT1062__)
  #if defined(ARDUINO_TEENSY40)
    SdFat SD;
    #define SDINIT SdSpiConfig(SS, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ))
    #define BOARD "TEENSY 4.0"
    #define board_teensy40
   #else
    SdFat SD;
    #define SDINIT SdioConfig(FIFO_SDIO)
    #define BOARD "TEENSY 4.1"
    #define board_teensy41
  #endif
#else
  #error "Unknown board"
#endif

#endif
