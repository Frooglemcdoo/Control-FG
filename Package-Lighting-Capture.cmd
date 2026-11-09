@echo off
setlocal
set "PACKAGER=%~dp0package-lighting-capture.py"
if not exist "%PACKAGER%" set "PACKAGER=%~dp0tools\package-lighting-capture.py"
where py >nul 2>nul
if errorlevel 1 goto python
py -3 "%PACKAGER%" %*
goto done
:python
python "%PACKAGER%" %*
:done
if errorlevel 1 echo Packaging failed. Keep the originals and send the error text.
pause
