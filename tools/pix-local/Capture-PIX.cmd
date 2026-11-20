@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Control-PIX.ps1" -Mode Capture
set "RESULT=%ERRORLEVEL%"
echo Exit code: %RESULT%
pause
exit /b %RESULT%
