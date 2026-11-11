@echo off
setlocal
set "CONTROLTOH_GI_FROZEN_PREPARE=1"
set "CONTROLTOH_GI_SINGLE=1"
set "CONTROLTOH_GI_COMPACT=0"
set "CONTROLTOH_GI_PHASE_AUDIT=0"
set "CONTROLTOH_GI_SPOT_TRACE=0"
set "CONTROLTOH_GI_OWNED_TRACE=0"
set "CONTROLTOH_GI_STACK_CAPTURE=0"
set "CONTROLTOH_GI_JOINT_TRACE=0"
set "CONTROLTOH_GI_RAY_TRACE=0"
set "CONTROLTOH_GI_BIND_AUDIT=0"
set "CONTROLTOH_GI_GEOMETRY=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_EVALUATOR="
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0GI-Pipeline-Check.ps1"
if errorlevel 1 echo Pipeline check could not finish. Keep the error text above.
pause
endlocal
