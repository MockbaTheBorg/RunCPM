#!/bin/sh

# Convert arguments to lines and emulates "SUBMIT" on those creating "A0:$$$.SUB"
perl -e 'print join("\n", reverse @ARGV)' "$@" | perl -ne 's/[\r\n]//g; printf "%-128s", chr(length($_)).$_.chr(0)' > 'A/0/$$$.SUB'

# EXIT is a RunCMD extension halting the emulator.
yes EXIT | ./RunCPM
