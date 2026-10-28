# Prüfbericht – 28. Oktober 2026

## Ergebnis und Geltungsbereich

Linux x86_64, GCC 14.2, C++20, CMake 4.4, JUCE 8.0.6 Commit `51a8a6d7aeae7326956d747737ccf1575e61e209`. Release-VST3 und Standalone wurden tatsächlich kompiliert. Das VST3-Bundle wurde von Steinbergs offiziellem Validator geladen und geprüft. SDK-Stand: Commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` samt Submodulen. **47 Tests bestanden, 0 fehlgeschlagen.** Die Warnung zu gleichzeitig vorhandener moduleinfo.json und IPluginCompatibility ist eine Prioritätshinweis-Meldung des Validators, kein fehlgeschlagener Test.

Windows/MSVC und Cubase 15 Pro standen nicht zur Verfügung. Ein tatsächlicher MinGW-Cross-Build-Versuch scheiterte am expliziten JUCE-Compilerverbot. Es gibt deshalb kein fertiges Windows-Binary in dieser Lieferung. Das Source-Projekt enthält ein Windows-MSVC-Buildskript, Installationsskript und eine noch nicht unter Windows ausgeführte CI-Konfiguration. Das ist kein Nachweis für einen erfolgreichen Windows-Build.

## Automatisch ausgeführt

- Kern-DSP: 440-Hz-Oszillator mit beiden Wellenformen bei 44,1/48/96/192 kHz; Einschwingen/Release; gradueller Slide und Zielton über zwei Oktaven; Accent-Energievergleich.
- Factory-Presets: 32 verschiedene Klang-/Pattern-Signaturen, hörbare Energie, endliche Samples, kein Überlauf, niedriger DC-Mittelwert und begrenzte benachbarte Sampledifferenzen.
- Sequencer: absolute PPQ-Position, 10.000 Swing-Positionen, gleiche Step-Grenzen bei Blockgrößen 32/64/128/256/512/1024, Seek-/Loop-Zuordnung. Ein eigener GUI-Timer ist für diese Tests nicht beteiligt.
- Filter: maximale Resonanz und Drive bei variierender Cutoff-Frequenz, sechs Sampleraten von 44,1 bis 192 kHz und allen drei Oversampling-Faktoren. Keine NaN/Inf-Werte oder Samples außerhalb des Begrenzers im Test.
- Persistenz: Kern-Preset-Roundtrip mit identischem erneut gerendertem Audio; APVTS-Roundtrip einschließlich Step außerhalb der ersten Seite und CC-Learn-Wert.
- Plugin-MIDI: Note-On an Sample 77 mit Stille davor, Last-Note-Priority, Rückkehr zur vorher gehaltenen Note, All Notes Off, MIDI Learn ohne geöffnetes Fenster.
- Plugin-Host: Step-Zuordnung nach Hoststart, Seek und Loop für sechs Buffergrößen.
- Audio-Callback: globaler `operator new`/`new[]`-Zähler während 100 vorbereiteter Callbacks mit 1024 Samples und MIDI-Note-On: null Aufrufe. Dieser Test erfasst C++-new-Allokationen im geprüften Pfad; er beweist nicht pauschal die Abwesenheit jedes denkbaren malloc- oder Betriebssystemaufrufs.
- WAV: C++-Tests für Dateigröße bei PCM24/Float32 und mono/stereo; unabhängiger Python/Scipy-Import aller 42 mitgelieferten WAVs.
- MIDI: C++-Headerprüfung und unabhängiges Python-SMF-Parsing aller 32 Dateien einschließlich Chunklängen, Delta-Zeiten, Notenpaaren, Velocity-Wertebereichen und Slide-/Accent-CCs.
- Kick: alle zehn Presets erzeugen endliche, begrenzte und nichtstumme Signale.
- GUI: echte JUCE-Komponenten-Snapshots bei 1120×790, 1400×987, 1680×1185 und 2240×1580. Sichtprüfung der Grundoberfläche. Keine Prüfung physischer Monitore oder nativer Windows-DPI-Wechsel.

Die 5.386.188 einzelnen Kern-Prüfbedingungen zählen zum großen Teil Sample-Stabilitätsprüfungen; dies sind keine Millionen unabhängiger Testszenarien. CTest enthält zwei Testsuiten. Rohprotokolle: `core-test.txt`, `plugin-test.txt`, `ctest.txt`, `vst3-validator.txt`, `linux-build.log`.

## Offline-Audiodaten

32 Bassline-Exporte: jeweils 145 BPM, zwei Patterndurchläufe plus eine Sekunde Ausklang, PCM24 mono 48 kHz. 10 Kick-Exporte: je zwei Sekunden, PCM24 mono 48 kHz. Unabhängige Analyse: maximaler absoluter Peak **0,699711**, größter absoluter DC-Mittelwert **0,00032954**. Per-Datei-Werte für RMS, Peak, DC, maximale Sampledifferenz und Hochfrequenzenergie stehen in `audio-analysis.json`.

Ein benachbarter Sampledifferenz-Grenzwert ist ein grober technischer Kontinuitätstest. Er ersetzt kein geschultes Abhören. Die Maschine hatte keine geprüfte Audiohardware. Klangcharakter, musikalische Qualität und Verhalten in einer realen Produktion müssen abgehört werden.

## Aliasing und Spektrum

Separater kohärenter 5-kHz-Test bei 48 kHz. Die Energie außerhalb der erwarteten harmonischen FFT-Bins wird relativ zur gesamten Wechselenergie ausgewertet:

| Testsignal | Relative Energie in Alias-Bins |
|---|---:|
| Sägezahn ohne BLEP | 0,131849 |
| Sägezahn PolyBLEP | 0,005975 |
| Rechteck ohne BLEP | 0,097056 |
| Rechteck PolyBLEP | 0,005815 |
| Getriebenes Filter, 1x | 0,020832 |
| Getriebenes Filter, 2x | 0,004824 |
| Getriebenes Filter, 4x | 0,001517 |

Damit reduziert BLEP die gemessene Alias-Bin-Energie in diesem gezielten Test um ungefähr 13,4 dB beim Sägezahn und 12,2 dB beim Rechteck. Der 4x-Filterpfad liegt in diesem Test etwa 11,4 dB unter 1x. Vollständig aliasfrei ist das Instrument nicht. Ausgangs-tanh, endliche FIR-Filter und extreme Cutoff-/Resonanz-/Tonhöhenkombinationen bleiben mögliche Aliasquellen. Das Spektrum der musikalischen WAVs allein würde Aliasing nicht eindeutig von gewünschten Obertönen unterscheiden.

## Noch manuell zu prüfen

Windows-MSVC-Build und Ausführung, Cubase-15-Instrumentenscan, echte MIDI-/ASIO-Hardware, Hostautomationsaufzeichnung, Projekt-Neuladen, native Dateidialoge, Undo-/Redo-Bedienung, Controller- und Fenster-Lebenszyklus, Windows-DPI-Änderungen, Last unter langen Sessions und praxisgerechtes Abhören. Der offizielle Validator läuft hier am Linux-Build; er validiert keine Windows-ABI und garantiert keine Cubase-Kompatibilität.

## Bewusste Grenzen

Eigenes 303-inspiriertes Modell, keine validierte Hardware-Emulation. Vorlage teilweise verdeckt/abgeschnitten, daher keine exakte grafische Replik. Blockweise Hostautomation mit DSP-Glättung; keine eigene Rekonstruktion samplegenauer VST3-Parameterqueues. Float32-Host-Audio, interne Double-Berechnung. MIDI-Kanäle zusammengeführt, kein Sustain und kein SysEx. Dual-Mono-Stereo, keine Stereo-Effekte. Kick ist ein separat prüfbares Offline-Exportmodul, kein zusätzlicher Echtzeit-Layer. MIDI-Drag-and-drop nicht implementiert; vollständiger Dateidialog-Export vorhanden. Factory- und User-Preset-Dateiformate sind getrennt und dokumentiert. Ungerade Patternlängen mit Swing können über zwei Durchläufe unterschiedliche Akzentuierung der Paarung ergeben. Export ohne Ausklang kann an der Endgrenze einen Schnitt besitzen.
