#ifndef ESP32_H
#define ESP32_H

SdFatSoftSpiEX<2, 15, 14> SD; // MISO, MOSI, SCK Some boards use 2,15,14,13, other 12,14,27,26
#define SDINIT 13 // CS
#define LED 22 // TTGO_T1=22 LOLIN32_Pro=5(inverted) DOIT_Esp32=2 ESP32-PICO-KIT=no led
#define LEDinv 0
#define BOARD "TTGO_T1"
#define board_esp32
#define board_digital_io

uint8 esp32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif