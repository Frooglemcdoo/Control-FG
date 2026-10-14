@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0PT-Enable.ps1"
if errorlevel 1 (
  echo PT enable failed. See the error above.
  pause
  exit /b 1
)
pause
