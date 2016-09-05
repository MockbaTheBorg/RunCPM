100 REM *** BDOS procedures in machine code ***
110 DATA 94,6,0,14,1,9,86,14,220,205,5,0,201
120 DATA 94,6,0,14,1,9,86,14,222,205,5,0,201
130 DATA 0,0,-1
140 REM *** Load machine code procedures to memory ***
150 BD$="" : READ A : WHILE A>-1 : BD$=BD$+CHR$(A) : READ A : WEND
160 REM *** Calculate machine code start address ***
170 ADR=PEEK(VARPTR(BD$)+1)+PEEK(VARPTR(BD$)+2)*256
180 REM *** USRx = BDOS CALL 22x ***
190 DEF USR0=ADR : DEF USR2=ADR+13
200 REM *** Input PIN number and its value (0 = LOW, 1 = HIGH) ***
210 INPUT "Pin";A : POKE ADR+26,A
220 INPUT "Value";A : POKE ADR+27,A
230 REM *** USR breaks when its argument is a variable ***
240 REM *** You must use a fixed number or memory cell ***
250 X=USR0(PEEK(ADR+26)*256+1)
260 X=USR2(PEEK(ADR+26)*256+PEEK(ADR+27))
