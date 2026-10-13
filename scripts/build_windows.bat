@echo off
setlocal

echo ============================================
echo Building FPS Overlay for Windows
echo ============================================

rem Resolve paths relative to this script's own location so the build
rem works regardless of the directory it's invoked from.
set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%..
set BUILD_DIR=%ROOT_DIR%\build\windows
set RELEASE_DIR=%ROOT_DIR%\release\windows
set VERSION=1.0.0

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

echo [1/4] Compiling overlay...
g++ -std=c++17 -O2 -static ^
    "%ROOT_DIR%\src\windows\overlay.cpp" ^
    -o "%BUILD_DIR%\fps-overlay.exe" ^
    -lgdi32 -lpdh -lpsapi -ldwmapi -lshell32 ^
    -mwindows
if errorlevel 1 goto :error

echo [2/4] Compiling settings core DLL...
g++ -std=c++17 -O2 -shared -static ^
    "%ROOT_DIR%\src\windows\settings_core.cpp" ^
    -o "%BUILD_DIR%\settings_core.dll"
if errorlevel 1 goto :error

echo [3/4] Copying Python settings...
copy /y "%ROOT_DIR%\src\windows\settings.py" "%BUILD_DIR%\settings.py" >nul
if errorlevel 1 goto :error

echo [4/4] Creating release package...
powershell -NoProfile -Command "Compress-Archive -Path '%BUILD_DIR%\*' -DestinationPath '%RELEASE_DIR%\FPS-Overlay-Windows-v%VERSION%.zip' -Force"
if errorlevel 1 goto :error

echo.
echo Build complete!
echo Release: %RELEASE_DIR%\FPS-Overlay-Windows-v%VERSION%.zip
exit /b 0

:error
echo.
echo Build FAILED.
exit /b 1
