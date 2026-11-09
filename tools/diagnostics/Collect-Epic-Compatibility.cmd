@echo off
setlocal
cd /d "%~dp0..\.."
where py >nul 2>nul
if not errorlevel 1 (
  py -3 tools\epic_compat_probe.py %*
  goto :done
)
where python >nul 2>nul
if not errorlevel 1 (
  python tools\epic_compat_probe.py %*
  goto :done
)
echo Python 3 was not found in PATH.
echo Run this from a machine with Python 3 installed, or pass the Epic path after installing Python.
exit /b 2
:done
set RC=%ERRORLEVEL%
echo.
pause
exit /b %RC%
