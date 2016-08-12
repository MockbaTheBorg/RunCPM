org  100h
mvi  c,220
mvi  d,53  ;pin number
mvi  e,1   ;pin state
push d
call 5
pop  d
mvi  c,222
call 5
ret
