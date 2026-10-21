@echo off
pushd "%~dp0"
Per-Light-DXR-Test.exe > Per-Light-DXR-Test.log 2>&1
set "test_result=%errorlevel%"
type Per-Light-DXR-Test.log
echo.
echo Send Per-Light-DXR-Test.log back.
popd
pause
exit /b %test_result%
