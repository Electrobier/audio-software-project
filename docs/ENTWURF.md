# White 224 — Anforderungsanalyse und Architektur

Stand: 27.10.2026. Spezifikation des Auftraggebers ist die einzige Referenz. Keine Referenzbilder, Hardwaremessungen, Originalkoeffizienten oder Referenzimpulsantworten wurden geliefert.

## Fakten und Annahmen
Dokumentierte Sollwerte: 8 Reverbprogramme und Chorus A, 12 Faderparameter, zwei Parameterseiten, CALL/IMMED/SET, vier Register, Wet/Bypass, zwei Ansichten, 42 Presets in sechs Bänken. Alle empfohlenen BASS/MID/Frequenz/Predelay-Werte werden übernommen. GUI-Geometrie ist ohne Bilder eine eigenständige Annäherung. Keine Originalgrafiken oder fremden Markenlogos werden verwendet.

Annahmen: DEPTH-Grundwert 35; Enhancement und Optimization jeweils 4 und eingeschaltet. SHIFT ist rastend; SET scharfstellen, anschließend A–D speichern; sonst A–D abrufen. CALL und IMMED sind alternative Programmwechselmodi. Programme ändern keine zweite Parameterseite. Ein 70-s-Preset ist ein langer endlicher Nachhall, kein mathematisch unendlicher Freeze.

## Architektur
JUCE 8.0.6 / C++20 / CMake. AudioProcessorValueTreeState hält Hostparameter; DSP besitzt keine JUCE-Abhängigkeit. Parameter werden blockweise aus atomaren Werten gelesen und im DSP geglättet. Audio-Callback führt keine Dateioperationen, Locks oder Allokationen aus. Zwei vorab reservierte Reverbkerne ermöglichen einen 80-ms-Übergang beim Programmwechsel. Schnellere Wechsel werden bis zum Ende des Übergangs zusammengefasst.

Signalweg: Eingangsverstärkung → vier Butterworth-Biquads (8. Ordnung) → stückweise exponentabhängige Quantisierung → Predelay mit 30-ms-Tap-Überblendung → Stereo-Early-Reflections und Allpass-Diffusion → zusätzlicher Depth-Delay mit Tap-Überblendung → achtliniges Hadamard-FDN → Mix/Output. Dry-Pfad bleibt unabhängig von Input-Gain und Bandbreite. Bypass überblendet auf unverändertes Dry und schließt den Tankeingang; gespeicherte Energie läuft weiter.

FDN: drei Frequenzbereiche mit getrennten Tiefen-/Mittenverlusten, stärkerer Höhenbedämpfung, acht langsamen Modulatoren und programmabhängigen Delayzeiten, Diffusion, Reflexionen und Stereoabgriffen. RT60-Rückkopplung: g=10^(-3*delaySeconds/RT60). Ein pegelgesteuerter Faktor reduziert bei Eingangssignal sanft die Sollhallzeit. Kein Anspruch auf originale Lexicon-Algorithmen. Chorus verwendet vier modulierte Delaystimmen ohne Reverb-Tank.

## DSP-Module
- Primitives: fraktionale Ringpuffer, Biquad-Tiefpass, Allpass, Glättung.
- VintageConverter: vier logarithmisch gestaffelte Quantisierungsbereiche; 12-Bit-Schrittweite pro Bereich; exponentbasierte Erweiterung oberhalb Nominalpegel. Deterministisch, kein künstlicher Zufallsrauschgenerator. Keine historische Transferfunktion.
- ReverbTank: acht Delayleitungen mit normierter Hadamard-Matrix und frequenzabhängigen Rückkopplungen.
- ReverbEngine: Stereo-Ein-/Ausgabe, Pegel, Programmüberblendung, Wet/Bypass, vierstimmiger Chorus.
- FactoryPresets: 9 dokumentierte Programme, 33 explizit rekonstruierte Klangvorschläge.
- PluginProcessor: Buslayout, APVTS, versionierter Zustand und Register.
- PluginEditor: gemeinsame Komponenten für Classic und schattierte Materialansicht; Parameterattachments, Browser, Dateioperationen ausschließlich im Message-Thread.

## GUI-Komponenten
Gehäuse mit eigenem White224-Schriftzug, rotes Segmentdisplay, L/R-Pegel, 8 Programmtasten mit LEDs, IMMED/SET/CALL/SHIFT/A/B/C/D, 6 Fader mit zwei Belegungen, 6 Momenttasten, Wet/Bypass, Presetbrowser, Save/Load und Designumschaltung. Fester Entwurfsraum 900×780 wird proportional skaliert. Fenstergröße und Design werden gespeichert.

## Umsetzung und Abnahme
1. Anforderungen/Umgebung protokollieren.
2. Unabhängigen DSP bauen, Impulsantwort und Stabilität testen.
3. JUCE-Prozessor und Zustands-/Presetvalidierung bauen.
4. GUI und Bedienlogik integrieren.
5. Automatisierte Integrations- und DSP-Tests, Messberichte und Paketprüfung.
6. Windows-Release über beiliegendes PowerShell-Skript oder CI; Cubase-Test separat manuell.

Ohne Original-IR sind 2,2 %/8 % Originaltreue und Korrelationsreferenzen keine nachgewiesenen Eigenschaften. Ohne Windows/Cubase dürfen deren Abnahmekriterien nicht als bestanden gelten. Hörprüfung und Abgleich anhand der fehlenden Bilder bleiben erforderlich.
