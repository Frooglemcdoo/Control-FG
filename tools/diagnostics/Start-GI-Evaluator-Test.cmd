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
echo R309 - COMPLETE GI EVALUATOR COMPARISON
set "CONTROLTOH_GI_LIVE=1"
set "CONTROLTOH_GI_EVALUATOR=1"
set "CONTROLTOH_GI_METADATA=0"
set "CONTROLTOH_GI_BINDINGS=0"
set "CONTROLTOH_GI_INDEX_SNAPSHOTS=0"
set "CONTROLTOH_GI_TRIANGLE_SNAPSHOTS=0"
set "CONTROLTOH_GI_INSTANCE_SNAPSHOTS=0"
set "CONTROLTOH_GI_GEOMETRY=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_SPOT_TRACE=0"
set "CONTROLTOH_GI_RAY_TRACE=0"
set "CONTROLTOH_GI_JOINT_TRACE=0"
set "CONTROLTOH_GI_STACK_CAPTURE=0"
set "CONTROLTOH_GI_SOURCE=full"
echo Translated GI evaluator requested. R307 sampler stays enabled in both modes.
echo Keep RT indirect lighting enabled. Keep your usual ray count and RR settings.
echo Check the lit plant and hair while stationary, then move the camera.
echo No capture hotkey is needed. Exit normally to package the logs.
echo F10: GI EVAL button switches Native / Translated without restarting.
echo Select GI RAW BYPASS and keep it the same for both comparisons.
echo This tests the full translated evaluator. ReSTIR sample reuse is not active.
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
if errorlevel 1 echo Collection failed. Keep the console output and existing logs.
pause
endlocal
