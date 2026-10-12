@echo off
cd /d "%~dp0"
PT-DXR-GPU-Test.exe > Path-Test-Results.txt 2>&1
set "PT_TEST_EXIT=%ERRORLEVEL%"
if not "%PT_TEST_EXIT%"=="0" goto done
PT-Path-GPU-Test.exe >> Path-Test-Results.txt 2>&1
set "PT_TEST_EXIT=%ERRORLEVEL%"
:done
type Path-Test-Results.txt
echo.
echo Exit code: %PT_TEST_EXIT%
echo Send Path-Test-Results.txt back for review.
pause
exit /b %PT_TEST_EXIT%
