@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Restore-R71-Shader.ps1"
set "repair_result=%errorlevel%"
echo.
pause
exit /b %repair_result%
