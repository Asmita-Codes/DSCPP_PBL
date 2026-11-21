@echo off
REM Starts the already-built game.  run.bat --windowed  /  run.bat --demo
cd /d "%~dp0"
set PATH=C:\msys64\ucrt64\bin;%PATH%
if not exist game.exe ( echo game.exe not built yet - double-click build_and_run.bat first. & pause & exit /b 1 )
game.exe %*
if errorlevel 1 pause
