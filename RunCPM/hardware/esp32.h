#ifndef ESP32_H
#define ESP32_H

/**********************************************************************************************
 * Board selection section START
 ***********************************************************************************************
 */

/*
 * If the symbol EPAPER_ESP32_DRIVER_BOARD is nonzero, following board is in use:
 * https://www.waveshare.com/product/displays/e-paper/driver-boards/e-paper-esp32-driver-board.htm
 */
#define ESP32_EPAPER_DRIVER_BOARD 1

/**
 * If the symbol EPAPER_ESP32_DRIVER_BOARD is nonzero, LILYGO TTGO T1 board is in use.
 */
#define ESP32_TTGO_T1 0

/**********************************************************************************************
 * Board selection section END
 ***********************************************************************************************
 */

#if ESP32_EPAPER_DRIVER_BOARD  && ESP32_TTGO_T1
#error Only one board can be selected!
#endif

#if ESP32_TTGO_T1
SdFatSoftSpiEX<2, 15, 14> SD; // MISO, MOSI, SCK Some boards use 2,15,14,13, other 12,14,27,26
#define SDINIT 13 // CS
#define LED 22 // TTGO_T1=22 LOLIN32_Pro=5(inverted) DOIT_Esp32=2 ESP32-PICO-KIT=no led
#define LEDinv 0
#define BOARD "TTGO_T1"
#define board_esp32
#define board_digital_io
#endif

#if ESP32_EPAPER_DRIVER_BOARD

SdFat SD;
#define SDINIT 18, 19, 23, 5 // HW SPI: CLK, MISO, MOSI, CS
#define SDMHZ 20 // If it fails, try 10
#define LED 22
#define LEDinv 0
#define BOARD "ePaper ESP32"
#define board_esp32_epaper_driver
#define board_digital_io
#endif

uint8 esp32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif