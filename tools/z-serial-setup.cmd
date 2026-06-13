@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "PYTHON=%REPO_ROOT%\tools\.venv\Scripts\python.exe"

pushd "%REPO_ROOT%" || exit /b 1
py -3 -m venv tools\.venv
if errorlevel 1 (
    python -m venv tools\.venv
    if errorlevel 1 exit /b 1
)

"%PYTHON%" -m pip install ^
    -i https://pypi.tuna.tsinghua.edu.cn/simple ^
    --trusted-host pypi.tuna.tsinghua.edu.cn ^
    --timeout 120 ^
    -r tools\requirements-z-serial.txt
if errorlevel 1 exit /b 1

echo z-serial environment ready. Start with: tools\z-serial.cmd
popd
