# audio-software-project
Entwicklung einer Audio-Software für Windows

## SAMPLE INTELLIGENCE PRO – fertige Windows-Anwendung

**[Windows-ZIP direkt herunterladen (ca. 170 MB)](https://github.com/Electrobier/audio-software-project/releases/download/sample-intelligence-pro-v1.0.0/SampleIntelligencePro-Windows-x64.zip)**

**[Release-Seite mit Download öffnen](https://github.com/Electrobier/audio-software-project/releases/tag/sample-intelligence-pro-v1.0.0)**

1. ZIP vollständig entpacken.
2. Im Ordner `SampleIntelligencePro` die Datei **SampleIntelligencePro.exe** starten.
3. Den Unterordner `_internal` und die anderen mitgelieferten Dateien zusammenlassen.

**Python und `Einrichten.bat` sind für dieses fertige Windows-Paket nicht nötig.**

### Geprüft

Auf GitHub unter Windows x64 gebaut. Alle **15 Funktionstests bestanden**; anschließend wurde die gepackte EXE erfolgreich gestartet und mit DSP-Audioanalyse, lokalem ONNX-KI-Modell, SQLite, Qt-Oberfläche, Cosinussuche und Hintergrunddekodierung getestet. Qt lief im automatisierten Test offscreen. Echte Audiohardware und Cubase-Drag-and-drop müssen noch praktisch geprüft werden.

[Windows-Prüflauf ansehen](https://github.com/Electrobier/audio-software-project/actions/runs/39232525219)

### Quellcode und Anleitung

**[Vollständigen Quellcode als ZIP herunterladen](https://github.com/Electrobier/audio-software-project/archive/refs/heads/sample-intelligence-pro-download.zip)**

[Programmdateien und ausführliche Anleitung ansehen](https://github.com/Electrobier/audio-software-project/tree/sample-intelligence-pro-download)

Nur beim Start aus dem Quellcode: ein passendes Python 3.10–3.13 (64 Bit) verwenden, `Einrichten.bat` und danach `Starten.bat` ausführen. Das Einrichtungsskript sucht vorhandene Versionen und versucht bei Bedarf `py install 3.12` mit dem Python Install Manager. Die Ersteinrichtung benötigt Internet; die Anwendung selbst läuft offline. `EXE_erstellen.bat` erstellt alternativ lokal das Windows-Paket.

Funktionsumfang, fachliche Grenzen und Prüfstatus stehen in README.md und PRUEFBERICHT.md im Programm-Branch. Das Modell erkennt grobe Instrumente und Ereignisse; spezielle Unterkategorien können eigene gelabelte Beispiele benötigen.
