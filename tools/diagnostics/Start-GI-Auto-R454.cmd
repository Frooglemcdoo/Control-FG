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
echo R454 consecutive-frame GI audit, with R453 rendering.
echo Use 10 GI rays, GI FIXED, RR ON, RELAX OFF for the comparison.
echo Keep Jesse near left-center at the troublesome light. Stand still for 20 seconds, then pan slowly for 25 seconds.
echo This records compact pixel summaries automatically; no capture hotkey is needed.
echo Close any running game before launching this session.
start "" /wait "Control_DX12.exe" -dx12
