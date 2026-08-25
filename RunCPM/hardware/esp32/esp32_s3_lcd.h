#ifndef ESP32_H
#define ESP32_H

// by rockcat
//
// Native SD Pins to SPI Mapping
// Native SD Signal 		SD Pin Name 			SPI Equivalent			Direction (Relative to Master/MCU)
// 
//    DAT3			Chip Select / Data 3		CS (Chip Select)		Input (Master → SD)
//
//    CMD			Command / Data In		MOSI (Master Out, Slave In)	Input (Master → SD)
//
//    CLK			Clock				SCK / CLK (Serial Clock)	Input (Master → SD)
//
//    DAT0			Data 0 / Data Out		MISO (Master In, Slave Out)	Output (SD → Master)
//
//    DAT1 / DAT2		Unused in SPI			NC (Not Connected)		Left floating or pulled high
//
// These are the module pins
// SD_CLK_PIN    14
// SD_CMD_PIN    15 
// SD_D0_PIN     16
// SD_D1_PIN     18
// SD_D2_PIN     17 
// SD_D3_PIN     21 

SdFat SD;

#define SPIINIT 14,16,15,21 // sck, miso, mosi, cs
#define SDMHZ 20
#define SDINIT 21, SD_SCK_MHZ(SDMHZ)

#define LED 2
#define LEDinv 0

#define BOARD "Waveshare ESP32-S3-LCD-1.47"

#define board_esp32
#define board_digital_io

uint8 esp32bdos(uint16 dmaaddr) {
	return(0x00);
}

#endif
