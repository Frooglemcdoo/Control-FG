@echo off
setlocal
set "CONTROLTOH_GI_JOINT_TRACE=1"
set "CONTROLTOH_GI_GEOMETRY=0"
echo R281 - center the crosshair on a speckled plant leaf or hair patch.
echo Captures every pixel in the central 32 by 32 area. Lighting stays unchanged.
call "%~dp0Start-GI-Ray-Trace.cmd"
endlocal
