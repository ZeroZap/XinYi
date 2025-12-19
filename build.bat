@echo off
REM Build script for XY Framework (Windows)

REM Function to show usage
goto :start

:show_usage
echo Usage: %0 [make^|cmake] [target]
echo   make  - Use Make build system
echo   cmake - Use CMake build system
echo.
echo Targets:
echo   all     - Build all components (default)
echo   clean   - Clean build artifacts
echo   test    - Run tests
echo   install - Install components
echo.
echo Examples:
echo   %0 make all
echo   %0 cmake test
exit /b 1

:start
REM Check arguments
if "%1"=="" (
    call :show_usage
    exit /b 1
)

set BUILD_SYSTEM=%1
set TARGET=%2
if "%TARGET%"=="" set TARGET=all

REM Build based on system
if "%BUILD_SYSTEM%"=="make" (
    echo Building with Make...
    make %TARGET%
) else if "%BUILD_SYSTEM%"=="cmake" (
    echo Building with CMake...
    if not exist build mkdir build
    cd build
    cmake ..
    cmake --build . --config Release --target %TARGET%
    cd ..
) else (
    echo Unknown build system: %BUILD_SYSTEM%
    call :show_usage
    exit /b 1
)