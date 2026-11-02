@echo off
setlocal
set "CONTROLTOH_GI_JOINT_TRACE=1"
set "CONTROLTOH_GI_GEOMETRY=0"
echo R284 - center the crosshair on a speckled plant leaf or hair patch.
echo One F7 action captures eight frames of spotlight, point, volume and material data.
echo Red outline: 128 by 128 area sampled every other pixel; white center: full detail.
echo Keep the camera and zoom steady until SAVED. Lighting stays unchanged.
call "%~dp0Start-GI-Ray-Trace.cmd"
endlocal
