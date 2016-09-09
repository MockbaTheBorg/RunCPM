Additional BDOS calls for Arduino.

=====================================================

BDOS function 220 (0xDC) - PinMode:

 LD C, 220
 LD D, pin_number
 LD E, mode (0 = INPUT, 1 = OUTPUT, 2 = INPUT_PULLUP)
 CALL 5

=====================================================

BDOS function 221 (0xDD) - DigitalRead:

 LD C, 221
 LD D, pin_number
 CALL 5

Returns result in A (0 = LOW, 1 = HIGH).

=====================================================

BDOS function 222 (0xDE) - DigitalWrite:

 LD C, 222
 LD D, pin_number
 LD E, value (0 = LOW, 1 = HIGH)
 CALL 5

=====================================================

BDOS function 223 (0xDF) - AnalogRead:

 LD C, 223
 LD D, pin_number
 CALL 5

Returns result in HL (0 - 1023).

=====================================================

BDOS function 224 (0xE0) - AnalogWrite:

 LD C, 224
 LD D, pin_number
 LD E, value (0-255)
 CALL 5

