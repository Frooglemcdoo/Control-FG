@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0PT-CPU-Profile.ps1"
if errorlevel 1 echo Capture failed. See the message above.
pause
