@echo off
setlocal
cd /d "%~dp0"
if not exist "src\probe.cpp" exit /b 1
call :build > Build-FSR3.log 2>&1
set "BUILD_RESULT=%ERRORLEVEL%"
type Build-FSR3.log
if not defined CONTROLFG_CI pause
exit /b %BUILD_RESULT%
:build
echo Control FG FSR3 v1.1.0-alpha1 - fixed 2x SDR bring-up
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
set "CONTROL_VSROOT="
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "CONTROL_VSROOT=%%I"
if not defined CONTROL_VSROOT exit /b 1
call "%CONTROL_VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Fetch-Streamline-Headers.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Fetch-FidelityFX.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-FidelityFX.ps1
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /c /std:c++17 /EHsc /W4 /O2 /MT src\abi_check.cpp /Fobuild\abi_check.obj
if errorlevel 1 exit /b 1
dumpbin /symbols build\abi_check.obj > build\abi-symbols.txt
pushd src
rc /nologo /fo ..\build\control_fg_logo.res control_fg_logo.rc
set "RC_RESULT=%ERRORLEVEL%"
popd
if not "%RC_RESULT%"=="0" exit /b %RC_RESULT%
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /LD /guard:cf /DUNICODE /D_UNICODE /DCONTROLFG_FSR3_BRINGUP /I "third_party\streamline\include" /I "third_party\fidelityfx" src\probe.cpp build\control_fg_logo.res /Fobuild\probe.obj /Febuild\dxgi.dll /link /DEF:src\dxgi.def /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT /GUARD:CF bcrypt.lib wintrust.lib user32.lib gdi32.lib msimg32.lib
if errorlevel 1 exit /b 1
dumpbin /exports build\dxgi.dll > build\exports.txt
dumpbin /imports build\dxgi.dll > build\imports.txt
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT src\smoke.cpp /Fobuild\smoke.obj /Febuild\smoke.exe
if errorlevel 1 exit /b 1
build\smoke.exe "%CD%\build\dxgi.dll"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-FSR3-Build.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Make-FSR3-DropIn.ps1
exit /b %ERRORLEVEL%
