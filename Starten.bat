@echo off
setlocal
cd /d "%~dp0"
if exist "dist\SampleIntelligencePro\SampleIntelligencePro.exe" (
  start "" "dist\SampleIntelligencePro\SampleIntelligencePro.exe"
  exit /b 0
)
if not exist ".venv\Scripts\python.exe" (
  echo Bitte zuerst Einrichten.bat ausfuehren. Python 3.12, 64 Bit wird benoetigt.
  pause
  exit /b 1
)
".venv\Scripts\python.exe" main.py
if errorlevel 1 pause
