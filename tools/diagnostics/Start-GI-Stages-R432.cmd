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
rem Process-local overrides: no saved settings are rewritten.
for /f "tokens=1 delims==" %%G in ('set CONTROLTOH_GI_ 2^>nul') do set "%%G="
set "CONTROLTOH_GI_SINGLE=1"
set "CONTROLTOH_GI_COMPACT=0"
set "CONTROLTOH_GI_FROZEN_PREPARE=0"
set "CONTROLTOH_GI_FROZEN_RENDER=0"
set "CONTROLTOH_GI_STAGE_CAPTURE=1"
set "CONTROLFG_NATIVE_CPU_TIMING=0"
echo R432: rejected guided rendering and preparation disabled.
echo Use RR ON, FG OFF, GI OWNED with RELAX OFF and Native Blend.
echo F10 - CAPTURE STAGES. One beep: hold still. Three: pan. Two: ZIP ready.
start "" /wait "Control_DX12.exe"
endlocal
