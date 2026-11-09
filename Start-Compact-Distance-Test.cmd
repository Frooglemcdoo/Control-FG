@echo off
setlocal
cd /d "%~dp0"
if exist "%~dp0..\..\Control_DX12.exe" cd /d "%~dp0..\.."
if not exist "Control_DX12.exe" (
  echo Install the complete package beside Control_DX12.exe before running it.
  pause
  exit /b 1
)
tasklist /FI "IMAGENAME eq Control_DX12.exe" /NH 2>nul | find /I "Control_DX12.exe" >nul
if not errorlevel 1 (
  echo Close Control before selecting a component.
  pause
  exit /b 1
)
echo R190 compact RR hit distance
 echo 1. Hit distance ON
 echo 2. Baseline - hit distance OFF
choice /C 12 /N /M "Select 1 or 2: "
rem Branch on CHOICE immediately: SET can overwrite its ERRORLEVEL.
if errorlevel 3 exit /b 1
if errorlevel 2 goto baseline
if errorlevel 1 goto filtered
exit /b 1
:baseline
set "CONTROLFG_RR_COMPACT_DISTANCE=off"
goto selected
:filtered
set "CONTROLFG_RR_COMPACT_DISTANCE=on"
:selected
echo Starting Control. Select NATIVE BLEND, RR on, FG off, SPEC 4.
echo Option 1 F10: HIT DIST: REQUESTED. Option 2: HIT DIST: BASELINE.
echo Check stationary painting/elevator splatter, lighting detail and FPS.
echo SPEC resets on restart: select SPEC 4 each time. SAVE LOG after inspection.
start "" /wait "%CD%\Control_DX12.exe"
endlocal
