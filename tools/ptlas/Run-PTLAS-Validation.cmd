@echo off
cd /d "%~dp0"
rt-capture-replay.exe capture.bin > PTLAS-Replay.txt 2>&1
if errorlevel 1 goto results
Control-PTLAS-Validate.exe capture.bin >> PTLAS-Replay.txt 2>&1
:results
set "TEST_RESULT=%ERRORLEVEL%"
type PTLAS-Replay.txt
echo.
echo Test exit code: %TEST_RESULT%
echo Results saved to PTLAS-Replay.txt in this folder.
pause
exit /b %TEST_RESULT%
