@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Start-Entrance-Capture.ps1" %*
pause
