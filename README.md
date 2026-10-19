# audio-software-project
Entwicklung einer Audio-Software für Windows

## SAMPLE INTELLIGENCE PRO herunterladen

**[Vollständiges Projekt als ZIP herunterladen](https://github.com/Electrobier/audio-software-project/archive/refs/heads/sample-intelligence-pro-download.zip)**

**[Programmdateien und Anleitung ansehen](https://github.com/Electrobier/audio-software-project/tree/sample-intelligence-pro-download)**

Das ZIP enthält den ausführbaren Python-Quellcode, die Desktop-Oberfläche, das lokale KI-Modell sowie Windows-Start- und Buildskripte.

### Start unter Windows

1. ZIP herunterladen und vollständig entpacken.
2. Python 3.12, 64 Bit installieren.
3. `Einrichten.bat` ausführen (Ersteinrichtung benötigt Internet).
4. `Starten.bat` öffnen. Die Anwendung läuft danach offline.

### Windows-EXE

Ein automatischer Windows-Build ist eingerichtet. **[Build und verfügbare Windows-Downloads ansehen](https://github.com/Electrobier/audio-software-project/actions/runs/39232525219)**. Nach erfolgreichem Build steht unten auf der Seite unter „Artifacts“ das ZIP `SampleIntelligencePro-Windows-x64` bereit (GitHub-Anmeldung erforderlich). Den gesamten ZIP-Inhalt entpacken und `SampleIntelligencePro.exe` starten. Erst ein erfolgreicher Build mit vorhandenem Artefakt bedeutet, dass dieses Windows-Paket tatsächlich verfügbar ist.

Alternativ erstellt `EXE_erstellen.bat` nach der lokalen Einrichtung das Windows-Paket.

Der Quellcode-Download wurde von GitHub zurückgeladen und einschließlich der Prüfsumme des enthaltenen KI-Modells kontrolliert. Funktionsumfang, verbleibende Grenzen und Prüfstatus stehen in der README und im Prüfbericht im Programm-Branch.
