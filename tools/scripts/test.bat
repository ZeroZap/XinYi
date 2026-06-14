@echo off
REM XinYi Framework - Test Script for Windows
REM Runs all tests using CTest

setlocal

echo ======================================
echo XinYi Framework - Test Runner
echo ======================================
echo.

REM Get project root directory
set "PROJECT_ROOT=%~dp0.."

REM Check if build directory exists
if not exist "%PROJECT_ROOT%\build" (
    echo Error: Build directory not found.
    echo Please run build.bat first.
    exit /b 1
)

cd "%PROJECT_ROOT%\build"

REM Build first
echo Building tests...
cmake --build . --config Release

echo.
echo Running tests...
echo.

REM Run all tests
ctest -C Release --output-on-failure --verbose

echo.
echo ======================================
echo Test run complete!
echo ======================================

endlocal
