@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0PT-Frame-Profile.ps1" -Resolution DLAA
if errorlevel 1 echo Capture failed. See the message above.
pause
