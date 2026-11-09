@echo off
if not exist "%~dp0Manage-Probe.ps1" (
  echo Extract the entire ZIP before running this file.
  pause
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Manage-Probe.ps1" -Action Uninstall
pause
