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
echo R183 reflection contribution diagnostic
echo 1. Full reflections - accepted baseline
echo 2. Emission removed - reflected shading remains
echo 3. Emission only - reflected shading removed
choice /C 123 /N /M "Select 1, 2 or 3: "
rem Branch on CHOICE immediately: SET can overwrite its ERRORLEVEL.
if errorlevel 255 exit /b 1
if errorlevel 3 goto emission
if errorlevel 2 goto shading
if errorlevel 1 goto full
exit /b 1
:emission
set "CONTROLFG_SPEC_COMPONENT=emission"
goto selected
:shading
set "CONTROLFG_SPEC_COMPONENT=shading"
goto selected
:full
set "CONTROLFG_SPEC_COMPONENT="
:selected
echo Starting Control. Select NATIVE BLEND, RR on, SPEC 4.
echo Before capturing, check F10: option 2 must show REFLECTION: SHADING.
echo Option 3 must show REFLECTION: EMISSION. If it does not, do not capture.
echo Dark or missing reflected lighting is intentional in component modes.
start "" /wait "%~dp0Control_DX12.exe"
endlocal
