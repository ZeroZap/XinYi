@echo off
REM XinYi Framework - Build Script for Windows
REM Builds the entire project using CMake

setlocal

echo ======================================
echo XinYi Framework - Build Script
echo ======================================
echo.

REM Check if CMake is installed
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake is not installed or not in PATH.
    echo Please install CMake from https://cmake.org/
    exit /b 1
)

REM Get project root directory
set "PROJECT_ROOT=%~dp0.."

REM Create build directory if not exists
if not exist "%PROJECT_ROOT%\build" (
    echo Creating build directory...
    mkdir "%PROJECT_ROOT%\build"
)

cd "%PROJECT_ROOT%\build"

REM Configure
echo Configuring project...
cmake .. -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release

REM Build
echo.
echo Building project...
cmake --build . --config Release

echo.
echo ======================================
echo Build complete!
echo ======================================

endlocal
