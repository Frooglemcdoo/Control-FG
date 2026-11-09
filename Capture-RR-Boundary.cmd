@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -CollectExisting
if errorlevel 1 echo Capture did not complete. Keep the diagnostics and send the console error.
pause
