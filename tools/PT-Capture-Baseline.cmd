@echo off
if not exist "%~dp0dxgi.dll" (
  echo Run this beside the installed dxgi.dll in the game directory.
  pause
  exit /b 1
)
>"%~dp0ControlFG-PT-Capture-Off.txt" echo R37 restart-only PT capture baseline
if errorlevel 1 (
  echo Could not set baseline mode.
  pause
  exit /b 1
)
echo Baseline mode selected. Fully close and restart Control.
echo PT is unavailable in this mode. Keep RR and FG settings the same.
echo Compare DLAA and Quality, then run the R37 collector.
echo Run PT-Capture-Restore.cmd and restart to restore PT.
pause
