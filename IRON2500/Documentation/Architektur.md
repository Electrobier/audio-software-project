# Architektur und Messdefinitionen

Der AudioProcessor besitzt genau eine CompressorEngine. Host-Parameter werden im Konstruktor als APVTS-Atomics gecacht und pro Block gelesen. prepareToPlay setzt alle DSP-Zustände zurück und berechnet die Oversampling-Taps; der Callback besitzt weder dynamische Container noch Datei-/GUI-Zugriffe. Der Editor liest sechs einzelne Meter-Atomics mit 30 Hz. Presetdateien und A/B werden außerhalb der Audioverarbeitung über APVTS verwaltet. Sample-Rate und Blocklänge werden nicht über globale Variablen geteilt.

Die Engine hält pro Kanal zwei Tilt-Filter, zwei Shape-Filter, den RMS-/Programmverlaufszustand, GR-Hüllkurve, Feedback-Sample, zwei Acht-Sample-Delays und eine Saturator-Instanz. Alle Buffer sind std::array. Die modulare Trennung erfolgt in Parameters, Gain-Computer-Funktionen, OnePole, Delay, Saturator und CompressorEngine; nicht jede im Masterprompt vorgeschlagene Klasse ist als eigene Datei ausgeführt.

## Kennlinie

Für Pegel L, Threshold T, Ratio R, Knee-Breite W:

- Außerhalb des Knies: GR = max(0,L−T) · (1−1/R).
- Innerhalb des Knies: GR = (1−1/R) · (L−T+W/2)² / (2W).
- Der Übergang besitzt eine stetige erste Ableitung.

Die Attack-/Release-Hüllkurve läuft im dB-GR-Bereich. Für eine Zeitkonstante tau ist alpha = 1−exp(−1/(tau·SampleRate)). Ein Pegelsprung erreicht nach tau ca. 63,2 % seiner Ziel-GR; nach einer Release-Zeit verbleiben ca. 36,8 %. Das ist ausdrücklich keine 10–90-%-Zeitdefinition. Messungen verwenden eine konstante Detektoramplitude, Hard Knee, New und Variable=0. Sehr kurze Zeiten werden auf ganze Samples aufgerundet; die theoretische Referenz wird am tatsächlichen Samplezeitpunkt berechnet.

Old führt den bereits komprimierten Pegel zurück und multipliziert die Feedforward-Kennlinie mit R; die stationäre Gesamtsteigung bleibt dadurch bei 1/R. Die Relaxationsrate wird durch 1+alpha·(R−1) geteilt, um die diskrete Rückkopplung bei hohen Ratios zu stabilisieren. F Back ergänzt einen RMS-Leistungsdetektor und langsamere Zeitkonstanten. Feedback wird vor Analog/Output abgenommen, damit Makeup und Sättigung nicht unkontrolliert in den Regelkreis eingehen.

Die Filter sind gematchte Einpol-Tiefpässe mit a = 1−exp(−2pi·fc/SampleRate). Tilt Medium: x−0,55·LP180+0,25·(x−LP2500). Tilt Loud: x−0,85·LP180+0,65·(x−LP2500). Diese Gewichte sind eigene Designentscheidungen, keine Rückentwicklung einer proprietären Schaltung.

Der Saturator interpoliert mit Zero-Stuffing auf 2×, filtert mit einem Blackman-gefensterten 17-Tap-Halbbandsinc, verarbeitet tanh(0,8x)/0,8 und filtert vor der Dezimation nochmals. Die Gesamtlaufzeit beträgt acht Basissamples. Analog Off verwendet einen reinen Delay-Pfad; Analog schaltet geglättet zwischen beiden ausgerichteten Pfaden um. Der Dry- und Bypass-Pfad besitzt ebenfalls acht Samples Verzögerung.

## Qualitätssicherung

Tests verwenden echte Engine-Ausgaben und Zustände; nicht nur UI-Werte. Nulltests berücksichtigen die acht Samples Latenz. Audio-Detektortests verwenden Sinusfrequenzen mit identischer Eingangsamplitude. Blocktests vergleichen identische Eingangsdaten bei verschiedener Gruppierung. Der JUCE-Integrationstest prüft Projektzustand, Disk-Preset, A/B, Prozessor-Stille sowie optional das Laden des gebauten VST3 und ein echtes Editor-Rendering. Ein Windows-Compilerlauf und Cubase sind gesonderte Abnahmeschritte.
