@echo off
setlocal
cd /d "%~dp0..\.."
if not exist "Control_DX12.exe" (
 echo Place the package beside Control_DX12.exe, keeping ControlTOH folders intact.
 pause
 exit /b 1
)
set "CONTROLTOH_GI_AUTO=1"
set "CONTROLTOH_GI_FROZEN_RENDER="
set "CONTROLTOH_GI_FROZEN_PREPARE="
echo R451 experimental automatic GI guidance.
echo Use 10 GI rays, GI FIXED, RR ON, RELAX OFF for the comparison.
echo Close any running game before launching this session.
start "" /wait "Control_DX12.exe" -dx12
