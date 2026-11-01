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
  echo Close Control before selecting a GI source diagnostic.
  pause
  exit /b 1
)
echo R267 - Indirect lighting source diagnostic, not a visual fix
echo 1. Full GI - baseline
echo 2. GI without point-light contributions
echo 3. GI without spot-light contributions
echo 4. GI without point or spot contributions - baked GI retained
choice /C 1234 /N /M "Select 1, 2, 3 or 4: "
if errorlevel 255 exit /b 1
if errorlevel 4 goto no_local
if errorlevel 3 goto no_spot
if errorlevel 2 goto no_point
if errorlevel 1 goto full
exit /b 1
:no_local
set "CONTROLTOH_GI_SOURCE=no_local"
goto selected
:no_spot
set "CONTROLTOH_GI_SOURCE=no_spot"
goto selected
:no_point
set "CONTROLTOH_GI_SOURCE=no_point"
goto selected
:full
set "CONTROLTOH_GI_SOURCE="
:selected
echo Keep the lamp intact, RT indirect lighting ON, RR ON, and Native Blend.
echo Use the same GI sample count and GI RAW BYPASS in each run. Keep FG OFF.
echo Missing bounced light is intentional. Direct lighting stays enabled.
echo This is not a performance comparison. Use CAPTURE RR then PACKAGE RR.
echo Normal launch restores full GI; no setting is saved by this launcher.
start "" /wait "Control_DX12.exe"
endlocal
