@echo off
if not exist "%~dp0dxgi.dll" (
  echo Extract this package beside the installed dxgi.dll first.
  pause
  exit /b 1
)
if exist "%~dp0ControlFG-PT-Capture-Off.txt" del "%~dp0ControlFG-PT-Capture-Off.txt"
if exist "%~dp0ControlFG-PT-Capture-Off.txt" (
  echo Could not remove the RT-only override. PT remains disabled.
  pause
  exit /b 1
)
>"%~dp0ControlFG-PT-Enabled.txt" echo PT enabled on next game launch
if errorlevel 1 (
  echo Could not enable PT. Check write access to this directory.
  pause
  exit /b 1
)
echo PT selected. Fully close and restart Control.
echo PT capture stays active in this mode even when the live PT toggle is off.
echo Run RT-Only.cmd and restart for full RT-only performance.
pause
