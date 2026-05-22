#ifndef ESP32_H
#define ESP32_H

// SPI_DRIVER_SELECT must be set to 0 in SdFat/SdFatConfig.h (default is 0)

SdFat SD;
#define SPIINIT 19,20,18,23 // sck, miso, mosi, cs
#define SDMHZ 20
#define SDINIT SdSpiConfig(23, 1, 20000000)
#define LED 6
#define LEDinv 0
#define BOARD "ESP32-C6-Geek"
#define board_esp32
#define board_digital_io

uint8 esp32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif
