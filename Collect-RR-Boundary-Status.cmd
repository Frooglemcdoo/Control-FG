@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
if errorlevel 1 echo Status collection failed. Send the console error.
pause
