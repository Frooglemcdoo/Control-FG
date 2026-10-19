@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-Reflected-Character-Shaders.ps1" -SourceRoot "%~dp0."
pause
