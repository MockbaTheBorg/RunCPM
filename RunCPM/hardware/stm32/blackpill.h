#ifndef STM32_H
#define STM32_H



SdFat SD;
const uint8_t SD_CS_PIN = PB6;




#define SDMHZ 25
#define SDINIT SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ), &SPI)
#define LED PC13
#define LEDinv 1
#define BOARD "Black Pill (128k)"
#define board_stm32

uint8 stm32bdos(uint16 dmaaddr) {
    return(0x00);
}

#endif