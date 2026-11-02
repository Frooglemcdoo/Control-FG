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
  echo Close Control before starting this test.
  pause
  exit /b 1
)
set "CONTROLTOH_GI_METADATA=1"
set "CONTROLTOH_GI_GEOMETRY=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_SPOT_TRACE=0"
set "CONTROLTOH_GI_RAY_TRACE=0"
set "CONTROLTOH_GI_JOINT_TRACE=0"
set "CONTROLTOH_GI_STACK_CAPTURE=0"
echo R286 - GI geometry integration test. Metadata recording starts automatically.
echo Load your usual save, move around for about one minute, then exit normally.
echo No F7, F9, CAPTURE RR, zoom, or fixed camera is needed.
echo This build tests geometry tracking; replacement GI remains disabled.
echo Existing lighting settings are preserved. The tracker can retain up to 512 MiB of buffer resources.
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
if errorlevel 1 echo Collection failed. Keep the console output and existing logs.
pause
endlocal
