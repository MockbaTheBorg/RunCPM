#ifndef TEENSY_H
#define TEENSY_H

#if defined(__MK64FX512__)
  SdFatSdio SD;
  #define SDINIT
  #define LED 13
  #define LEDinv 0
  #define BOARD "TEENSY 3.5"
  #define board_teensy35
  #define board_analog_io
  #define board_digital_io
#elif defined(__MK66FX1M0__)
  SdFatSdio SD;
  #define SDINIT
  #define LED 13
  #define LEDinv 0
  #define BOARD "TEENSY 3.6"
  #define board_teensy36
  #define board_analog_io
  #define board_digital_io
#elif defined(__IMXRT1062__)
  #if defined(ARDUINO_TEENSY40)
    SdFat SD;
    #define SDINIT 10
    #define LED 13
    #define LEDinv 0
    #define BOARD "TEENSY 4.0"
    #define board_teensy40
    #define board_analog_io
    #define board_digital_io
  #else
    SdFat SD;
    #define SDINIT SdioConfig(FIFO_SDIO)
    #define LED 13
    #define LEDinv 0
    #define BOARD "TEENSY 4.1"
    #define board_teensy41
    #define board_analog_io
    #define board_digital_io
  #endif
#else
  #error "Unknown board"
#endif

#endif
