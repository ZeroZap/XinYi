@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "PYTHON=%REPO_ROOT%\tools\.venv\Scripts\python.exe"

if not exist "%PYTHON%" (
    echo tools\.venv is missing. Run: tools\z-serial-setup.cmd 1>&2
    exit /b 1
)

pushd "%REPO_ROOT%" || exit /b 1
set "PYTHONPATH=tools"
"%PYTHON%" -m xy_host_tools.serial_cli gui %*
set "EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %EXIT_CODE%
