@echo off
setlocal
cd /d "%~dp0"
if not exist "Control_DX12.exe" (
  echo Place this file beside Control_DX12.exe and exit any running Control session first.
  pause
  exit /b 1
)
set "CONTROLFG_NATIVE_CPU_TIMING=1"
set "CONTROLFG_CPU_APPEND_HOOKS=0"
set "CONTROLFG_RT_APPEND_FAST=0"
set "CONTROLFG_RT_POINTER_FAST=0"
start "" /wait "%~dp0Control_DX12.exe"
endlocal
