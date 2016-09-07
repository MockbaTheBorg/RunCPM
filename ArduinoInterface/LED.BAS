100 REM ### BDOS procedures in machine code ###
110 REM
120 REM  PUSH HL
130 REM  LD   E,(HL) ;load arg low byte into E
140 REM  LD   BC,1
150 REM  ADD  HL,BC  ;memory pointer + 1
160 REM  LD   D,(HL) ;load arg high byte into D
170 REM  LD   C,220  ;pinMode()
180 REM  CALL 5
190 REM  POP  HL
200 REM  RET
210 REM
220 DATA 229,94,1,1,0,9,86,14,220,205,5,0,225,201
230 REM
240 REM  PUSH HL
250 REM  LD   BC,1
260 REM  ADD  HL,BC  ;memory pointer + 1
270 REM  LD   D,(HL) ;load arg high byte into D
280 REM  LD   C,221  ;digitalRead()
290 REM  CALL 5
300 REM  POP  HL
310 REM  LD   (HL),A ;store result low byte
320 REM  PUSH HL
330 REM  LD   A,0
340 REM  LD   BC,1
350 REM  ADD  HL,BC
360 REM  LD   (HL),A ;clear result high byte
370 REM  POP  HL
380 REM  RET
390 REM
400 DATA 229,1,1,0,9,86,14,221,205,5,0,225,119,229,62,0,1,1,0,9,119,225,201
410 REM
420 REM  PUSH HL
430 REM  LD   E,(HL) ;load arg low byte into E
440 REM  LD   BC,1
450 REM  ADD  HL,BC  ;memory pointer + 1
460 REM  LD   D,(HL) ;load arg high byte into D
470 REM  LD   C,222  ;digitalWrite()
480 REM  CALL 5
490 REM  POP  HL
500 REM  RET
510 REM
520 DATA 229,94,1,1,0,9,86,14,222,205,5,0,225,201
530 REM
540 REM  PUSH HL
550 REM  LD   BC,1
560 REM  ADD  HL,BC  ;memory pointer + 1
570 REM  LD   D,(HL) ;load arg high byte into D
580 REM  LD   C,223  ;analogRead()
590 REM  CALL 5
600 REM  LD   D,H
610 REM  LD   E,L
620 REM  POP  HL
630 REM  LD   (HL),E ;store result low byte
640 REM  PUSH HL
650 REM  LD   BC,1
660 REM  ADD  HL,BC
670 REM  LD   (HL),D ;store result high byte
680 REM  POP  HL
690 REM  RET
700 REM
710 DATA 229,1,1,0,9,86,14,223,205,5,0,84,93,225,115,229,1,1,0,9,114,225,201
720 DATA 0,0,-1
730 REM
740 REM ### Load machine code procedures to memory ###
750 REM
760 BD$="" : READ A : WHILE A>-1 : BD$=BD$+CHR$(A) : READ A : WEND
770 ADR=PEEK(VARPTR(BD$)+1)+PEEK(VARPTR(BD$)+2)*256
780 REM
790 REM ### USRx = BDOS CALL 22x ###
800 REM
810 DEF USR0=ADR : DEF USR1=ADR+14 : DEF USR2=ADR+37 : DEF USR3=ADR+51
820 REM
830 REM ### Input PIN number and its value (0 = LOW, 1 = HIGH) ###
840 REM
850 INPUT "Pin";P : INPUT "Value";V
860 REM
870 REM ### Variables do not work with USRx as described in MBasic manual ###
880 REM ### But you can pass argument via memory using fixed numbers only ###
890 REM
900 POKE ADR+74,P : POKE ADR+75,V
910 REM
920 REM ### Turn a LED on or off ###
930 REM
940 X=USR0(PEEK(ADR+74)*256+1)
950 X=USR2(PEEK(ADR+74)*256+PEEK(ADR+75))
960 REM
970 REM ### Read analog value from the pin ###
980 REM
990 REM X=USR3(PEEK(ADR+74)*256) : PRINT X
