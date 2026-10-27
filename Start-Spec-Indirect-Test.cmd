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
echo R187 reflected diffuse / environment diagnostic
echo 1. Full reflections - R184 baseline
echo 2. Reflected diffuse only
echo 3. Environment specular only - diagnostic brightness x8
choice /C 123 /N /M "Select 1, 2 or 3: "
rem Branch on CHOICE immediately: SET can overwrite its ERRORLEVEL.
if errorlevel 255 exit /b 1
if errorlevel 3 goto environment
if errorlevel 2 goto diffuse
if errorlevel 1 goto full
exit /b 1
:environment
set "CONTROLFG_SPEC_INDIRECT_COMPONENT=environment"
goto selected
:diffuse
set "CONTROLFG_SPEC_INDIRECT_COMPONENT=diffuse"
goto selected
:full
set "CONTROLFG_SPEC_INDIRECT_COMPONENT="
:selected
echo Starting Control. Select NATIVE BLEND, RR on, SPEC 4.
echo Check F10: option 2 must show REFLECTION: DIFFUSE.
echo Option 3 must show ENV SPEC x8. If it does not, stop this test.
echo Dark or missing reflected lighting is intentional. This is not a performance test.
echo SPEC resets on restart: select SPEC 4 each time. SAVE LOG after inspection.
start "" /wait "%~dp0Control_DX12.exe"
endlocal
