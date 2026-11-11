@echo off
setlocal
cd /d "%~dp0\..\.."
if not exist "Control_DX12.exe" (
 echo Run from ControlTOH\Diagnostics inside the installed game folder.
 pause
 exit /b 1
)
tasklist /FI "IMAGENAME eq Control_DX12.exe" /NH 2>nul | find /I "Control_DX12.exe" >nul
if not errorlevel 1 (
 echo Close Control before starting this session.
 pause
 exit /b 1
)
set "CONTROLTOH_GI_COMPACT=1"
set "CONTROLTOH_GI_SINGLE=1"
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
echo GI compact scheduling preview. This does not enable the frozen guide.
echo Select GI: OWNED to use compact phases. GI: NATIVE uses native rays.
echo Normal game launch keeps compact scheduling off. No capture is started.
start "" /wait "Control_DX12.exe"
endlocal
