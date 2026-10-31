@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-Texture-Trace.ps1" -OutputDirectory "%~dp0."
pause
