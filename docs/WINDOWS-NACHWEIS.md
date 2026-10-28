# Windows-Nachweis – 28. Oktober 2026

[Erfolgreicher Windows-Prüflauf](https://github.com/Electrobier/audio-software-project/actions/runs/40678724592)

Validierter Quellcode: `3ace963d3998b0bc7fa4658210fe34b000da1943`. Build auf GitHub Actions `windows-2022`, Visual Studio 2022, MSVC 19.44, Windows SDK 10.0.26100, Release x64, JUCE 8.0.6.

- Vollständiges Windows-VST3-Bundle und Standalone-EXE kompiliert.
- Beide CTest-Suiten DSP und Plugin bestanden.
- Plugin-Integration inklusive samplegenauem MIDI-Start, Note-Priority, MIDI Learn ohne Editor, Zustandswiederherstellung, Host-Sprüngen und sechs Buffergrößen bestanden.
- Null C++-new-Allokationen im geprüften Audio-Callback-Pfad.
- Windows-GUI-Snapshots bei 100/125/150/200 Prozent erzeugt.
- Standalone blieb beim automatischen Starttest fünf Sekunden aktiv; keine Audiohardware dabei geprüft.
- 32 Bassline-WAVs und zehn Kick-WAVs unter Windows gerendert.
- Alle 32 SMF-Dateien und 42 WAVs unabhängig mit Python geprüft; Spektraltest bestanden.
- Offizieller Steinberg-VST3-Validator am Windows-Bundle: **47 bestanden, 0 fehlgeschlagen**.
- Das Windows-VST3 wurde als PE-Datei erkannt und mit vollständiger Bundle-Struktur verpackt.

Die Rohberichte unter `windows-*` wurden vom Runner erzeugt. Die Download-ZIP und SHA256 liegen auf dem separaten Branch unter `Downloads/`. Am validierten Synthesizer-Code wurde für die anschließende Bereitstellung nichts geändert.

Manuell offen bleiben Cubase 15 Pro, physische Windows-10/11-Systeme, echte MIDI-/Audiohardware, ASIO, native Dateidialog-Bedienung, Automation/Projekt-Neuladen im realen Host und praxisgerechtes Abhören. Das Plugin ist eine eigene 303-inspirierte Entwicklung, keine validierte identische Emulation des unbekannten Originals.

Der erste Windows-Lauf baute bereits die Binärdateien, scheiterte aber bei der Bereinigung einer im Test noch geöffneten MIDI-Datei. Die Datei wird nun vor dem Löschen geschlossen; der vollständige korrigierte Windows-Lauf ist oben verlinkt.
