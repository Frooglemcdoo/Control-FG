@echo off
setlocal
cd /d "%~dp0\..\.."
if not exist "Control_DX12.exe" (
  echo Run this from ControlTOH\Diagnostics inside the game folder.
  pause
  exit /b 1
)
tasklist /FI "IMAGENAME eq Control_DX12.exe" /NH 2>nul | find /I "Control_DX12.exe" >nul
if not errorlevel 1 (
  echo Close Control before starting the capture session.
  pause
  exit /b 1
)
set "CONTROLTOH_GI_CAPTURE=1"
set "CONTROLTOH_GI_SOURCE=full"
set "CONTROLTOH_GI_SHADOW=native"
echo R271 - Native GI hit-array capture. This is a diagnostic, not a speckle fix.
echo Keep the desk lamp intact. Use RR ON, Native Blend, GI RAW BYPASS, FG OFF.
echo Aim at the original plant with Jesse's hair visible on the left.
echo Press F10, CAPTURE RR, close F10 and hold still until capture completes.
echo Then PACKAGE RR and send every generated capture-part ZIP plus logs.
echo Capture work can hitch. Do not use this run for performance comparisons.
start "" /wait "Control_DX12.exe"
endlocal
