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
set "CONTROLTOH_GI_MOTION_CAPTURE=1"
echo R489 motion diagnostic: 4 rays, 16 consecutive full-screen frames.
echo Use GI FIXED, RR ON, FG OFF. REBLUR and RELAX are off. Never F7.
echo F10 > CAPTURE MOTION (or Alt+Insert): move Jesse until STOP, then release movement.
echo This is a large diagnostic capture, not a performance test.
echo Memory and free disk are checked before recording. No silent downsampling.
echo Wait for Saved, close the game, then run Package-GI-Motion.cmd.
start "" /wait "Control_DX12.exe" -dx12
