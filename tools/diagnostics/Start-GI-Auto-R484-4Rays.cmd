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
set "CONTROLTOH_GI_TEXTURE_MIP1="
set "CONTROLTOH_GI_SCENE_CAPTURE=1"
set "CONTROLTOH_GI_CAPTURE=1"
set "CONTROLTOH_GI_AUTO_RAYS=4"
set "CONTROLTOH_GI_SPOT_TRACE="
set "CONTROLTOH_GI_JOINT_TRACE="
set "CONTROLTOH_GI_FROZEN_RENDER="
set "CONTROLTOH_GI_FROZEN_PREPARE="
set "CONTROLTOH_GI_MOTION_CAPTURE="
echo R484 4-ray normal mode uses pre-R483 sampling.
echo For the motion diagnostic use Start-GI-Motion-R484.cmd.
start "" /wait "Control_DX12.exe" -dx12
