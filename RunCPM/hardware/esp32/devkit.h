#ifndef ESP32_H
#define ESP32_H

// SPI_DRIVER_SELECT must be set to 0 in SdFat/SdFatConfig.h (default is 0)

SdFat SD;
#define SPIINIT 18,19,23,5 // sck, miso, mosi, cs
#define SDMHZ 20
#define SDINIT SS, SD_SCK_MHZ(SDMHZ)
#define LED 2
#define LEDinv 0
#define BOARD "DOIT ESP32 DevKit"
#define board_esp32
#define board_digital_io

uint8 esp32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif