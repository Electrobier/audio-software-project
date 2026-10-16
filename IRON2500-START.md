# IRON 2500 für Windows

1. [Vollständiges Quellpaket herunterladen](https://github.com/Electrobier/audio-software-project/raw/refs/heads/iron2500-delivery/IRON2500-Quellcode.zip).
2. ZIP unter Windows entpacken. Es enthält das Verzeichnis `IRON2500` mit JUCE, Quellcode, Tests und Dokumentation.
3. **Developer PowerShell for VS 2022** im entpackten Verzeichnis `IRON2500` öffnen.
4. Ausführen:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-release --parallel 2
ctest --preset windows-release
```

Voraussetzungen: Visual Studio 2022 mit **Desktopentwicklung mit C++** und Windows SDK sowie CMake ≥3.22. Das vollständige ZIP enthält JUCE 8.0.6; es braucht dafür keinen weiteren Download. Bei Verwendung des Git-Checkouts ohne ZIP wird JUCE bei der CMake-Konfiguration aus GitHub geladen; dafür wird zusätzlich Git benötigt.

Erwartetes Ergebnis nach erfolgreichem Windows-Build:

`Build/windows/IRON2500_artefacts/Release/VST3/IRON 2500.vst3`

Den gesamten `.vst3`-Ordner nach `C:\Program Files\Common Files\VST3\` kopieren und Cubase neu starten.

**Stand:** 0.1.0, Entwicklungsstand. Native Linux-VST3-/Standalone-Builds und eigene DSP-/Integrationstests bestanden. Windows/MSVC und Cubase sind noch nicht praktisch verifiziert. Das Paket enthält Quellcode, kein bereits gebautes Windows-Plugin. Weitere Grenzen und tatsächliche Testergebnisse stehen im Paket unter `Documentation`.
