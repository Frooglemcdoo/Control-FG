@echo off
setlocal
cd /d "%~dp0..\.."
if not exist "Control_DX12.exe" (
 echo Place the package beside Control_DX12.exe, keeping ControlTOH folders intact.
 pause
 exit /b 1
)
set "CONTROLTOH_GI_AUTO=1"
set "CONTROLTOH_GI_SPOT_TRACE="
set "CONTROLTOH_GI_JOINT_TRACE="
set "CONTROLTOH_GI_FROZEN_RENDER="
set "CONTROLTOH_GI_FROZEN_PREPARE="
echo R456 movable capture box, with R453 rendering.
echo Use 10 GI rays, GI FIXED, RR ON, RELAX OFF for the comparison.
echo Move the orange box using Alt+arrows, then press F7 to start.
echo Wait for the five-second countdown. Stand still for 20 seconds, then pan slowly until complete.
echo Close any running game before launching this session.
start "" /wait "Control_DX12.exe" -dx12
