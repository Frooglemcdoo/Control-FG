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
echo R270 - Spotlight GI lighting isolation, diagnostic only
echo 1. White projection texture - normal R270 launch default
echo 2. Flat distance falloff
echo 3. Bypass secondary Fresnel reduction
echo 4. Native spotlight GI - baseline
echo 5. Remove spotlight GI - previous successful isolation
choice /C 12345 /N /M "Select 1, 2, 3, 4 or 5: "
if errorlevel 255 exit /b 1
if errorlevel 5 goto no_spot
if errorlevel 4 goto native
if errorlevel 3 goto fresnel
if errorlevel 2 goto falloff
if errorlevel 1 goto cookie
exit /b 1
:no_spot
set "CONTROLTOH_GI_SOURCE=no_spot"
goto selected
:native
set "CONTROLTOH_GI_SOURCE=full"
goto selected
:fresnel
set "CONTROLTOH_GI_SOURCE=no_fresnel"
goto selected
:falloff
set "CONTROLTOH_GI_SOURCE=flat_falloff"
goto selected
:cookie
set "CONTROLTOH_GI_SOURCE=white_cookie"
:selected
set "CONTROLTOH_GI_SHADOW=native"
echo Keep lamp intact, RT indirect ON, RR ON, Native Blend, GI RAW BYPASS, FG OFF.
echo Tests 1-3 retain native shadow visibility and change one lighting term each.
echo Brightness changes are expected. These are diagnostics, not shipping fixes.
echo Check the plant and hair while still and moving; report the option number.
echo CAPTURE RR then PACKAGE RR can record each comparison.
start "" /wait "Control_DX12.exe"
endlocal
