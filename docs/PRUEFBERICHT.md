# Prüfbericht — 27.10.2026

Version 0.1.0. Tatsächlich verwendete Umgebung: Linux x86_64, GCC 14.2.0, CMake 4.4.4, JUCE 8.0.6. JUCE-Quellarchiv wird im CMake-Projekt per SHA256 fixiert. Native System-Entwicklungsbibliotheken wurden in einem lokalen Sysroot bereitgestellt, nicht systemweit installiert.

## Erledigt

- Eigenständiger C++20-DSP-Kern: Release kompiliert.
- JUCE Shared Code, Linux-x64-VST3 und Linux-Standalone: erfolgreich kompiliert. Standalone blieb unter Xvfb fünf Sekunden bis zum kontrollierten Timeout aktiv. Die Umgebung hat kein ALSA-Audiogerät; kein Audiohardware-Test.
- DSP-Prüflauf: 19.091.056 Einzelprüfungen bestanden (einschließlich pro Sample geprüfter Endlichkeit). Die hohe Zahl entsteht durch Sampleprüfungen und bezeichnet nicht ebenso viele unterschiedliche Testszenarien.
- Acht Reverb-Programme und Chorus liefern neun verschiedene nichtstumme Impulsantworten.
- Alle zwölf Fader beeinflussen den vorgesehenen Audiopfad. Die fünf in Chorus ausgeschlossenen Primärparameter verändern dessen Ausgabe nicht.
- 44,1/48/88,2/96 kHz, Blockgrößen 1/17/64/257/1024: schnelle Programmwechsel, Extremwerte und Automation ohne NaN/Inf in den Prüfläufen. Statische Impulsantwort bei Blöcken 1 und 512 invariant.
- Instrumentierte `new`/`new[]`-Aufrufe im DSP-Callback: 0. Keine Locks oder Dateioperationen im DSP-Code. Speicher vorab reserviert; kein Beweis für alle denkbaren Hostpfade.
- Vorverzögerung und DEPTH verwenden 30-ms-Überblendungen zwischen festen Abgriffen; ein zusätzlicher Sinus-Test prüft den Übergang bei großen Delaysprüngen.
- Maximales Predelay: gemessene zusätzliche Onset-Verzögerung 12.240 Samples bei 48 kHz (255 ms, Toleranz 2 Samples).
- Wet-Override unabhängig vom gespeicherten Mix; stabil aktivierter Bypass gibt unverändertes Dry aus. Eingangssilence ergibt Nullsignal. NaN-/Inf-Eingänge werden abgefangen.
- Achtpoliger Eingangsfilter: an allen zwölf Kombinationen aus vier Sampleraten und 3/8,1/20 kHz am effektiven Cutoff ungefähr −3,0103 dB. Bei 44,1 kHz ist 20 kHz auf 19,845 kHz begrenzt.
- Factory-Presets: 42 gültige Datensätze, Bankgrößen 9/7/5/11/5/5.
- APVTS-/Binärzustand, Register, User-Preset-Datei und Wiederherstellung getestet. Numerische Vergleiche beim logarithmischen Normalisieren erlauben maximal 2 ppm relative bzw. 0,00001 absolute Floatabweichung. Beschädigte Binärdaten, NaN-/Out-of-range-Werte, unbekannte/doppelte Parameter, Zukunftsversionen und defektes XML ändern den vorherigen Zustand nicht.
- GUI-Integration: alle acht Programmtasten, CALL/IMMED, SHIFT/Chorus, Enhancement-/Decay-Schalter, SET/Register D, SHIFT-Faderattachment, Wet-/Bypassattachments, beide Designs und min./max. Fenstergrößen automatisiert geprüft. GUI-Snapshots unter Xvfb erzeugt und visuell geprüft. Parameteranzeigetasten sind implementiert; Halten/Loslassen mit realer Maus bleibt Teil des manuellen Bedienchecks.
- ASan+UBSan-DSP-Prüflauf bestanden. LeakSanitizer ist in dieser Sandbox wegen ptrace nicht ausführbar und wurde für den erfolgreichen Lauf deaktiviert. Kein separater Leak-Nachweis.

## Akustische Messungen

Methodik: Impulsamplitude 0,5 auf beiden Kanälen, 48 kHz, Wet 100 %, Input/Output 0 dB, Enhancement und Optimization aus; je Reverb 12 Sekunden, 70-s-Sonderlauf 90 Sekunden. Vierpoliger Butterworth-Oktavbandpass, Schroeder-Rückwärtsintegration und T20-Regression zwischen −5 und −25 dB; auf RT60 extrapoliert. Details einschließlich Bestimmtheitsmaß in `measurements/metrics.json`, Tabelle in `octave_rt60.csv`.

Die 70-s-Einstellung ergibt bei 125 Hz rund 69,99 s und bei 1 kHz rund 69,26 s. Das sind extrapolierte T20-Werte, keine Übereinstimmung mit Originalhardware. Untere und mittlere Oktaven der acht Programme liegen in der Größenordnung der vorgegebenen Bass-/Mid-Werte; Frequenzbereiche überlappen, daher ist z. B. 1 kHz kein isolierter MID-Kanal. Tabellenwerte sind Soll-/Modellkontrolle, keine Behauptung von 2,2 % Originalabweichung.

Die gemessene **Wellenformkorrelation** der Reverb-Tails liegt hier etwa zwischen −0,112 und +0,144. Sie ist ausdrücklich nicht die in der Aufgabenstellung erwähnte **Hüllkurvenkorrelation**. Für Chorus ist der gewählte späte Messabschnitt stumm; dort gibt es keinen gültigen Korrelationswert.

Im letzten Release-Prüflauf benötigten die drei CPU-Messungen für drei Sekunden Stereo-Audio rund 0,056–0,058 Sekunden Wandzeit (1,86–1,92 % eines Echtzeitbudgets). Die größten dabei beobachteten Callbacks lagen zwischen 0,277 und 0,815 ms. Dies ist kein vollständiger Belastungstest.

CPU-Messungen in `cpu.csv` sind Wandzeitmessungen einer geteilten Cloudumgebung mit CPU-Quota und zeitweise parallel laufendem Compiler. In früheren Läufen wurden durch Scheduling auch wesentlich größere Spitzen beobachtet; diese können Audio-Blockfristen überschreiten. Daraus folgt weder eine garantierte Echtzeitlatenz noch eine verifizierte Windows-CPU-Belastung. DSP-Verarbeitung benötigt konstante, programmabhängige Arbeit; zwei Kerne werden nur beim 80-ms-Programmübergang berechnet. DAW-Lasttests auf dem Zielrechner bleiben notwendig.

## Noch nicht abgenommen

- Windows-11/MSVC-Build, Windows-Standalone-Start und Windows-VST3-Scan: zunächst ausstehend; nur ein zusätzlich tatsächlich erzeugter `WINDOWS-BUILD.txt`-Beleg dokumentiert einen späteren Windows-CI-Lauf.
- Cubase 15 Pro: weder Zugriff noch Test; keine Cubase-Zertifizierung.
- Hörprüfung, reale Audiogeräte, MIDI-/Steuergeräteintegration, 4K/Windows-DPI und praktische Echtzeitmessungen unter DAW-Last.
- Referenzbilder, Originalimpulsantworten und Originalmessungen fehlen. Pixelgenauer/fotorealistischer Abgleich und Originalklangvergleich sind daher offen.
- Kein unendliches Freeze, keine bitgenaue historische Wandlertransferfunktion. Modulation und Companding sind dokumentierte Annäherungen.

Die vollständige professionelle Endabnahme nach den zwölf Punkten des Auftrags ist damit **noch nicht erreicht**. Die vorhandenen Implementierungen und automatisierten Ergebnisse sind konkret nutzbar und prüfbar; fehlende Zielsystemtests werden nicht als bestanden behandelt.
