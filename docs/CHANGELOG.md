# Änderungsprotokoll

## 0.1.0 — 27.10.2026

- Spezifikation analysiert; fehlende Referenzbilder/-IR und historische Unbekannte dokumentiert.
- C++20-DSP-Grundgerüst unabhängig von JUCE kompiliert.
- Achtliniges Hadamard-FDN, frequenzabhängige Verluste, Stereo-Reflexionen, Allpässe, Depth, modulierte Delayzeiten, vierstimmigen Chorus integriert.
- Programmwechsel auf zwei vorreservierten Kernen überblendet. Fader, Wet/Bypass, Modulations- und Feedbackübergänge geglättet.
- Diskrete Delay-Grundlängen auf unterschiedliche Primzahllängen gerundet; Modulation nutzt fraktionale Interpolation.
- APVTS, 17 Hostparameter, Mono→Stereo, Stereo→Stereo, Host-/interner Bypass implementiert.
- Vier Register, versionierte XML-Presets, strikte numerische Validierung und atomaren Dateiersatz ergänzt.
- 42 Werkspresets und zwei gemeinsame vektorbasierte GUI-Ansichten implementiert.
- DSP-, Zustands- und GUI-Prüfungen, Oktavband-/Filtermessung, Windows-Paketierungsworkflow ergänzt.

- Abschließende Delayprüfung: Predelay/Depth mit 30-ms-Tap-Crossfades gegen große Lesekopfsprünge abgesichert, Steuerwerte sampleweise übernommen; Filterkoeffizienten bleiben auf 32-Sample-Steuerrate.
- Defektes XML, unbekannte/doppelte Parameter, Bereichsüberschreitungen und künftige Presetversionen zusätzlich getestet.
