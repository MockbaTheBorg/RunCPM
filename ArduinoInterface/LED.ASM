; Turn on a LED wired to pin 53
org  100h   ;start address
mvi  c, 220 ;pinmode
mvi  d, 53  ;digital pin number
mvi  e, 1   ;value (0 = low, 1 = high)
push d      ;save arguments
call 5      ;call BDOS
pop  d      ;restore arguments
mvi  c, 222 ;digital write
call 5      ;call BDOS
ret         ;exit to CP/M
