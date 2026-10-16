# IRON 2500 · tatsächlicher Teststand

Datum: 16. Oktober 2026. Version: 0.1.0. Umgebung: Debian 13 x64, GCC 14.2, CMake/Ninja, JUCE 8.0.6 (Commit 51a8a6d7aeae7326956d747737ccf1575e61e209), Release. Kein Windows und kein Cubase verfügbar.

## Ergebnis

| Prüfung | Tatsächliches Ergebnis |
|---|---|
| Portabler C++20-DSP-Build | Bestanden |
| Vollständiger nativer JUCE-Build | Bestanden: Linux-VST3 und Standalone kompiliert |
| CTest DSP | Bestanden: 1.115.868 Assertions |
| CTest State | Bestanden: Zustand, Dateipreset, A/B, Float-Stille und Double-Prozessorproben |
| CTest Gesamtlauf | 2/2 Tests bestanden, 1,65 s im abschließenden Lauf; keine Leistungszusage |
| VST3-Testhost (JUCE) | Discovery, Instanziierung, 1000 Audioblöcke à 128 Samples, State-Roundtrip bestanden; gemeldete Latenz 8 Samples |
| GUI | Kompilierten Editor in virtueller X-Anzeige erzeugt und als 1200×660 PNG gerendert; visuell geprüft und Beschriftungsprobleme korrigiert |
| Windows-Cross-Build | Fehlgeschlagen: JUCE 8 verweigert MinGW ausdrücklich (`MinGW is not supported. Please use an alternative compiler.`) |
| MSVC/Windows/Cubase | Nicht durchgeführt; kein Windows-Binary erzeugt |
| Steinberg-Validator/pluginval | Nicht durchgeführt; der eigene JUCE-Testhost ist kein offizieller Validator |

## DSP-Abdeckung

Kennlinien für 1,5/2/3/4/6/10/1000:1, Hard/Medium/Soft Knee, Monotonie und Kniegrenzen. Attack: alle sieben Zeiten bei allen sechs Abtastraten. Release: alle sechs Zeiten bei allen sechs Abtastraten. New, Old und F Back werden stationär auf die Zielratio geprüft. Stereo-Link wird mit 0/35/100 % geprüft. Bypass, sauberes Wet und Mix=0 bestehen Nulltests mit acht Samples Verzögerung und einer Toleranz von 1e−12 im Double-DSP.

Weitere Proben: Ausgangsverstärkung, Analog-Transfer, Glättung, alle Type/Shape/Thrust-Kombinationen bei extrem kurzer Attack/hoher Ratio, Sample-Rate-Wechsel, Rauschen, Sinus, Pegelsprünge, Stereo-Asymmetrie, Impulse und ungültige Eingangswerte (NaN/Inf). Unterschiedliche Gruppierung desselben Eingangssignals in 1/7/64/511/2048/5561 Samples erzeugt identische DSP-Ausgaben. Die Prozessorintegration prüft zusätzlich Mono und Stereo, Double sowie Blockgrößen 1/7/64/511/2048 bei 44,1/48/88,2/96/176,4/192 kHz.

Die Kennlinien-/Zeittests verwenden kontrollierte Detektorsignale. Es fand kein Hörvergleich mit kommerziellen Referenzplugins und keine Bewertung anhand echter Musikaufnahmen statt.

## Zeitmessungen

Attack/Release sind als 63,2-%-/36,8-%-Zeitkonstanten definiert, nicht als 10–90-%-Zeiten. Die Engine wird mit einem Amplitudensprung angeregt; ihre tatsächliche GR wird nach der aufgerundeten Sampleanzahl ausgelesen und mit der analytischen Zielkurve verglichen (Toleranz 1e−7 dB). Die vollständigen 42 Attack- und 36 Release-Messpunkte stehen in `Messprotokolle/dsp.txt`.

Beispiel: Bei 192 kHz und Release=1 s verbleiben nach einer Sekunde von 15 dB Anfangs-GR etwa 5,51819 dB. Auch Release=2 s liefert nach zwei Sekunden denselben relativen Restwert. Sehr kurze Attackzeiten werden auf ganze Samples aufgerundet.

## Detektor-Frequenzproben

Identische Sinusamplitude (0,8), Threshold −20 dB, Ratio 2:1, Hard Knee:

| Signal | Normal | Loud |
|---|---:|---:|
| 60 Hz | 8,90910 dB GR | 4,42170 dB GR |
| 6 kHz | 8,99508 dB GR | 10,64450 dB GR |

Zusätzlich werden HP/LP-Unterschied und die längere Rückstellung mit Variable Release geprüft. Diese Messungen belegen funktionale Unterschiede; sie belegen keine Übereinstimmung mit einer API-Thrust-Schaltung.

## GUI-Abnahme

Die Vorschau stammt direkt aus `IronEditor::createComponentSnapshot` der kompilierten Anwendung, ohne Bildgenerierung und ohne nachträgliche Retusche. Geprüft wurden lesbare Beschriftungen, Anordnung, Parameterwerte, Meterflächen und Überlappungen. Die Vorschau zeigt das Clean-Preset. Alle sichtbaren Parameterregler verwenden APVTS-Attachments. Während des Renderns meldete Fontconfig einen nicht beschreibbaren Cache; das Rendern selbst war erfolgreich. Eine komplette automatisierte Maus-/Tastaturprüfung sowie reale Windows-DPI-/Monitorwechseltests sind offen. Ein Referenzbild zum Vergleich war nicht vorhanden.

## Reproduktion

Das Paket enthält sämtliche verwendeten Projekt- und JUCE-Quellen. Windows: `scripts/build-windows.ps1 -Configuration Release`. Optional liegt eine noch nicht ausgeführte GitHub-Actions-Konfiguration für Windows 2022 bei. Sie ist kein bereits erfolgreicher Windows-Build.

Die Build- und Testprotokolle liegen in `Messprotokolle/`. Für GUI/Host unter Linux: eine X-Anzeige bereitstellen und `iron_state_tests --preview /abs/pfad.png` bzw. `iron_state_tests --host /abs/IRON\ 2500.vst3` ausführen. Windows-Installationsanleitung und Parameterdetails: `Handbuch.md`. Offene Abnahme: `Abweichungen.md`.
