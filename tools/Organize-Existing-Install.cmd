@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Organize-Existing-Install.ps1"
set "result=%errorlevel%"
if not "%result%"=="0" echo Organization failed. See the error above; any files already moved remain in ControlTOH\LegacyBackup.
pause
exit /b %result%
