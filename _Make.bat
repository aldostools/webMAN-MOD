@echo off

set CYGWIN=C:\cygwin\bin

if not exist %CYGWIN%\bash.exe set CYGWIN=C:\msys\1.0\bin

set CHERE_INVOKING=1
%CYGWIN%\bash --login -i -c 'make; rm webftp_server.prx; rm webftp_server.sym; exec bash'
