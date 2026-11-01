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
  echo Close Control before selecting a GI shadow diagnostic.
  pause
  exit /b 1
)
echo R269 - Spotlight GI shadow isolation, diagnostic only
echo 1. Spotlight GI shadow bypass - normal R269 launch default
echo 2. Native spotlight GI shadows - baseline
echo 3. Remove spotlight GI - previous successful isolation
choice /C 123 /N /M "Select 1, 2 or 3: "
if errorlevel 255 exit /b 1
if errorlevel 3 goto no_spot
if errorlevel 2 goto native
if errorlevel 1 goto bypass
exit /b 1
:no_spot
set "CONTROLTOH_GI_SOURCE=no_spot"
goto selected
:native
set "CONTROLTOH_GI_SOURCE=full"
goto selected
:bypass
set "CONTROLTOH_GI_SOURCE=unshadowed_spot"
:selected
set "CONTROLTOH_GI_SHADOW=native"
echo Keep lamp intact, RT indirect ON, RR ON, Native Blend, GI RAW BYPASS, FG OFF.
echo Option 1 retains spotlight bounce but removes its secondary shadow attenuation.
echo Brighter bounced light or light leaks are expected diagnostics, not a final fix.
echo Direct shadows and ray tracing geometry remain enabled.
echo CAPTURE RR then PACKAGE RR can record each comparison.
start "" /wait "Control_DX12.exe"
endlocal
