@echo off
cd /d "%~dp0"
Control-RT-Scene-Probe.exe
set "PROBE_RESULT=%ERRORLEVEL%"
echo.
echo Return RT-Scene-Probe.txt from this folder.
pause
exit /b %PROBE_RESULT%
