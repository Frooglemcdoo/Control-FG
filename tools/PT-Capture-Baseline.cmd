@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0PT-Enable.ps1" -GameDirectory "%~dp0." -Mode Off
if errorlevel 1 (
  echo Could not save RT-only mode. See the error above.
  pause
  exit /b 1
)
echo Start Control normally. PT capture will be bypassed for this session.
pause
