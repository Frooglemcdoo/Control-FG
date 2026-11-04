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
 echo Close Control before starting this test.
 pause
 exit /b 1
)
echo R331 - OWNED GI WITH F10 RECORD BUTTON
set "CONTROLTOH_GI_LIVE=1"
set "CONTROLTOH_GI_EVALUATOR=1"
set "CONTROLTOH_GI_OWNED=1"
set "CONTROLTOH_GI_METADATA=1"
set "CONTROLTOH_GI_LEGACY_AUDIT=0"
set "CONTROLTOH_GI_BINDINGS=1"
set "CONTROLTOH_GI_INDEX_SNAPSHOTS=0"
set "CONTROLTOH_GI_TRIANGLE_SNAPSHOTS=0"
set "CONTROLTOH_GI_INSTANCE_SNAPSHOTS=0"
set "CONTROLTOH_GI_GEOMETRY=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_SPOT_TRACE=0"
set "CONTROLTOH_GI_RAY_TRACE=0"
set "CONTROLTOH_GI_JOINT_TRACE=0"
set "CONTROLTOH_GI_STACK_CAPTURE=0"
set "CONTROLTOH_GI_SOURCE=full"
echo Load the problem room with RR ON, FG OFF, and GI RAW BYPASS.
echo F10: select GI OWNED, then click RECORD GI. Allow the Windows administrator prompt.
echo Five-second delay. One beep: stand still. Next beep: move camera slowly.
echo Two beeps: close the game. Capture ZIPs are packaged and opened automatically.
start "" /wait "Control_DX12.exe"
endlocal
