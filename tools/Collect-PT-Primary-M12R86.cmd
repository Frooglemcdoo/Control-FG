@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-PT-Primary-M12R86.ps1"
if errorlevel 1 echo Collection failed. Send the error text.
pause
