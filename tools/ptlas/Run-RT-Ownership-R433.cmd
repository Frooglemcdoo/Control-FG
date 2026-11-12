@echo off
setlocal
cd /d "%~dp0"
echo Testing queued RT scene ownership. No game files or settings are changed.
Control-PTLAS-Validate.exe --queued > RT-Ownership-R433.txt 2>&1
set "test_result=%errorlevel%"
type RT-Ownership-R433.txt
echo.
echo Return RT-Ownership-R433.txt whether the test passes or fails.
pause
exit /b %test_result%
