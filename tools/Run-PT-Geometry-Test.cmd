@echo off
cd /d "%~dp0"
PT-Geometry-GPU-Test.exe > Geometry-Test-Results.txt 2>&1
set "PT_TEST_EXIT=%ERRORLEVEL%"
if not "%PT_TEST_EXIT%"=="0" goto done
PT-Geometry-GPU-Test.exe --translated >> Geometry-Test-Results.txt 2>&1
set "PT_TEST_EXIT=%ERRORLEVEL%"
if not "%PT_TEST_EXIT%"=="0" goto done
PT-Geometry-GPU-Test.exe --scaled >> Geometry-Test-Results.txt 2>&1
set "PT_TEST_EXIT=%ERRORLEVEL%"
:done
type Geometry-Test-Results.txt
echo.
echo Exit code: %PT_TEST_EXIT%
echo Send Geometry-Test-Results.txt back for review.
pause
exit /b %PT_TEST_EXIT%
