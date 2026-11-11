@echo off
setlocal
cd /d "%~dp0\..\.."
if not exist "Control_DX12.exe" (
  echo Run this from ControlTOH\Diagnostics inside the game folder.
  pause
  exit /b 1
)
tasklist /FI "IMAGENAME eq Control_DX12.exe" /NH 2>nul | find /I "Control_DX12.exe" >nul
if not errorlevel 1 (
  echo Close Control before starting the source capture session.
  pause
  exit /b 1
)
set "CONTROLTOH_GI_SPOT_TRACE=1"
set "CONTROLTOH_GI_OWNED_TRACE=1"
set "CONTROLTOH_GI_STACK_CAPTURE=1"
set "CONTROLTOH_GI_JOINT_TRACE=0"
set "CONTROLTOH_GI_RAY_TRACE=0"
set "CONTROLTOH_GI_BIND_AUDIT=0"
set "CONTROLTOH_GI_GEOMETRY=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_EVALUATOR=1"
echo R424: owned GI source capture while moving. No lighting settings are changed.
echo Keep your current reproducing settings, Owned GI and RR ON, FG OFF, RELAX OFF.
echo An orange box marks the captured region. Ctrl+arrow keys move that box.
echo Put the affected hair OR jaw inside it; press F7, then keep reproducing the movement.
echo Capture starts after five seconds. Keep moving until SAVED appears.
echo Do not use CAPTURE MOTION or CAPTURE RR during this session.
echo Close the game normally after SAVED; the capture will be packaged automatically.
echo This records previously missing per-ray sources, not a quality or performance test.
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Capture-RR-Boundary.ps1" -OwnedSource -CollectExisting -RequireRR
if errorlevel 1 echo Collection failed. Keep the packaged diagnostics and console message.
pause
endlocal
