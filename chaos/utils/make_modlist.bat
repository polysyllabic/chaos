@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PY_SCRIPT=%SCRIPT_DIR%src\make_modlist.py"

where py >NUL 2>&1
if not errorlevel 1 (
  py -3 "%PY_SCRIPT%" %*
  exit /b %ERRORLEVEL%
)

python "%PY_SCRIPT%" %*
exit /b %ERRORLEVEL%
