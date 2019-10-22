@echo off
if "%~1" == "" goto :noarg

:send
pscp -r -pw banana99 "%~1" pi@192.168.1.81:/home/pi/openmilight
goto :EOF

:noarg
echo Require filename to send

