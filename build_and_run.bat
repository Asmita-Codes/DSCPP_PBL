@echo off
REM ======================================================================
REM  Dynamic Navigation and Entity Management System  -  SDL2 edition
REM  Double-click this file. First run: installs SDL2 (if needed), builds
REM  game.exe, copies the DLLs next to it, and starts the game.
REM  Needs MSYS2 (https://www.msys2.org). If it is not in C:\msys64, edit MSYS.
REM ======================================================================
cd /d "%~dp0"
set MSYS=C:\msys64

if not exist "%MSYS%\usr\bin\bash.exe" (
  echo MSYS2 was not found in %MSYS%.
  echo Install it from https://www.msys2.org or edit the MSYS line in this file.
  pause
  exit /b 1
)
set PATH=%MSYS%\ucrt64\bin;%PATH%

if not exist "%MSYS%\ucrt64\include\SDL2\SDL_ttf.h" (
  echo Installing compiler + SDL2 + SDL2_ttf ^(one time, needs internet^)...
  "%MSYS%\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf"
)

echo Building the game...
g++ -std=c++17 -O2 *.cpp -o game.exe -lSDL2_ttf -lSDL2 -static-libgcc -static-libstdc++
if errorlevel 1 (
  echo.
  echo BUILD FAILED. Copy the FIRST error line above and send it for help.
  pause
  exit /b 1
)

echo Copying the DLLs next to game.exe ^(so it also runs by double-click^)...
"%MSYS%\usr\bin\bash.exe" -lc "cd \"$(cygpath -u '%CD%')\" && ldd game.exe | grep -i ucrt64 | awk '{print $3}' | xargs -r -I{} cp -u {} ."

echo Starting...
game.exe %*
if errorlevel 1 pause
