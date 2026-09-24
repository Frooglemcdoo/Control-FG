@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if not errorlevel 1 (
  py -3 tools\gog_compat_probe.py %*
  goto :done
)
where python >nul 2>nul
if not errorlevel 1 (
  python tools\gog_compat_probe.py %*
  goto :done
)
echo Python 3 was not found in PATH.
exit /b 2
:done
set RC=%ERRORLEVEL%
echo.
pause
exit /b %RC%
