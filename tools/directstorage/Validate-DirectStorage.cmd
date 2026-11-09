@echo off
setlocal
cd /d "%~dp0"
ControlFG-DirectStorage-Validate.exe > DirectStorage-Validation.log 2>&1
set "result=%errorlevel%"
type DirectStorage-Validation.log
echo Exit code: %result%
pause
exit /b %result%
