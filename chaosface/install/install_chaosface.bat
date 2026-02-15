@echo off
setlocal

set SCRIPT_DIR=%~dp0

where py >nul 2>&1
if %ERRORLEVEL% EQU 0 (
  py -3 "%SCRIPT_DIR%install_chaosface.py" %*
  exit /b %ERRORLEVEL%
)

where python >nul 2>&1
if %ERRORLEVEL% EQU 0 (
  python "%SCRIPT_DIR%install_chaosface.py" %*
  exit /b %ERRORLEVEL%
)

echo Could not find a Python launcher. Install Python 3.10+ and try again.
exit /b 1
