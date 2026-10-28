# Psy303 Bassline

Eigenständiger monophoner Acid-/Psytrance-Synthesizer in C++20, JUCE 8.0.6 und CMake. Echte Audio-Engine, grafischer 64-Step-Editor, 32 eigene Factory-Presets, MIDI-/WAV-Export und separates Kick-Exportmodul. Eigene Vektorgrafik, angelehnt an die sichtbare silbergraue Referenz. Keine messtechnisch validierte Hardware-Emulation und kein identischer Nachbau des unbekannten Originals.

## Liefer- und Prüfstatus

**Windows-VST3 ist in diesem Paket noch nicht kompiliert.** Die Entwicklungsmaschine ist Linux. Ein Cross-Build mit MinGW wurde versucht; JUCE 8.0.6 lehnt diesen Compiler ausdrücklich ab. Der vorgesehene Windows-Weg ist Visual Studio 2022. Die beiliegende Windows-CI-Konfiguration wurde hier nicht ausgeführt.

Hier erfolgreich ausgeführt: vollständiger Linux-Release-Build von VST3 und Standalone, DSP- und Plugin-Integrationstests, offizieller Steinberg-Validator am Linux-VST3 (**47 bestanden, 0 fehlgeschlagen**), unabhängige SMF-/WAV-Prüfungen, Offline-Rendering aller 32 Basslines und 10 Kicks, GUI-Snapshots in vier Größen. Berichte unter `docs/`. Eine Linux-Binärdatei lässt sich nicht in Windows-Cubase laden.

## Windows 10/11 x64: bauen

1. Visual Studio 2022 mit **Desktopentwicklung mit C++**, MSVC v143 und Windows 10/11 SDK installieren.
2. CMake ab 3.22 installieren und zur PATH-Variable hinzufügen. PowerShell öffnen.
3. ZIP vollständig in einen beschreibbaren Ordner entpacken. JUCE liegt unter `ThirdParty/JUCE` bei. Internet wird für den normalen Build nicht benötigt.
4. Im Projektordner ausführen:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

Oder einzeln:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Debug:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1 -Configuration Debug
```

Ergebnis: `build/Psy303_artefacts/Release/VST3/Psy303 Bassline.vst3/Contents/x86_64-win/Psy303 Bassline.vst3`. Der **gesamte äußere Ordner** `Psy303 Bassline.vst3` ist das Plugin-Bundle. Standalone: `build/Psy303_artefacts/Release/Standalone/Psy303 Bassline.exe`.

Eine andere lokale JUCE-Kopie kann mit `-DJUCE_PATH=C:/Pfad/zu/JUCE` gewählt werden. Ohne beiliegendes oder lokales JUCE lädt CMake Version 8.0.6 von GitHub. Eine passende JUCE-Installation kann über `JUCE_DIR` verwendet werden.

## Installation und Cubase 15 Pro

Cubase schließen. Den äußeren VST3-Ordner nach `C:\Program Files\Common Files\VST3\` kopieren. Alternativ in einer PowerShell mit Administratorrechten:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-windows.ps1
```

Cubase starten, im VST-Plug-in-Manager neu scannen, Instrumentenspur mit **Psy303 Bassline** anlegen. Im Modus `SEQUENCER` folgt das Pattern dem laufenden Host. Bei gestopptem Host schaltet `PLAY / STOP` den Freilauf mit eigenem BPM ein. Für MIDI-Spuren oder Keyboard `EXTERNAL MIDI` wählen. Der Hoststart hat im Sequenzermodus Vorrang vor dem Freilaufschalter; den laufenden Host mit Cubase stoppen. `PANIC` beendet Noten und Freilauf. Cubase spielt bei laufendem Host im Sequenzermodus beim nächsten Sample weiter.

Wird das Plugin blockiert: Bundle-Struktur und x64-Build prüfen, Cubase-Blocklist kontrollieren. Eine Debug-Version ist für die Entwicklung, Release für die Produktion gedacht. MSVC-Laufzeit bei Bedarf als Microsoft Visual C++ Redistributable x64 installieren. Audiohardware und ASIO für die Standalone-Version separat konfigurieren.

## Bedienung

- Sechs Hauptregler: Tuning, Cutoff, Resonance, Env Mod, Decay, Accent. Weitere Regler: Volume, Slide Time, Drive, Fine.
- Shift ermöglicht feinere Reglersteuerung, Doppelklick setzt den Default zurück. Werte können auch eingegeben werden. Alle Hauptparameter und 64 Steps sind hostautomatisierbar.
- Factory-Dropdown: 01–08 Acid, 09–16 Rolling, 17–24 Dark, 25–32 Experimental. `<` / `>` wechseln Presets. Factory-Wechsel lädt Klang und Pattern. Gespeichert wird der vollständige aktuelle Parametersatz.
- Ein Step-Klick wählt einen Step. Die Reihen GATE, ACC und SLIDE schalten Aktivität, Accent und Slide. GATE aus bedeutet Rest. Unter dem Raster ändern `MIDI` und `gate` Tonhöhe und Gate-Länge. Mausrad über einem Step ändert Halbton, Shift+Mausrad die Oktave. Klaviaturklick spielt vor und setzt im Sequenzermodus die Tonhöhe des ausgewählten Steps.
- Seitenwahl zeigt Steps 1–16, 17–32, 33–48 oder 49–64. `steps` legt die tatsächlich abgespielte Länge fest. Transpose verändert das ganze Pattern. Division und Rhythm schalten Viertel bis Zweiunddreißigstel, gerade/punktiert/triolisch. Swing verschiebt die zweite Note eines Paares bei unveränderter Paardauer.
- Slide gehört zur **Quellnote** und bindet zur nächsten aktiven Note. Normale Steps triggern die Filterhüllkurve; legato bleibt sie erhalten. Accent kann während einer Bindung seine eigene Hüllkurve neu anstoßen.
- `SAVE AS` speichert einen vollständigen User-Zustand als `.psy303`, `SAVE` überschreibt die zuletzt geladene/gespeicherte Datei; ohne vorherigen Dateinamen öffnet SAVE den Dialog. `LOAD`, `INIT`, `UNDO` und `REDO` sind vorhanden. Cubase-Projektzustände umfassen Parameter, Sequenzerdaten und MIDI-Zuordnungen.

## MIDI

Note On/Off, Velocity, Last-Note-Priority mit Rückkehr zu gehaltenen Noten, Pitch Bend ±2 Halbtöne, CC1 Filtermodulation, CC65 Slide und CC80 Accent. Velocity ab 110 aktiviert Accent. MIDI-Kanäle werden für dieses monophone Instrument zusammengeführt. System-Exclusive-Nachrichten werden ignoriert. CC123/120 und PANIC beenden Noten; Sustain-Pedal-Unterstützung ist nicht implementiert.

MIDI Learn: Zielparameter aus `Learn:` wählen, MIDI LEARN drücken, Hardware-CC bewegen. Die Zuordnung bleibt auch ohne geöffneten Editor wirksam und wird im Projekt gespeichert. Die zehn Haupt-/Zusatzregler sind lernbar; Step-Parameter werden über Hostautomation gesteuert. Feste CC1/65/80 sollten für Learn möglichst nicht doppelt belegt werden. Im Editor werden gelernte CC-Werte zusätzlich in die Hostparameter gespiegelt; bei geschlossenem Editor wirken sie direkt im DSP und werden beim Speichern des Zustands berücksichtigt.

`EXPORT...` mit `MIDI pattern` erzeugt standardkonformes SMF Format 0, 960 PPQ, Tempo-Metaevent. Notenhöhen, Swing, Gate-Dauer und Accent-Velocity werden exportiert. Slide-Noten überlappen leicht. CC65 dokumentiert Slide und CC80 Accent. In fremden Instrumenten sind diese CCs nicht universell gleich belegt. Zum Wiederimport in Psy303 `EXTERNAL MIDI` wählen. Exportiertes MIDI enthält einen Pattern-Durchlauf; die Wiederholungswahl betrifft WAV. Kein ungetestetes DAW-Drag-and-drop.

## WAV und Kick

Exportformat wählen: PCM24 oder Float32, 44,1/48/96 kHz, mono oder identisches Dual-Mono-Stereo. `repeats` wählt 1–16 Durchläufe. `EXPORT TAIL` fügt einen einsekündigen Release-/Filterausklang hinzu. Ohne Tail endet die Datei genau an der Pattern-Grenze und kann bei ungünstiger Phase einen Schnitt enthalten. Für Loop-Schnitte gegebenenfalls in Cubase überblenden. WAV-Rendering startet unabhängig vom Host bei PPQ 0 und verwendet eine separate Instanz derselben Synthese-Engine. Der Export verwendet den sichtbaren BPM-Wert, nicht einen versteckten DAW-Tempoverlauf. Exporte laufen außerhalb des Audiothreads.

Das getrennte Kick-Modul wird über KICK 1–10 und KICK EDIT bedient: Tune, Pitch Start, Pitch Decay, Amp Decay, Click, Drive und Output. Für One-Shots `Kick WAV 24 / 48k` und EXPORT wählen. Kick-Parameter werden mit dem Plugin-Zustand gespeichert. Das Modul ist ein Offline-Synthesizer; es besitzt keinen zusätzlichen Echtzeit-MIDI-Ausgang oder Live-Kick-Layer.

Die beiliegenden Basslines wurden mit 145 BPM, zwei Durchläufen, einer Sekunde Tail, 48 kHz/PCM24/mono erzeugt. Die zehn Kicks sind 48 kHz/PCM24/mono, je zwei Sekunden. Eigene Klänge, keine fremden Samples.

Alle Factory-Dateien erneut erzeugen:

```powershell
.\build\Release\psy303_render.exe LibraryNeu
```

`Library/Presets/*.psyfactory` sind lesbare Kern-/Bibliotheks-Presets mit allen Klang- und Patternwerten. Im Plugin sind diese 32 Factory-Presets eingebaut. Das LOAD-Feld lädt das umfassendere binäre JUCE-Userformat `.psy303`, nicht `.psyfactory`.

## Architektur und DSP

`Source/Core.h`: JUCE-unabhängiger Kern, doppelte interne Rechengenauigkeit, Float32-Ausgabe. PolyBLEP-Saw/Square, phasenkontinuierlich. Dreistufiges trapezoidales Tiefpassmodell mit impliziter tanh-Rückkopplung und begrenzter Newton-Iteration, nominal 18 dB/Oktave. Eigenes Modell ohne Messdatenabgleich. Filterhüllkurve exponentiell mit nichtlinearer Intensität, separate Accent-Abklingkurve, geglättete Lautstärke und Tonhöhe, DC-Sperre und begrenzender Ausgang.

1x/2x/4x DSP-Oversampling mit 33-Tap-Halbband-Decimatoren; zwei Stufen bei 4x. Konstante, dem Host gemeldete Latenz von 12 Samples. Parameter, Wellenform, Pitch Bend und Modwheel werden geglättet; Oversampling-Wechsel werden überblendet. Float32-Hostverarbeitung, interne Double-DSP-Berechnungen; kein separater Float64-Hostcallback. Unterstützte Sampleraten 44,1–192 kHz.

`Source/Plugin.*`: APVTS, feste MIDI-Key-Queue, atomare Step-/Reglerwerte, samplegenaue MIDI-Ereignisse, Last-Note-Priority und Hostposition. Der Audiocallback hat keine Dateizugriffe, GUI-Aufrufe oder von uns eingebauten blockierenden Locks. Kern und Callback werden separat getestet. Hostautomation wird durch JUCE in Blockauflösung übernommen und intern geglättet; samplegenaue VST3-Automationskurven werden nicht gesondert rekonstruiert.

`Source/Editor.*`: eigene JUCE-Vektorgrafik, GUI-Timer liest nur Anzeigezustand; er erzeugt kein Sequencer-Timing. Snapshots in `docs/interface-*.png`. Die kleine abgeschnittene Vorlage erlaubt keine pixelgenaue Rekonstruktion der verdeckten Bereiche.

Sequencer-Positionen werden aus absoluter PPQ abgeleitet. Host-Sprünge setzen Note-/Step-Zustände zurück. Das Pattern ist bei PPQ 0 verankert, nicht bei der Cubase-Loop-Grenze. Beim Start mitten im Takt setzt es an der entsprechenden Patternposition ein; eine Sprungnote retriggert. Ohne Host-PPQ wird der samplebasierte Freilauf verwendet. Negative PPQ vor Projektbeginn wird auf 0 begrenzt. Bei ungeraden Patternlängen läuft die absolute Swing-Paarung weiter; für identisch wiederholbare Swing-Loops gerade Längen verwenden.

## Tests erneut ausführen

CMake/CTest wie oben. Ohne JUCE für reine DSP-/Exporttests:

```powershell
cmake -S . -B corebuild -DPSY303_PLUGIN=OFF
cmake --build corebuild --config Release
ctest --test-dir corebuild -C Release --output-on-failure
```

Mit Python, NumPy und SciPy:

```powershell
python tools/analyse.py Library
.\build\Release\psy303_diagnostics.exe diagnostics
python tools/spectrum.py diagnostics
```

Steinbergs offiziellen Validator aus dessen SDK separat bauen und gegen das **Windows-Bundle** ausführen. Hier wurde nur das Linux-Bundle mit dem offiziellen Validator getestet. Windows-Skripte und GitHub-Workflow liegen bei, wurden hier jedoch nicht unter Windows ausgeführt.

## Vor Produktion manuell prüfen

Windows-MSVC-Build, Cubase-15-Scan, ASIO und echte Audioausgabe, Hostautomation und Projektspeicherung/-öffnung, Loop-/Tempoänderungen, externe Controller, Dateidialoge, native DPI-Änderungen und Undo in der realen Session. GUI-Snapshots prüfen Schärfe/Anordnung, ersetzen keinen Bedienungstest. Keine Klangidentität oder vollständige Feature-Identität mit einem fremden Original zugesichert. Detaillierte Grenzen und Messwerte: `docs/PRUEFBERICHT.md`.

## Lizenz

Eigener Projektcode: AGPL-3.0-only. JUCE 8 wird unter der mitgelieferten JUCE-Lizenz (AGPLv3 oder passende kommerzielle JUCE-Lizenz) verwendet. Die JUCE- und Drittanbieter-Hinweise in `ThirdParty/JUCE` bleiben erhalten. Vor einem kommerziellen Vertrieb sind die zutreffenden JUCE-Lizenzbedingungen zu beachten.
