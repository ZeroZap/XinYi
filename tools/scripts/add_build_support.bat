@echo off
REM Script to add Makefile, CMakeLists.txt, and Kconfig support to all components

REM Create build support for all components
echo Adding build support to all components...

REM Process each component directory
for /D %%d in (components\*) do (
    call :process_component "%%d"
)

echo Build support added to all components!
goto :eof

:process_component
set component_dir=%1
set component_name=%~n1

echo Processing component: %component_name%

REM Skip hidden directories
if "%component_name:~0,1%"=="." goto :eof

REM Check if Makefile exists, create if not
if not exist "%component_dir%\Makefile" (
    echo   Creating Makefile...
    powershell -Command "(Get-Content utils\templates\Makefile.template) -replace '@COMPONENT_NAME@', '%component_name%' | Set-Content '%component_dir%\Makefile'"
)

REM Check if CMakeLists.txt exists, create if not
if not exist "%component_dir%\CMakeLists.txt" (
    echo   Creating CMakeLists.txt...
    powershell -Command "(Get-Content utils\templates\CMakeLists.txt.template) -replace '@COMPONENT_NAME@', '%component_name%' | Set-Content '%component_dir%\CMakeLists.txt'"
)

REM Check if Kconfig exists, create if not
if not exist "%component_dir%\Kconfig" (
    echo   Creating Kconfig...
    set component_name_uc=
    for /f "delims=" %%i in ('powershell -Command "'%component_name%'.ToUpper()"') do set component_name_uc=%%i
    powershell -Command "(Get-Content utils\templates\Kconfig.template) -replace '@COMPONENT_NAME@', '%component_name%' | Set-Content '%component_dir%\Kconfig.tmp'"
    powershell -Command "(Get-Content '%component_dir%\Kconfig.tmp') -replace '@COMPONENT_NAME_UC@', '%component_name_uc%' | Set-Content '%component_dir%\Kconfig'"
    powershell -Command "(Get-Content '%component_dir%\Kconfig') -replace '@COMPONENT_DESCRIPTION@', 'various functionalities' | Set-Content '%component_dir%\Kconfig'"
    del "%component_dir%\Kconfig.tmp"
)

goto :eof