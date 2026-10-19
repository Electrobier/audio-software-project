@echo off
setlocal
cd /d "%~dp0"
echo SAMPLE INTELLIGENCE PRO - lokale Einrichtung
echo Tipp: Die fertige Windows-EXE braucht keine Python-Einrichtung.
echo Download: https://github.com/Electrobier/audio-software-project/releases/latest
echo.
if exist "SampleIntelligencePro.exe" (
  echo Das fertige Programm ist bereits vorhanden. Bitte SampleIntelligencePro.exe starten.
  pause
  exit /b 0
)
if exist ".venv\Scripts\python.exe" (
  ".venv\Scripts\python.exe" -c "import struct,sys; sys.exit(0 if struct.calcsize('P')==8 and (3,10)<=sys.version_info[:2]<=(3,13) else 1)" >nul 2>nul
  if not errorlevel 1 goto dependencies
  echo Die vorhandene .venv ist unpassend. Bitte diesen Quellcode in einen neuen Ordner entpacken.
  goto error
)
set "SIP_PYTHON="
call :probe py -3.12
if defined SIP_PYTHON goto createvenv
call :probe py -3.13
if defined SIP_PYTHON goto createvenv
call :probe py -3.11
if defined SIP_PYTHON goto createvenv
call :probe py -3.10
if defined SIP_PYTHON goto createvenv
call :probe python
if defined SIP_PYTHON goto createvenv
where py >nul 2>nul
if errorlevel 1 goto missing
 echo Keine passende Python-Version gefunden. Python Install Manager wird versucht.
 echo Python 3.12 wird bei vorhandenem Install Manager aus dem Internet installiert.
py install 3.12
if errorlevel 1 goto missing
call :probe py -3.12
if not defined SIP_PYTHON goto missing
:createvenv
echo Verwende: %SIP_PYTHON%
%SIP_PYTHON% -m venv .venv
if errorlevel 1 goto error
:dependencies
echo Die Ersteinrichtung laedt Python-Pakete aus dem Internet.
echo Danach funktioniert die Anwendung offline.
".venv\Scripts\python.exe" -m pip install --upgrade pip
if errorlevel 1 goto error
".venv\Scripts\python.exe" -m pip install -r requirements.txt
if errorlevel 1 goto error
echo Fertig. Anwendung mit Starten.bat starten.
pause
exit /b 0
:probe
%* -c "import struct,sys; sys.exit(0 if struct.calcsize('P')==8 and (3,10)<=sys.version_info[:2]<=(3,13) else 1)" >nul 2>nul
if not errorlevel 1 set "SIP_PYTHON=%*"
exit /b 0
:missing
echo Es wurde kein passendes Python 3.10 bis 3.13, 64 Bit gefunden.
echo Mit dem neuen Python Install Manager: py install 3.12
echo Alternativ Python 3.12 x64 von python.org installieren und dann erneut versuchen.
echo Einfacher: Das fertige Windows-Paket von der oben genannten Downloadseite nutzen.
:error
echo Einrichtung fehlgeschlagen. Die konkrete Fehlermeldung steht oben.
pause
exit /b 1
