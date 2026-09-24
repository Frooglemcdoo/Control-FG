@echo off
setlocal
if "%~1"=="" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-ControlFG-Compact-Logs.ps1" -OutputDirectory "%~dp0."
) else (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-ControlFG-Compact-Logs.ps1" -ProjectDirectory "%~f1\." -OutputDirectory "%~dp0."
)
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Compact log collection failed.
pause
exit /b %RESULT%
