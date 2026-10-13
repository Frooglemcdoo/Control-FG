@echo off
if exist "%~dp0ControlFG-PT-Capture-Off.txt" del "%~dp0ControlFG-PT-Capture-Off.txt"
if exist "%~dp0ControlFG-PT-Capture-Off.txt" (
  echo Could not restore PT capture mode.
  pause
  exit /b 1
)
echo PT capture restored. Fully close and restart Control.
pause
