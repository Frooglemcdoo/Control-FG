@echo off
setlocal
cd /d "%~dp0..\.."
if not exist "Control_DX12.exe" (
 echo Place the package beside Control_DX12.exe, keeping ControlTOH folders intact.
 pause
 exit /b 1
)
set "CONTROLTOH_GPU_FAULT="
set "CONTROLTOH_GI_AUTO=1"
set "CONTROLTOH_GI_AUTO_RAYS=4"
set "CONTROLTOH_GI_SPOT_TRACE="
set "CONTROLTOH_GI_JOINT_TRACE="
set "CONTROLTOH_GI_FROZEN_RENDER="
set "CONTROLTOH_GI_FROZEN_PREPARE="
echo R472 4-ray GI with receiver-local GI guidance.
echo This launcher selects 4 GI rays. Use GI FIXED, RR ON, RELAX OFF.
echo Diagnostic capture: Alt+arrows to position the box; Alt+Insert to capture.
echo Do not press F7. Judge FPS outside the capture.
echo Normal performance mode: forced GPU crash diagnostics are disabled.
echo Close any running game before launching this session.
start "" /wait "Control_DX12.exe" -dx12
