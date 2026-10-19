# SAMPLE INTELLIGENCE PRO

Lokaler Audio-Sample-Manager für Windows 10/11 x64. Python, PySide6/Qt, librosa, SQLite und ein mitgeliefertes EfficientAT-Audiomodell. Die Oberfläche folgt der gelieferten Referenz: linke Navigation, Stereo-Wellenform oben, kompakte Metadatentabelle, Kategorienraster unten. Ein zusätzlicher Inspektor ist ausblendbar und dockbar.

## Sofort starten unter Windows

1. Diesen vollständigen Ordner auf ein lokales Laufwerk entpacken.
2. **Python 3.12, 64 Bit** von python.org installieren. „Add Python to PATH“ aktivieren.
3. **Einrichten.bat** doppelklicken. Nur die Ersteinrichtung benötigt Internet zum Installieren der Python-Pakete.
4. **Starten.bat** doppelklicken.
5. **+ Bibliothek** wählen und einen Sample-Ordner hinzufügen. Der erste Scan berechnet die Analysewerte im Hintergrund.

Bei der ersten Analyse kann die Initialisierung von librosa/Numba etwas dauern. Ein leerer Browser vor dem ersten Import ist beabsichtigt. Es werden keine erfundenen Bibliotheksdaten angezeigt.

Zum ersten Ausprobieren kann `python tools/beispielsamples.py` ausgeführt werden. Das erzeugt echte synthetische WAV-Dateien im Ordner `Beispielsamples`; diesen danach als Bibliothek hinzufügen. Diese Signale sind Funktionstestmaterial, kein kommerzielles Sample-Pack.

### Eigenständiges Windows-Paket erstellen

Nach der Einrichtung **EXE_erstellen.bat** ausführen. Das erzeugt:

```
dist\SampleIntelligencePro\SampleIntelligencePro.exe
```

**Den gesamten Ordner `SampleIntelligencePro` weitergeben.** Die EXE benötigt die daneben enthaltenen Bibliotheken und das mitgelieferte Modell. Auf dem Ziel-PC ist dann kein Python erforderlich. Die App hat keine Registrierung oder Aktivierung. Das Paket ist nicht signiert; Windows kann deshalb eine SmartScreen-Abfrage anzeigen.

**Lieferstatus:** Der Quellcode und die Start-/Buildskripte sind vorhanden. Die Anwendung wurde in einer Linux-Testumgebung ausgeführt. Eine fertig gebaute oder auf Windows getestete EXE liegt diesem Quellpaket nicht bei. Die Datei `.github/workflows/windows-build.yml` enthält zusätzlich einen manuell startbaren Windows-Build für GitHub Actions; dieser wurde hier nicht ausgelöst.

## Bedienung

- **Ordner:** Mehrere Bibliotheken hinzufügen. Rechtsklick auf einen Bibliothekseintrag zum Öffnen oder Entfernen aus dem Index. Originaldateien werden dabei nicht entfernt. Unterordner erscheinen aus dem Index, maximal 1.000 Ordner pro Bibliothek in der Navigation.
- **Scannen:** Neue/geänderte Dateien analysieren. Unveränderte Dateien werden übersprungen. „Abbrechen“ beendet nach dem aktuellen DSP-Schritt; erneutes Scannen setzt fort. Optional werden Ordneränderungen jede Minute geprüft. Nicht erreichbare Bibliotheken werden protokolliert, ohne alle Dateien als gelöscht zu markieren.
- **Verschobene Dateien:** Innerhalb derselben Bibliothek bleibt bei eindeutigem SHA-256-Treffer zu genau einem fehlenden Eintrag die Identität mit Tags, Favoriten und Projektzuordnungen erhalten. Mehrdeutige oder bibliotheksübergreifende Umzüge werden als fehlende/neu gefundene Dateien geführt.
- **Suche:** Dateiname, Tags und Pfad. Typ, Format, Kategorien, Bibliothek, Projekt und Favoriten sowie erweiterte Zahlenbereiche sind kombinierbar. Tonarten werden als z. B. `C Moll` eingegeben. Suchabfragen können gespeichert werden.
- **Tabelle:** Klick auf den Spaltenkopf sortiert. Spalten durch Ziehen verschieben und vergrößern; Rechtsklick auf den Kopf zum Ein-/Ausblenden. Es werden 500 Zeilen pro Seite geladen. Strg/Shift dient zur Mehrfachauswahl innerhalb einer Seite.
- **Player:** Leertaste für Play/Pause; Escape stoppt. Mausklick in die Wellenform setzt die Abspielposition, Mausrad oder Regler zoomt. Vor/Zurück, ±5 Sekunden, Loop, Lautstärke und optionales Auto-Vorhören sind enthalten. Unter „Ansicht“ ist das Ausgabegerät wählbar. Analyse und Wiedergabe verwenden getrennte Worker/Audio-Callbacks.
- **Find Similar:** Sample auswählen oder externe Referenzdatei laden. Modi: allgemeiner Klang, neuronales Embedding, Rhythmus, Klangfarbe, Dynamik und Frequenzcharakteristik. Gewichte und Mindestscore einstellen. Die aktuellen Filter begrenzen die Treffer. Die gesamte passende Bibliothek wird durchsucht; die besten 1.000 Treffer werden angezeigt. Der Score ist eine Merkmalsähnlichkeit, keine Wahrscheinlichkeit.
- **Live-Suche:** „Record Audio“, Eingabegerät und 1–30 Sekunden wählen. Danach dieselbe Ähnlichkeitssuche. Aufnahme und externe Referenzen werden nicht in die Bibliothek aufgenommen. Temporäre Aufnahmen werden beim Beenden entfernt.
- **Kategorien:** Alle 31 geforderten Kategorien sind vorhanden; eigene lassen sich hinzufügen. Mehrfachzuordnung und manuelle Korrekturen sind möglich. Rechts im Inspektor sieht man Quelle und Score. Manuelle Zuordnungen dienen als Trainingsbeispiele. „Organisieren → Klassifikator auf Bibliothek anwenden“ überträgt ähnliche Beispiele; manuelle Labels werden nicht überschrieben.
- **Projekte und Sammlungen:** Über die linke Schaltfläche erstellen. Samples per Drag-and-drop auf einen Eintrag ziehen oder über das Kontextmenü zuordnen. Favoriten können ebenfalls als Drop-Ziel dienen. Organisation verändert die Audiodateien nicht.
- **Export:** Auswahl markieren und Strg+E. Originaldateien werden bitgenau kopiert. Nummerierung, Präfix/Suffix und Ordnerstruktur sind wählbar. Vorhandene Zieldateien werden übersprungen und gemeldet. Nie wird eine Originaldatei umbenannt oder überschrieben.
- **Cubase:** Aus der Tabelle werden echte lokale Datei-URLs per Qt/Windows-Drag-and-drop angeboten. Diese können in Cubase oder den Explorer gezogen werden. Alternativ in den Cubase-Projektordner exportieren. Das ist eine Dateischnittstelle, kein Cubase-Plug-in und keine Tempo-/Transportfernsteuerung. Drag-and-drop kann scheitern, wenn Cubase und diese App mit unterschiedlichen Administratorrechten laufen.
- **Duplikate:** „Organisieren → Duplikate prüfen“. Gruppen unterscheiden bytegleiche Dateien, identische dekodierte PCM-Inhalte und ähnliche Kandidaten. Keine Auswahl ist vorab markiert. Vor dem Verschieben in den Systempapierkorb erscheint eine Dateiliste zur Bestätigung. Pro angezeigter Gruppe muss mindestens eine Datei bleiben. Der Windows-Papierkorb kann vom System, Laufwerk oder Netzwerkpfad abhängen; bei Fehlern wird nicht permanent gelöscht.

## Audioanalyse und KI

Die Datenbank speichert Dateieigenschaften, Sample-Rate/Kanäle/Codec, Dauer, Peak/RMS, geschätzten True Peak, LUFS, Crest, Dynamik, Stilleanteil, spektrale Werte, neun Frequenzbänder, harmonisch/perkussive Anteile, Transienten, BPM/Beat-Positionen, geschätzte Tonart, Loop-/One-Shot-Typ, Attack/Decay und Merkmalsvektoren.

**Messgrenzen sind bewusst sichtbar:**

- SHA-256, dekodierter PCM-Hash, Sample-Peak, RMS und Stilleanteil beziehen sich auf die vollständige Datei. Native PCM-Identität wird mit float64-Dekodierung geprüft, damit unterschiedliche 64-Bit-Signale nicht durch float32-Rundung als identisch gelten.
- Spektral-, Rhythmus-, Hüllkurven- und Fingerprint-Analyse verwendet höchstens die ersten **90 Sekunden**. Der Player zeigt nach dem Laden die vollständige Wellenform.
- LUFS wird mit pyloudnorm für vollständige Dateien zwischen 400 ms und 90 s und höchstens fünf Kanälen bestimmt; sonst unbekannt. Dies ist kein zertifizierter Broadcast-Meter.
- True Peak ist eine **4×-Oversampling-Schätzung** des Analyseausschnitts. Dynamik ist P95 minus P10 der Kurzzeit-RMS, **kein EBU-LRA**. Attack/Decay sind Hüllkurven-Schätzungen. „Helligkeit“ ist der Energieanteil oberhalb 2 kHz im 22,05-kHz-Analyseton.
- BPM wird nur für hinreichend regelmäßige erkannte Beats ausgegeben, Tonart nur bei ausreichendem Harmonie-/Profilscore. Stille und unsichere Ergebnisse bleiben unbekannt. Diese Schwellen sind Heuristiken und kein Beweis musikalischer Richtigkeit. Auch Loop-Erkennung kann sich irren.
- Das enthaltene **EfficientAT `mn04_as`** wurde auf AudioSet trainiert. Es liefert 384-dimensionale neuronale Embeddings und 527 Ereignis-Scores. Für die Ausführung wird ausschließlich ONNX Runtime auf der CPU verwendet; PyTorch wird auf dem Ziel-PC nicht benötigt.
- Das Modell erkennt grobe Instrumente und Ereignisse. **Es hat keine vortrainierte zuverlässige Unterscheidung aller geforderten Spezialkategorien** wie Synth Pad/Lead/Pluck, Sub Bass oder Breakbeat. Solche Kategorien lassen sich mit eigenen manuell gelabelten Beispielen über den lokalen Nächste-Beispiele-Klassifikator lernen. Ohne sicheren Treffer bleibt die Kategorie unbekannt. Ein Modellscore ist nicht kalibriert; Beispiel-Konfidenzen sind Cosinusähnlichkeiten.
- Die Ähnlichkeitssuche verwendet echte KI-/DSP-Vektoren und Cosinusmetriken. SQLite speichert die Vektoren getrennt von den großen Analysedaten. Es wird eine blockweise exakte lineare Suche verwendet, kein FAISS-Index. Das verhindert einen zusätzlichen Indexabgleich, die Laufzeit wächst aber mit der Bibliotheksgröße.
- Nahe Duplikate werden über normalisierte, resampelte Fingerprints und zeit-/spektrumbasierte Kandidaten gesucht. Die Kandidatensuche begrenzt Vergleiche pro Bucket auf 64 und die Anzeige auf 2.000 Gruppen. **Es gibt keine Vollständigkeitsgarantie für alle Normalisierungs-, Pitch-, Schnitt- oder Codecvarianten.** Ähnlichkeit allein weist keine Audioidentität nach.

WAV/AIFF/FLAC/OGG und je nach libsndfile-Version MP3 werden nativ dekodiert. Für zusätzliche Formate wie M4A/AAC/WMA kann eine separate FFmpeg-Installation im PATH nötig sein. FFmpeg ist nicht im Paket enthalten. Die App meldet Decoderfehler im Analyseprotokoll.

## Offline und lokale Daten

Zur Laufzeit gibt es keine Download-, Cloud-, Registrierungs- oder Uploadfunktion. ONNX-Telemetrie wird deaktiviert. Das Modell ist mitgeliefert. Audiodateien und Analysen bleiben lokal. Die Installation der Abhängigkeiten und ein möglicher GitHub-Actions-Build benötigen Internet; diese Schritte sind von der lokalen Anwendung getrennt.

Datenbank: Unter Windows normalerweise `%LOCALAPPDATA%\SampleIntelligence\Sample Intelligence Pro\library.sqlite3` (Qt bestimmt den endgültigen Pfad). „Hilfe → Datenordner öffnen“ zeigt den tatsächlichen Ort. Für eine Sicherung die Anwendung schließen und diesen Ordner kopieren. Oberflächeinstellungen werden über QSettings im Benutzerprofil gespeichert. Ohne Bibliothekseinträge werden keine Laufwerke selbstständig durchsucht.

Der Player dekodiert die ausgewählte Datei vollständig in den RAM. Für typische Samples ist das schnell; sehr lange Mehrkanalaufnahmen sind kein optimierter Streaming-Anwendungsfall.

## Entwicklung und Verifikation

```bash
python -m venv .venv
# Windows: .venv\Scripts\activate
# Linux/macOS: source .venv/bin/activate
python -m pip install -r requirements.txt pytest
python main.py
python -m pytest -q
python main.py --self-test selftest.json
python tools/benchmark.py
```

Unter Linux zusätzlich PortAudio (`libportaudio2`) und die Qt-Systembibliotheken installieren. Auf kopflosen Testsystemen `QT_QPA_PLATFORM=offscreen` setzen. Die Windows-Pakete von sounddevice enthalten die PortAudio-DLL.

Die Tests prüfen Messwerte an erzeugten Signalen, Container-/Bit-Tiefen-Identität, float64-Unterschiede, Audioähnlichkeit, kombinierte Filter, Incremental Scan, Verschiebeerkennung, Kategorienlernen, Exportkonflikte, Stereo-/Loop-/Pause-Callbacks und die Duplikat-Auswahlregeln. `--self-test` prüft zusätzlich die echte ONNX-Inferenz, Worker-Dekodierung und das Qt-Fenster. Der beigefügte Prüfbericht benennt, was tatsächlich ausgeführt wurde.

**Noch auf einem Windows-PC zu prüfen:** EXE-Build und Start, echte Audioausgabe, Mikrofon-/Loopbackgeräte, Windows-Papierkorb, Cubase 15 Drag-and-drop und eine echte Bibliothek mit mehr als 100.000 Audiodateien. Der synthetische 100.001-Zeilen-Benchmark prüft lediglich Datenbank und Suchpfad, keine entsprechende Audio-Sammlung oder Echtzeit-Garantie.

Optionale Anforderungen Pitch-Shifting, Time-Stretching, Host-Tempo-Synchronisation und ein Cubase-Plug-in sind nicht implementiert. Der übermittelte Auftrag endete im Abschnitt Cubase-Integration nach einem leeren Aufzählungspunkt; spätere Anforderungen lagen nicht vor.

## Modellherkunft

EfficientAT von Florian Schmid und Mitwirkenden: https://github.com/fschmid56/EfficientAT

Originalgewichte: `mn04_as_mAP_432.pt`, GitHub-Release `v0.0.1`; in ONNX exportiert, Inferenz numerisch mit PyTorch verglichen. Evaluierungs-Melbank und Fenster liegen in `models/preprocess.npz`. Die NumPy-Vorverarbeitung wurde gegen die Originalimplementierung verglichen. Modell- und Lizenzinformationen stehen in `models/README.md` und `models/LICENSE-EfficientAT.txt`.
