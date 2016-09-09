\ Arduino CP/M interface
: pinmode ( val pin -- ) 256 * + 220 bdos drop ;
: din ( pin -- n ) 256 * 221 bdos ;
: dout ( val pin -- ) 256 * + 222 bdos drop ;
: ain ( pin -- n ) 256 * 223 fdos drop ;
: aout ( val pin -- ) 256 * + 224 bdos drop ;

\ Turn LED on or off
: ledon ( pin -- ) 1 over pinmode 1 swap dout ;
: ledoff ( pin -- ) 1 over pinmode 0 swap dout ;

\ Use Light Dependent Resistor to adjust LED voltage
\ LED = 255 - (LDR / 4)
: fade ( led ldr -- ) ain 4 / 255 swap - swap aout ;

\ LED connected to pin D9 (PWM)
\ LDR connected to pin A8
\ Run in loop with 100 ms delay until key pressed
: run begin 9 8 fade 100 ms key? until ;
