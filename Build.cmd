@echo off
setlocal
cd /d "%~dp0"
if not exist "src\probe.cpp" (
  echo Extract the entire ZIP before running this file.
  pause
  exit /b 1
)
call :build > Build.log 2>&1
set "BUILD_RESULT=%ERRORLEVEL%"
type Build.log
echo.
if "%BUILD_RESULT%"=="0" (echo Build, verification, and release packaging passed. See the release folder.) else (echo Build failed. Review Build.log.)
pause
exit /b %BUILD_RESULT%

:build
echo Control FG v1.0.0 - public release build
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio C++ tools were not found. Install the Desktop development with C++ workload and a Windows SDK.
  exit /b 1
)
set "CONTROL_VSROOT="
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "CONTROL_VSROOT=%%I"
if not defined CONTROL_VSROOT (
  echo Visual Studio needs Desktop development with C++ and a Windows SDK.
  exit /b 1
)
call "%CONTROL_VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Fetch-Streamline.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-Streamline.ps1
if errorlevel 1 exit /b 1
if not exist build mkdir build
if exist build\build-validation.json del build\build-validation.json
cl /nologo /c /std:c++17 /EHsc /W4 /O2 /MT src\abi_check.cpp /Fobuild\abi_check.obj
if errorlevel 1 exit /b 1
dumpbin /symbols build\abi_check.obj > build\abi-symbols.txt
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-Build.ps1 -AbiOnly
if errorlevel 1 exit /b 1
pushd src
rc /nologo /fo ..\build\control_fg_logo.res control_fg_logo.rc
set "RC_RESULT=%ERRORLEVEL%"
popd
if not "%RC_RESULT%"=="0" exit /b %RC_RESULT%
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /LD /guard:cf /DUNICODE /D_UNICODE /I "third_party\streamline\include" src\probe.cpp build\control_fg_logo.res /Fobuild\probe.obj /Febuild\dxgi.dll /link /DEF:src\dxgi.def /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT /GUARD:CF bcrypt.lib wintrust.lib user32.lib gdi32.lib msimg32.lib
if errorlevel 1 exit /b 1
dumpbin /exports build\dxgi.dll > build\exports.txt
if errorlevel 1 exit /b 1
dumpbin /imports build\dxgi.dll > build\imports.txt
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT src\smoke.cpp /Fobuild\smoke.obj /Febuild\smoke.exe
if errorlevel 1 exit /b 1
build\smoke.exe "%CD%\build\dxgi.dll"
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-Build.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Make-DropIn.ps1
exit /b %ERRORLEVEL%
