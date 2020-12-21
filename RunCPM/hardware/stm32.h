#ifndef ESP32_H
#define ESP32_H

//SdFatSoftSpiEX<PC8, PD2, PC12> SD; // MISO, MOSI, SCK
SdFat SD;
#define SDINIT PC11 // CS
#define LED PD5
#define LEDinv 1 // 0=normal 1=inverted
#define BOARD "STM32F407DISC1"
#define board_stm32

uint8 stm32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif