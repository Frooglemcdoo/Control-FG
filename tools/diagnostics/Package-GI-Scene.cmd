@echo off
setlocal
if "%~1"=="--run" goto run
start "GI Scene Packager" cmd /d /k ""%~f0" --run"
exit /b
:run
cd /d "%~dp0..\.."
set "PACKLOG=%TEMP%\ControlTOH-GI-Packager.log"
echo Original game captures will be kept. No new game run is needed.
echo Enter a NEW output folder on a drive with free space, or press Enter for the game folder.
set "PACKOUT="
set /p "PACKOUT=Output folder: "
set "PACKARGS="
if defined PACKOUT set PACKARGS=--output "%PACKOUT%"
echo Packaging may take several minutes. Progress and errors are saved to:
echo %PACKLOG%
echo You can open that log while packaging is running.
where py >nul 2>nul
if errorlevel 1 goto python
py -3 -u "%~dp0collect-gi-scene.py" %PACKARGS% >"%PACKLOG%" 2>&1
goto result
:python
python -u "%~dp0collect-gi-scene.py" %PACKARGS% >"%PACKLOG%" 2>&1
:result
set "PACKRESULT=%ERRORLEVEL%"
type "%PACKLOG%"
echo.
echo Packager exit code: %PACKRESULT%
echo Log: %PACKLOG%
if not "%PACKRESULT%"=="0" echo FAILED. Send ControlTOH-GI-Packager.log. Keep your saved captures.
echo This window stays open. You may close it when finished.
exit /b %PACKRESULT%
