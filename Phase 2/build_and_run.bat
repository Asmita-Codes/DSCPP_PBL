@echo off
cd /d "%~dp0"
set PATH=C:\msys64\ucrt64\bin;%PATH%
echo Building the game...
g++ -std=c++17 -O2 *.cpp -o game.exe -lsfml-graphics -lsfml-window -lsfml-system -static-libgcc -static-libstdc++
if errorlevel 1 (
  echo.
  echo BUILD FAILED. Copy the FIRST error line above and send it for help.
  pause
  exit /b 1
)
game.exe
if errorlevel 1 pause
