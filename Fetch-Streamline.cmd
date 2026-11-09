@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Fetch-Streamline.ps1"
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Streamline staging failed.
pause
exit /b %RESULT%
