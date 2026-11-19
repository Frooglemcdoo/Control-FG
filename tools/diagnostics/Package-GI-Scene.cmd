@echo off
setlocal
cd /d "%~dp0..\.."
where py >nul 2>nul
if not errorlevel 1 (
 py -3 "%~dp0collect-gi-scene.py"
) else (
 python "%~dp0collect-gi-scene.py"
)
pause
