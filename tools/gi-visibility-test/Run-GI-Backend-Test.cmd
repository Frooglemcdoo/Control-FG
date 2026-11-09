@echo off
setlocal
cd /d "%~dp0"
echo Running the GI GPU integration test. Control does not need to be running.
echo This does not change game files or settings.
GI-Backend-Test.exe > GI-Backend-Result.txt 2>&1
set "GI_TEST_EXIT=%ERRORLEVEL%"
type GI-Backend-Result.txt
echo.
if "%GI_TEST_EXIT%"=="0" (echo Test passed. Send GI-Backend-Result.txt.) else (echo Test failed. Send GI-Backend-Result.txt.)
pause
exit /b %GI_TEST_EXIT%
