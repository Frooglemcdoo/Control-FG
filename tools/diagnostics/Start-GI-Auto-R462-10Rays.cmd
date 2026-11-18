@echo off
setlocal
cd /d "%~dp0..\.."
if not exist "Control_DX12.exe" (
 echo Place the package beside Control_DX12.exe, keeping ControlTOH folders intact.
 pause
 exit /b 1
)
set "CONTROLTOH_GI_AUTO=1"
set "CONTROLTOH_GI_AUTO_RAYS=10"
set "CONTROLTOH_GI_SPOT_TRACE="
set "CONTROLTOH_GI_JOINT_TRACE="
set "CONTROLTOH_GI_FROZEN_RENDER="
set "CONTROLTOH_GI_FROZEN_PREPARE="
echo R462 10-ray GI with retired scratch reuse.
echo This launcher selects 10 GI rays. Use GI FIXED, RR ON, RELAX OFF.
echo GPU stage timings are sampled automatically. Do not press F7 for this run.
echo Play the same scene for about 60 seconds, then collect compact logs.
echo Close any running game before launching this session.
start "" /wait "Control_DX12.exe" -dx12
