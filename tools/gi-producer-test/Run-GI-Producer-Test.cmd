@echo off
setlocal
cd /d "%~dp0"
GI-Producer-Test.exe > GI-Producer-Result.txt 2>&1
set "GI_TEST_EXIT=%ERRORLEVEL%"
type GI-Producer-Result.txt
echo.
echo Send GI-Producer-Result.txt back, whether this passed or failed.
pause
exit /b %GI_TEST_EXIT%
