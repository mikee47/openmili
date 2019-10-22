@echo off
if "%1" == "" goto :noarg
setlocal
set dst=%2
if "%dst%" == "" set dst=.

echo "Fetching '%1' to '%dst%'"

pscp -r -pw banana99 pi@192.168.1.81:/home/pi/openmilight/%1 "%dst%"
goto :EOF

:noarg
echo Require filename to send

