@echo off
setlocal

rem XinYi Crypto Windows helper.
rem
rem The historical version of this script compiled src/xy_*.c files directly.
rem Most crypto algorithms now use module-directory sources through the canonical
rem CMake target, so this wrapper delegates to the repository CMake build instead
rem of naming individual implementation files that may no longer exist.

set SCRIPT_DIR=%~dp0
for %%I in ("%SCRIPT_DIR%..\..") do set REPO_ROOT=%%~fI

if "%BUILD_DIR%"=="" set BUILD_DIR=%REPO_ROOT%\build\pc
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
if "%HAL_PLATFORM%"=="" set HAL_PLATFORM=PC

echo XinYi Crypto CMake Build Wrapper
echo ================================
echo Repository : %REPO_ROOT%
echo Build dir  : %BUILD_DIR%
echo Platform   : %HAL_PLATFORM%
echo Build type : %BUILD_TYPE%
echo.

cmake -B "%BUILD_DIR%" -S "%REPO_ROOT%" -DHAL_PLATFORM=%HAL_PLATFORM% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --target xy_tiny_crypto --config %BUILD_TYPE%
if errorlevel 1 exit /b %errorlevel%

echo.
echo Crypto target build completed: xy_tiny_crypto
echo Optional unit gate from the repository root:
echo   make test-unit

endlocal
