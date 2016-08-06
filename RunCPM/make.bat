@echo off

rem ********** Wrapper for make.exe to use from inside Visual Studio
rem ********** Must be on the same directory as the Makefile and sources
rem ********** Do not use from command line as it accumulates the PATH variable

set TOOLCHAIN=C:\MinGW\bin
set UTILS=C:\MinGW\msys\1.0\bin

set PATH=%TOOLCHAIN%;%UTILS%;%PATH%

make.exe -f Makefile.mingw %*
