@echo off
REM Scrcpy Control Center — Local Build Script
REM Requires: Visual Studio 2019/2022 with C++ Desktop workload
REM
REM Usage:
REM   build.bat          — Build Release (Win32 x86)
REM   build.bat debug    — Build Debug (Win32 x86)

setlocal

set CONFIG=Release
if /i "%1"=="debug" set CONFIG=Debug

echo ============================================
echo  Scrcpy Control Center - Build
echo  Configuration: %CONFIG%
echo  Platform: Win32 (x86)
echo ============================================
echo.

REM Find cmake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH.
    echo Please run this from a "Developer Command Prompt for VS"
    echo or install CMake and add it to PATH.
    exit /b 1
)

REM Configure
echo [1/3] Configuring CMake...
cmake -S . -B build -A Win32 -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 (
    echo [ERROR] CMake configure failed!
    exit /b 1
)

REM Build
echo [2/3] Building...
cmake --build build --config %CONFIG% -j
if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

REM Copy config
echo [3/3] Preparing output...
if not exist "build\%CONFIG%\config" mkdir "build\%CONFIG%\config"
xcopy /E /Y /Q config "build\%CONFIG%\config\" >nul

echo.
echo ============================================
echo  BUILD SUCCESSFUL
echo  Output: build\%CONFIG%\ScrcpyControlCenter.exe
echo ============================================

endlocal
