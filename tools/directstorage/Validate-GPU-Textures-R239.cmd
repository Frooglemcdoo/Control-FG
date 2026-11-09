@echo off
setlocal
cd /d "%~dp0"
if not exist ControlFG-GpuTexture-Test.exe (
  if exist build\ControlFG-GpuTexture-Test.exe cd build
)
if not exist ControlFG-GpuTexture-Test.exe (
  echo Place this launcher beside ControlFG-GpuTexture-Test.exe and the DirectStorage runtime DLLs.
  pause
  exit /b 1
)
ControlFG-GpuTexture-Test.exe --require-gpu > GPU-Texture-Hardware-R239.log 2>&1
set "TEXTURE_TEST_EXIT=%ERRORLEVEL%"
type GPU-Texture-Hardware-R239.log
echo.
echo Send GPU-Texture-Hardware-R239.log. This validator does not modify or launch Control.
pause
exit /b %TEXTURE_TEST_EXIT%
