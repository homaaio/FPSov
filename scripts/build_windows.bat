@echo off
echo ============================================
echo Building FPS Overlay for Windows
echo ============================================

set BUILD_DIR=..\build\windows
set RELEASE_DIR=..\release\windows
set VERSION=1.0.0

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %RELEASE_DIR% mkdir %RELEASE_DIR%

echo [1/4] Compiling overlay...
g++ -std=c++17 -O2 -static ^
    src/windows/overlay.cpp ^
    -o %BUILD_DIR%\fps-overlay.exe ^
    -lgdi32 -lpdh -lpsapi -ldwmapi -lshell32 ^
    -mwindows

echo [2/4] Compiling settings core DLL...
g++ -std=c++17 -O2 -shared -static ^
    src/windows/settings_core.cpp ^
    -o %BUILD_DIR%\settings_core.dll

echo [3/4] Copying Python settings...
copy src\windows\settings.py %BUILD_DIR%\settings.py

echo [4/4] Creating release package...
powershell Compress-Archive ^
    -Path %BUILD_DIR%\* ^
    -DestinationPath %RELEASE_DIR%\FPS-Overlay-Windows-v%VERSION%.zip

echo.
echo Build complete!
echo Release: %RELEASE_DIR%\FPS-Overlay-Windows-v%VERSION%.zip
