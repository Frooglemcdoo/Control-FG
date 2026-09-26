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
echo Control FG v2.1.1 - GI26 F Specular Camera Matrix Fix - Steam, Epic Games Store and GOG
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
set "CL=/wd4505 %CL%"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Fetch-Streamline.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-Streamline.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-RRRuntime.ps1
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Patch-OverlayStability.ps1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX validation\sdr-correction\test.cpp /Fe:sdr-correction-test.exe
if errorlevel 1 exit /b 1
sdr-correction-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX validation\ui-private\test.cpp /Fe:ui-private-test.exe
if errorlevel 1 exit /b 1
ui-private-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX validation\ui-private\device-identity-test.cpp /Fe:ui-device-identity-test.exe
if errorlevel 1 exit /b 1
ui-device-identity-test.exe
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Verify-Source.ps1
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /std:c++20 /EHsc /W4 /O2 /MT /LD /DNOMINMAX /DUNICODE /D_UNICODE src\rtx40_mfg\control_bridge.cpp src\rtx40_mfg\ada_patch.cpp src\rtx40_mfg\provider_policy.cpp src\rtx40_mfg\gates.cpp src\rtx40_mfg\patches.cpp src\rtx40_mfg\log.cpp /Fobuild\ /Febuild\ControlFG.RTX40MFG.dll /link /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT bcrypt.lib version.lib
if errorlevel 1 exit /b 1

cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\source-selection\test.cpp /Fobuild\source-selection.obj /Febuild\source-selection.exe
if errorlevel 1 exit /b 1
build\source-selection.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\off-present\test.cpp /Fobuild\off-present.obj /Febuild\off-present.exe
if errorlevel 1 exit /b 1
build\off-present.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /c validation\pixels\compile.cpp /Fobuild\pixel-capture-compile.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\alignment\test.cpp /Fobuild\alignment-test.obj /Febuild\alignment-test.exe
if errorlevel 1 exit /b 1
build\alignment-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\rt-stack\policy.cpp /Fobuild\rr-rt-stack.obj /Febuild\rr-rt-stack.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\native-option-isolation\policy.cpp /Fobuild\rr-native-option-policy.obj /Febuild\rr-native-option-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\live-guides\record-test.cpp /Fobuild\rr-r21t-live-record.obj /Febuild\rr-r21t-live-record.exe
if errorlevel 1 exit /b 1
build\rr-r21t-live-record.exe
if errorlevel 1 exit /b 1
build\rr-native-option-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r20r-current\mode-test.cpp /Fobuild\rr-r20r-mode-test.obj /Febuild\rr-r20r-mode-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r20s-current\mode-test.cpp /Fobuild\rr-r20s-mode-test.obj /Febuild\rr-r20s-mode-test.exe
if errorlevel 1 exit /b 1
build\rr-r20s-mode-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r20t-current\mode-test.cpp /Fobuild\rr-r20t-mode-test.obj /Febuild\rr-r20t-mode-test.exe
if errorlevel 1 exit /b 1
build\rr-r20t-mode-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r20x-current\mode-test.cpp /Fobuild\rr-r20x-mode-test.obj /Febuild\rr-r20x-mode-test.exe
if errorlevel 1 exit /b 1
build\rr-r20x-mode-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r21z-preset-default\test.cpp /Fobuild\rr-r21z-preset-default.obj /Febuild\rr-r21z-preset-default.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r22-release\log-filter-test.cpp /Fobuild\release-log-filter.obj /Febuild\release-log-filter.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\hdr-fg-ui\policy-test.cpp /Fobuild\hdr-fg-ui-policy.obj /Febuild\hdr-fg-ui-policy.exe
if errorlevel 1 exit /b 1
build\hdr-fg-ui-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\hdr-fg-ui\color-test.cpp /Fobuild\hdr-fg-ui-color.obj /Febuild\hdr-fg-ui-color.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r26-hdr-transition-warmup\policy-test.cpp /Fobuild\r26-hdr-transition-warmup.obj /Febuild\r26-hdr-transition-warmup.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r27-hdr-transition-quiesce\policy-test.cpp /Fobuild\r27-hdr-transition-quiesce.obj /Febuild\r27-hdr-transition-quiesce.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r28-display-hdr-prepresent\policy-test.cpp /Fobuild\r28-display-hdr-prepresent.obj /Febuild\r28-display-hdr-prepresent.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r29-fresh-output-hdr\policy-test.cpp /Fobuild\r29-fresh-output-hdr.obj /Febuild\r29-fresh-output-hdr.exe
if errorlevel 1 exit /b 1
build\r29-fresh-output-hdr.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r30-hdr-hotkey-guard\policy-test.cpp /Fobuild\r30-hdr-hotkey-guard.obj /Febuild\r30-hdr-hotkey-guard.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\r31-hdr-hard-reset\policy-test.cpp /Fobuild\r31-hdr-hard-reset.obj /Febuild\r31-hdr-hard-reset.exe
if errorlevel 1 exit /b 1
build\r31-hdr-hard-reset.exe
if errorlevel 1 exit /b 1
build\r30-hdr-hotkey-guard.exe
if errorlevel 1 exit /b 1
build\r28-display-hdr-prepresent.exe
if errorlevel 1 exit /b 1
build\r27-hdr-transition-quiesce.exe
if errorlevel 1 exit /b 1
build\r26-hdr-transition-warmup.exe
if errorlevel 1 exit /b 1
build\hdr-fg-ui-color.exe
if errorlevel 1 exit /b 1
build\release-log-filter.exe
if errorlevel 1 exit /b 1
build\rr-r21z-preset-default.exe
if errorlevel 1 exit /b 1
build\rr-r20r-mode-test.exe
if errorlevel 1 exit /b 1
build\rr-rt-stack.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /c validation\performance\windows_compile.cpp /Fobuild\rr-performance-windows.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\performance\policy.cpp /Fobuild\rr-performance-policy.obj /Febuild\rr-performance-policy.exe
if errorlevel 1 exit /b 1
build\rr-performance-policy.exe
if errorlevel 1 exit /b 1
if exist build\build-validation.json del build\build-validation.json
cl /nologo /std:c++17 /EHsc /W4 /c validation\reflection-backend\windows_compile.cpp /Fobuild\reflection-windows.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /Isrc validation\reflection-backend\test_backend.cpp /Fobuild\reflection-backend.obj /Febuild\reflection-backend.exe
if errorlevel 1 exit /b 1
build\reflection-backend.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /Isrc validation\reflection-backend\windows-max.cpp /Fobuild\reflection-windows-max.obj /Febuild\reflection-windows-max.exe
if errorlevel 1 exit /b 1
build\reflection-windows-max.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /Isrc validation\reflection-backend\present-test.cpp /Fobuild\reflection-present.obj /Febuild\reflection-present.exe
if errorlevel 1 exit /b 1
build\reflection-present.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /Isrc validation\primary-handoff\test.cpp /Fobuild\rr-handoff-test.obj /Febuild\rr-handoff-test.exe
if errorlevel 1 exit /b 1
build\rr-handoff-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /c validation\specular-noisy\windows_compile.cpp /Fobuild\rr-native-copy-windows.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\specular-noisy\test.cpp /Fobuild\rr-native-copy-test.obj /Febuild\rr-native-copy-test.exe
if errorlevel 1 exit /b 1
build\rr-native-copy-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /c validation\native-specular-clamp\windows_compile.cpp /Fobuild\rr-native-specular-clamp-windows.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\native-specular-clamp\policy-test.cpp /Fobuild\rr-native-specular-clamp-policy.obj /Febuild\rr-native-specular-clamp-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\evaluation-entry\entry-test.cpp /Fobuild\rr-r22-evaluation-entry.obj /Febuild\rr-r22-evaluation-entry.exe
if errorlevel 1 exit /b 1
build\rr-r22-evaluation-entry.exe
if errorlevel 1 exit /b 1
build\rr-native-specular-clamp-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\frame-coordinator\test.cpp /Fobuild\rr-frame-test.obj /Febuild\rr-frame-test.exe
if errorlevel 1 exit /b 1
build\rr-frame-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\frame-coordinator\recovery-test.cpp /Fobuild\rr-recovery-test.obj /Febuild\rr-recovery-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\frame-coordinator\resolution-test.cpp /Fobuild\rr-resolution-test.obj /Febuild\rr-resolution-test.exe
if errorlevel 1 exit /b 1
build\rr-resolution-test.exe
if errorlevel 1 exit /b 1
build\rr-recovery-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\frame-coordinator\parameters.cpp /Fobuild\rr-parameters-test.obj /Febuild\rr-parameters-test.exe
if errorlevel 1 exit /b 1
build\rr-parameters-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT validation\rr-input-capture\policy.cpp /Fobuild\rr-input-policy.obj /Febuild\rr-input-policy.exe
if errorlevel 1 exit /b 1
build\rr-input-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT validation\rr-input-capture\record.cpp /Fobuild\rr-input-record.obj /Febuild\rr-input-record.exe
if errorlevel 1 exit /b 1
build\rr-input-record.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT validation\rr-no-clamp\test.cpp /Fobuild\rr-no-clamp.obj /Febuild\rr-no-clamp.exe
if errorlevel 1 exit /b 1
build\rr-no-clamp.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT validation\rr-f-distance\test.cpp /Fobuild\rr-f-distance-test.obj /Febuild\rr-f-distance-test.exe
if errorlevel 1 exit /b 1
build\rr-f-distance-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tools\compile-distance.cpp /Fobuild\compile-distance.obj /Febuild\compile-distance.exe /link d3dcompiler.lib
if errorlevel 1 exit /b 1
build\compile-distance.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tools\compile-live.cpp /Fobuild\compile-live.obj /Febuild\compile-live.exe /link d3dcompiler.lib
if errorlevel 1 exit /b 1
build\compile-live.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tools\compile-skin-mask.cpp /Fobuild\compile-skin-mask.obj /Febuild\compile-skin-mask.exe /link d3dcompiler.lib
if errorlevel 1 exit /b 1
build\compile-skin-mask.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tools\compile-reflectance.cpp /Fobuild\compile-reflectance.obj /Febuild\compile-reflectance.exe /link d3dcompiler.lib
if errorlevel 1 exit /b 1
build\compile-reflectance.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\eye-variants\test.cpp /Fobuild\eye-selection-test.obj /Febuild\eye-selection-test.exe
if errorlevel 1 exit /b 1
build\eye-selection-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\eye-variants\integration.cpp /Fobuild\eye-integration-test.obj /Febuild\eye-integration-test.exe
if errorlevel 1 exit /b 1
build\eye-integration-test.exe validation\foliage-variant\evidence\pair-000-original_vs.dxbc validation\foliage-variant\evidence\pair-000-replacement_vs.dxbc validation\foliage-variant\evidence\pair-000-replacement_ps.dxbc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\foliage-variant\selection-test.cpp /Fobuild\foliage-selection-test.obj /Febuild\foliage-selection-test.exe
if errorlevel 1 exit /b 1
build\foliage-selection-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\g12-current\reflectance-test.cpp /Fobuild\reflectance-test.obj /Febuild\reflectance-test.exe
if errorlevel 1 exit /b 1
build\reflectance-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\part1\metadata-test.cpp /Fobuild\part1-metadata.obj /Febuild\part1-metadata.exe
if errorlevel 1 exit /b 1
build\part1-metadata.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\part1\diagnostic-test.cpp /Fobuild\part1-diagnostic.obj /Febuild\part1-diagnostic.exe
if errorlevel 1 exit /b 1
build\part1-diagnostic.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\part1\copy-policy-test.cpp /Fobuild\part1-copy-policy.obj /Febuild\part1-copy-policy.exe
if errorlevel 1 exit /b 1
build\part1-copy-policy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\part1\copy-runtime-test.cpp /Fobuild\part1-copy-runtime.obj /Febuild\part1-copy-runtime.exe
if errorlevel 1 exit /b 1
build\part1-copy-runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\reflectance-gpu\projection-test.cpp /Fobuild\projection-test.obj /Febuild\projection-test.exe
if errorlevel 1 exit /b 1
build\projection-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT validation\reflectance-gpu\compare-test.cpp /Fobuild\compare-test.obj /Febuild\compare-test.exe
if errorlevel 1 exit /b 1
build\compare-test.exe
if errorlevel 1 exit /b 1
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
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tools\compile-fg-ui-alpha.cpp /Fobuild\compile-fg-ui-alpha.obj /Febuild\compile-fg-ui-alpha.exe /link d3dcompiler.lib
if errorlevel 1 exit /b 1
build\compile-fg-ui-alpha.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /c validation\ui-private\windows_compile.cpp /Fobuild\ui-private-compile.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tools\compile-clamp-strength.cpp /Fobuild\compile-clamp-strength.obj /Febuild\compile-clamp-strength.exe /link d3dcompiler.lib
if errorlevel 1 exit /b 1
for /l %%S in (25,1,75) do (
  build\compile-clamp-strength.exe %%S
  if errorlevel 1 exit /b 1
)
copy /y src\rr_clamp_variants.in.h build\rr_clamp_variants.h >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX validation\clamp-strength\policy-test.cpp /Febuild\clamp-strength-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX validation\clamp-strength\dispatch-test.cpp /Febuild\clamp-dispatch-test.exe
if errorlevel 1 exit /b 1
build\clamp-dispatch-test.exe
if errorlevel 1 exit /b 1
build\clamp-strength-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX validation\clamp-strength\dispatch-test.cpp /Febuild\clamp-dispatch-test.exe
if errorlevel 1 exit /b 1
build\clamp-dispatch-test.exe
if errorlevel 1 exit /b 1
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
