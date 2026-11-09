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
  echo Close Control before starting this diagnostic session.
  pause
  exit /b 1
)
set "CONTROLTOH_GI_BIND_AUDIT=1"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_SOURCE=full"
set "CONTROLTOH_GI_SHADOW=native"
echo R273 - GI shader binding audit. No image capture or lighting replacement.
echo Load the original plant and desk-lamp area with RT indirect and RR ON.
echo Use Native Blend and FG OFF. Leave the lamp intact.
echo Remain in gameplay for 15 seconds, then exit Control normally.
echo Do not click CAPTURE RR. Logs will be packaged after the game closes.
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
if errorlevel 1 echo Collection failed. Keep the console error and existing logs.
pause
endlocal
