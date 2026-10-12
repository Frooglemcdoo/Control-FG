@echo off
cd /d "%~dp0"
PT-DXR-GPU-Test.exe > GPU-Test-Results.txt 2>&1
set "PT_TEST_EXIT=%ERRORLEVEL%"
type GPU-Test-Results.txt
echo.
echo Exit code: %PT_TEST_EXIT%
echo Send GPU-Test-Results.txt back for review.
pause
exit /b %PT_TEST_EXIT%
