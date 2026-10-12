@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-PT-Scene-M6.ps1"
if errorlevel 1 echo Collection failed. Send the error text.
pause
