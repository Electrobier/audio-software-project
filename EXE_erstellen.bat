@echo off
setlocal
cd /d "%~dp0"
if not exist ".venv\Scripts\python.exe" (
  echo Zuerst Einrichten.bat ausfuehren.
  pause
  exit /b 1
)
".venv\Scripts\python.exe" -m pip install "pyinstaller>=6.11,<7"
if errorlevel 1 goto error
".venv\Scripts\python.exe" -m PyInstaller --noconfirm --clean SampleIntelligencePro.spec
if errorlevel 1 goto error
echo Fertig: dist\SampleIntelligencePro\SampleIntelligencePro.exe
echo Den gesamten Ordner SampleIntelligencePro weitergeben, nicht nur die EXE.
pause
exit /b 0
:error
echo EXE-Erstellung fehlgeschlagen. Fehlermeldung oben pruefen.
pause
exit /b 1
