@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Make-DropIn.ps1"
set "R=%ERRORLEVEL%"
echo.
if "%R%"=="0" (echo Drop-in package created.) else (echo Drop-in packaging failed. Copy the output and send it to the chat.)
pause
exit /b %R%
