@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-ControlFG-Logs.ps1"
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Log collection failed.
pause
exit /b %RESULT%
