@echo off
setlocal
cd /d "%~dp0"
echo SAMPLE INTELLIGENCE PRO - lokale Einrichtung
echo Die Ersteinrichtung laedt Python-Pakete aus dem Internet.
echo Danach funktioniert die Anwendung vollstaendig offline.
where py >nul 2>nul
if not errorlevel 1 (
  py -3.12 -m venv .venv
) else (
  python -m venv .venv
)
if errorlevel 1 goto error
".venv\Scripts\python.exe" -c "import struct,sys; assert struct.calcsize('P')==8 and (3,10)<=sys.version_info[:2]<=(3,13), 'Python 3.10 bis 3.13 (64 Bit) erforderlich'"
if errorlevel 1 goto error
".venv\Scripts\python.exe" -m pip install --upgrade pip
if errorlevel 1 goto error
".venv\Scripts\python.exe" -m pip install -r requirements.txt
if errorlevel 1 goto error
echo Fertig. Anwendung mit Starten.bat starten.
pause
exit /b 0
:error
echo Einrichtung fehlgeschlagen. Bitte Python 3.12 x64 von python.org installieren.
echo Bei der Installation Python dem PATH hinzufuegen und dann erneut versuchen.
pause
exit /b 1
