#ifndef STM32_H
#define STM32_H

// SPI_DRIVER_SELECT must be set to 2 in SdFat/SdFatConfig.h (default is 0)

SdFat SD;
const uint8_t SD_CS_PIN = 10;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SDMHZ 50
#define SDINIT SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ), &softSpi)
#define LED PD5
#define LEDinv 1
#define BOARD "STM32F746G-DISCOVERY"
#define board_stm32

uint8 stm32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif