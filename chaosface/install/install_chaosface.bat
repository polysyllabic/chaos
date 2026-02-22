@echo off
setlocal

echo DEPRECATED: use install_chaosface_standalone.bat instead. 1>&2
call "%~dp0install_chaosface_standalone.bat" %*
exit /b %ERRORLEVEL%
