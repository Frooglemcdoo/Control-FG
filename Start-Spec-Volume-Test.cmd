@echo off
setlocal
cd /d "%~dp0"
if not exist "Control_DX12.exe" (
  echo Copy this package beside Control_DX12.exe before running it.
  pause
  exit /b 1
)
tasklist /FI "IMAGENAME eq Control_DX12.exe" /NH 2>nul | find /I "Control_DX12.exe" >nul
if not errorlevel 1 (
  echo Close Control before selecting a component.
  pause
  exit /b 1
)
echo R189 volume roughness filtering - normal brightness
 echo 1. Roughness filtering ON
 echo 2. Baseline - filtering OFF
choice /C 12 /N /M "Select 1 or 2: "
rem Branch on CHOICE immediately: SET can overwrite its ERRORLEVEL.
if errorlevel 3 exit /b 1
if errorlevel 2 goto baseline
if errorlevel 1 goto filtered
exit /b 1
:baseline
set "CONTROLFG_SPEC_VOLUME_ROUGHNESS=off"
goto selected
:filtered
set "CONTROLFG_SPEC_VOLUME_ROUGHNESS=on"
:selected
echo Starting Control. Select NATIVE BLEND, RR on, FG off, SPEC 4.
echo Option 1 F10: VOLUME: ROUGHNESS. Option 2: VOLUME: BASELINE.
echo Check stationary painting/elevator splatter, lighting detail and FPS.
echo SPEC resets on restart: select SPEC 4 each time. SAVE LOG after inspection.
start "" /wait "%~dp0Control_DX12.exe"
endlocal
