#!/bin/sh

# Generate A0:$$$.SUB with desired commands. End with "EXIT" to exit cleanly.

perl -e 'print reverse <>' <<EOT | perl -ne 's/[\r\n]//g; printf "%-128s", chr(length($_)).$_.chr(0)' > 'A/0/$$$.SUB'
DIR A:
DIR B:
MAC ZCPR3
ERA CCP-ZCP3.BIN
MLOAD CCP-ZCP3.BIN=ZCPR3.HEX
ERA ZCPR3.HEX
ERA ZCPR3.SYM
EXIT
EOT
./RunCPM