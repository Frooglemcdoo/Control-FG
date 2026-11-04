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
echo R316 - OWNED GI INTEGRATION
set "CONTROLTOH_GI_LIVE=1"
set "CONTROLTOH_GI_EVALUATOR=1"
set "CONTROLTOH_GI_OWNED=1"
set "CONTROLTOH_GI_METADATA=1"
set "CONTROLTOH_GI_LEGACY_AUDIT=0"
set "CONTROLTOH_GI_BINDINGS=1"
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
echo Owned GI requested: proposal, ray tracing, hit shading and output resolve.
echo Keep RT indirect lighting and RR enabled. Select GI RAW BYPASS.
echo F10: the GI button switches NATIVE / OWNED without restarting.
echo OWNED WAIT means replacement has not been admitted; native output remains.
echo Check the lit plant and hair while stationary, then move the camera.
echo No capture hotkey or standalone producer test is needed.
echo Exit normally to package the logs. Send the resulting diagnostics ZIP.
echo Spatial and temporal sample reuse are not enabled in this build.
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -DiagnosticsOnly
if errorlevel 1 echo Collection failed. Keep the console output and existing logs.
pause
endlocal
