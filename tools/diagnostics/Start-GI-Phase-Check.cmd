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
set "CONTROLTOH_GI_PHASE_AUDIT=1"
set "CONTROLTOH_GI_SPOT_TRACE=0"
set "CONTROLTOH_GI_OWNED_TRACE=0"
set "CONTROLTOH_GI_STACK_CAPTURE=0"
set "CONTROLTOH_GI_JOINT_TRACE=0"
set "CONTROLTOH_GI_RAY_TRACE=0"
set "CONTROLTOH_GI_BIND_AUDIT=0"
set "CONTROLTOH_GI_GEOMETRY=0"
set "CONTROLTOH_GI_CAPTURE=0"
set "CONTROLTOH_GI_EVALUATOR="
set "CONTROLTOH_GI_SINGLE=1"
echo R425: automatic GI command-list ordering check, not a noise-fix build.
echo Keep GI enabled. Load a level, play normally for 15 seconds, then exit.
echo No hotkey, camera position, recording or settings comparison is needed.
echo The metadata window stops automatically. The ZIP is collected on exit.
for /f "delims=" %%T in ('powershell.exe -NoProfile -Command "[DateTime]::UtcNow.ToString('o')"') do set "PHASE_RUN_START=%%T"
start "" /wait "Control_DX12.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-GI-Phase-Check.ps1" -RunStartedUtc "%PHASE_RUN_START%"
if errorlevel 1 echo Collection failed. Keep the console error and GI-Phase-R425 log.
pause
endlocal
