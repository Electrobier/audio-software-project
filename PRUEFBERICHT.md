# Prüfbericht – 19. Oktober 2026

## Tatsächlich ausgeführt

- Python 3.12 unter Linux x64; Qt-Oberfläche im Offscreen-Modus gestartet und als Bild aufgenommen.
- **15 automatisierte Tests bestanden** (`15 passed in 6.23s`). Sie prüfen Audioanalyse, PCM-Identität über Container/Bit-Tiefen, float64-Präzision, Filtern, KI-/DSP-Suche, Kategoriezuordnung, inkrementelle Scans, Verschiebeerkennung, unverändernde Exporte, Duplikatauswahl sowie Stereo-/Loop-/Pause-Audiocallbacks ohne Hardware.
- Vortrainiertes EfficientAT-Modell geladen und mit ONNX Runtime ausgeführt; Embedding mit 384 Komponenten.
- NumPy-Vorverarbeitung gegen Original-PyTorch/torchaudio-Vorverarbeitung verglichen: maximale Abweichung 3,82e-6.
- ONNX-/PyTorch-Logits beim Export verglichen: maximale Abweichung 9,54e-6.
- Gesamttest der Python-Anwendung: DSP → ONNX → SQLite → Qt-Tabelle → Cosinussuche → Hintergrunddekodierung erfolgreich.
- **PyInstaller-Paket unter Linux gebaut und denselben Gesamttest erfolgreich darin ausgeführt.** Dabei wurde ein Numba-/librosa-Cacheproblem gefunden und durch mitgelieferte Python-Quelldateien und einen beschreibbaren Cachepfad behoben.
- 12 echte synthetische Audiodateien für einen Oberflächentest erzeugt, analysiert und angezeigt; keine erfundenen Datenbankeinträge für die Vorschau.
- Datenbanktest mit **100.001 synthetischen Einträgen**: Einfügen 0,471 s; Filterzählung und Seite 0,028 s; vollständige Suche mit 9-dimensionalen Spektralvektoren 0,468 s, begrenzt auf 1.000 angezeigte Ergebnisse. Das sind Messwerte der Testumgebung, keine zugesicherte Leistung auf Benutzerhardware oder bei 384-dimensionalen KI-Vektoren.

## Nicht ausgeführt / keine Abnahmebehauptung

- Windows-10/11-Build oder Start einer Windows-EXE.
- Hörtest mit echten Audiogeräten, Mikrofon-/Windows-Loopbackaufnahme oder Mehrgeräteverhalten.
- Verschieben echter Benutzerdateien in den Windows-Papierkorb.
- Tatsächliches Drag-and-drop nach Cubase 15 Pro.
- Analyse und Bedienung einer echten Bibliothek von mehr als 100.000 Audiodateien.
- Validierung der Instrumenten-/BPM-/Tonarterkennung auf einem repräsentativen, gelabelten Sample-Datensatz.

Die Windows-Startskripte und die PyInstaller-Spezifikation liegen bereit. Der Quellcode ist ausführbar; eine vollständige professionelle Windows-Abnahme aller Anforderungen ist noch offen. Fachliche Grenzen und nicht implementierte optionale Funktionen sind in README.md beschrieben.

Maschinenlesbare Berichte und der echte Screenshot liegen unter docs/.
