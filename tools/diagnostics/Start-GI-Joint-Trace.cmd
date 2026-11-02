@echo off
setlocal
set "CONTROLTOH_GI_JOINT_TRACE=1"
set "CONTROLTOH_GI_STACK_CAPTURE=1"
rem Optional sampling override: 8, 16, 32 or 64. Empty selects a bounded screen grid.
set "CONTROLTOH_GI_SAMPLE_STEP="
set "CONTROLTOH_GI_GEOMETRY=0"
echo R285 - frame Jesse hair and the plant together anywhere on screen.
echo F7 captures eight screen-wide GI samples and one full-resolution RR pair.
echo Keep camera and zoom steady until SAVED. Lighting stays unchanged.
call "%~dp0Start-GI-Ray-Trace.cmd"
endlocal
