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
set "CONTROLTOH_GI_SPOT_TRACE=1"
set "CONTROLTOH_GI_RAY_TRACE=1"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_SOURCE=full"
set "CONTROLTOH_GI_SHADOW=native"
if "%CONTROLTOH_GI_JOINT_TRACE%"=="1" (
  echo R285 - screen-wide GI, point/spot shadows, material guides and RR boundary capture.
) else (
  echo R278 - GI runtime accounting and native output comparison.
)
echo Load the original plant and desk-lamp area with RT indirect and RR ON.
echo Use Native Blend and FG OFF. Leave the lamp intact.
echo Select GI RAW BYPASS and REF CLAMP: BYPASS.
echo Aim at the plant and Jesse hair, close the overlay.
echo Press F7 OR click ARM GI TRACE in F10. A visible ARMED countdown must appear.
echo Close F10 and hold your mouse zoom. Wait for SAVED before exiting.
echo If it says FAILED or stays WAITING, send the status text and packaged logs.
echo Do not click CAPTURE RR. Logs will be packaged after the game closes.
start "" /wait "Control_DX12.exe"
if "%CONTROLTOH_GI_STACK_CAPTURE%"=="1" (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -GiStack -CollectExisting -RequireRR
) else (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
)
if errorlevel 1 echo Collection failed. Keep the console error and existing logs.
pause
endlocal
