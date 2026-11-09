@echo off
setlocal
cd /d "%~dp0"
if exist dxgi.dll (
  echo Extract this test into its own folder, outside the Control game folder.
  pause
  exit /b 1
)
echo Keep Control closed. This test does not install or replace game files.
"%~dp0Capture-Preflight.exe"
echo.
pause
