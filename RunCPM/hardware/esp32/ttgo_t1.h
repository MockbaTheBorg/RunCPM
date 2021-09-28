#ifndef ESP32_H
#define ESP32_H

// SPI_DRIVER_SELECT must be set to 0 in SdFat/SdFatConfig.h (default is 0)

SdFat SD;
#define SPIINIT 14,2,15,13 // sck, miso, mosi, cs
#define SDMHZ 25
#define SDINIT SS, SD_SCK_MHZ(SDMHZ)
#define LED 22
#define LEDinv 0
#define BOARD "TTGO T1"
#define board_esp32
#define board_digital_io

uint8 esp32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif