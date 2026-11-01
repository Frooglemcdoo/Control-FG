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
  echo Close Control before selecting a comparison.
  pause
  exit /b 1
)
set "CONTROLTOH_GI_SOURCE="
echo R268 - Full spotlight GI retained in both options
echo 1. Corrected PCF comparison filtering - same as normal launch
echo 2. Native GI shadow sampling - comparison
choice /C 12 /N /M "Select 1 or 2: "
if errorlevel 255 exit /b 1
if errorlevel 2 goto native
if errorlevel 1 goto pcf
exit /b 1
:native
set "CONTROLTOH_GI_SHADOW=native"
goto selected
:pcf
set "CONTROLTOH_GI_SHADOW=pcf"
:selected
echo Keep the lamp intact, RT indirect lighting ON, RR ON, and Native Blend.
echo Keep GI RAW BYPASS, the same sample count, and FG OFF.
start "" /wait "Control_DX12.exe"
endlocal
