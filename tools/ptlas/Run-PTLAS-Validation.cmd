@echo off
cd /d "%~dp0"
Control-PTLAS-Validate.exe > PTLAS-Validation.txt 2>&1
set "TEST_RESULT=%ERRORLEVEL%"
type PTLAS-Validation.txt
echo.
echo Test exit code: %TEST_RESULT%
echo Results saved to PTLAS-Validation.txt in this folder.
pause
exit /b %TEST_RESULT%
