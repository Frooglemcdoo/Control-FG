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
set "CONTROLTOH_GI_BIND_AUDIT=0"
set "CONTROLTOH_GI_SPOT_TRACE=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_SOURCE=full"
set "CONTROLTOH_GI_SHADOW=native"
echo R276 - Reflection clamp 60 percent versus bypass.
echo Keep Native Blend, GI RAW BYPASS, RR and RT indirect ON, FG OFF.
echo Leave the lamp intact and keep the same view and sampling settings.
echo In F10 compare REF CLAMP: 60%% then BYPASS then 60%% again.
echo Close F10 and allow RR to settle after each change. Mouse zoom is supported.
echo Check both stationary speckles and moving highlights. No F7 or CAPTURE RR needed.
echo Exit normally to package the logs.
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
if errorlevel 1 echo Collection failed. Keep the console error and existing logs.
pause
endlocal
