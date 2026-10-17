@echo off
cd /d "%~dp0"
python build_module.py
exit /b %errorlevel%
