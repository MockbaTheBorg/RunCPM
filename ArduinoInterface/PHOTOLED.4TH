\ Arduino CP/M interface
: pinmode ( val pin -- ) 256 * + 220 bdos drop ;
: din ( pin -- ) 256 * 221 bdos ;
: dout ( val pin -- ) 256 * + 222 bdos drop ;
: ain ( pin -- ) 256 * 223 fdos drop ;
: aout ( val pin -- ) 256 * + 224 bdos drop ;

\ Turn LED on or off
: ledon ( pin -- ) 1 over pinmode 1 swap dout ;
: ledoff ( pin -- ) 1 over pinmode 0 swap dout ;

\ React with led at pin 52 to treshold from photoresistor at pin A8
: react ( treshold -- ) 8 ain > if 52 ledon else 52 ledoff then ;

\ Run in loop with 500 ms delay until key pressed
: run ( treshold -- ) begin dup react 500 ms key? until ;

\ 150 run
