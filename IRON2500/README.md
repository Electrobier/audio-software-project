# IRON 2500

Ein eigenständiger C++20/JUCE-Stereo-Bus-Kompressor mit echter Audioverarbeitung, skalierbarer Rack-GUI, 16 automatisierbaren Parametern und zwölf Werks-Presets. Windows x64 VST3 und Standalone; Entwicklungsstand 0.1.0. Eigenes DSP, keine Waves/API-Ressourcen oder proprietärer Quellcode.

## Windows selbst bauen

Voraussetzungen: Visual Studio 2022 mit **Desktopentwicklung mit C++**, Windows SDK, CMake ≥3.22 und Git. Das vollständige Quellpaket enthält JUCE 8.0.6. Wenn ThirdParty/JUCE fehlt, lädt die erste Konfiguration den festgelegten Commit aus dem Internet. Alternativ `-DJUCE_SOURCE_DIR=C:/Pfad/zu/JUCE` verwenden. Die getestete Version ist bewusst festgelegt; sie ist kein Versprechen der jeweils neuesten JUCE-Version.

Im Projektverzeichnis mit PowerShell:

```powershell
./scripts/build-windows.ps1 -Configuration Release
# Für Debug:
./scripts/build-windows.ps1 -Configuration Debug
```

Oder:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-release --parallel 2
ctest --preset windows-release
```

Ergebnis: `Build/windows/IRON2500_artefacts/Release/VST3/IRON 2500.vst3` sowie `Standalone/IRON 2500.exe`. Das komplette VST3-Bundle nach `C:\Program Files\Common Files\VST3\` kopieren. Praktische Cubase-Prüfung ist noch offen.

## Tests ohne JUCE

```sh
cmake -S . -B Build/dsp -DIRON_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build Build/dsp
ctest --test-dir Build/dsp --output-on-failure
```

Der DSP-Test benötigt nur einen C++20-Compiler. Der vollständige Build ergänzt State-/Preset-/Prozessor-Tests. Der Testprozess kann mit `--preview /absoluter/pfad.png` die tatsächliche JUCE-Oberfläche rendern und mit `--host /absoluter/pfad.vst3` ein gebautes VST3 laden. Unter Linux wird für GUI-/Hosttests eine X-Anzeige benötigt. Portabler DSP sowie Integration werden getrennt geprüft.

## Architektur

`Source/DSP/CompressorEngine.h`: speicherfeste samplebasierte Engine, Kennlinie, Feedback-Regelung, Sidechain-Filter, Stereo-Link, Glättung, 2× FIR-Sättigung und acht Samples Latenzausgleich.

`PluginProcessor`: Mono/Stereo, Float/Double, APVTS-Automation, Host-Bypass, Zustandsverwaltung und Presets. Parameter werden einmal pro Block atomar gelesen, keine Dateizugriffe oder Heap-Allokationen im Callback. `PluginEditor`: echte JUCE-Bedienelemente und Vektorgrafik; keine eingebrannte Hintergrund-Screenshot-GUI.

Bedienung und alle Parameter: [Handbuch](Documentation/Handbuch.md). Grenzen und Abnahme: [Abweichungen](Documentation/Abweichungen.md). Tatsächliche Ergebnisse: [Testbericht](Documentation/Testbericht.md).

## Lizenzen

Der eigene Quellcode ist AGPL-3.0-or-later. JUCE 8 wird separat unter seiner GNU AGPLv3-/kommerziellen Lizenz angeboten. Die Kombination wird hier unter **GNU AGPLv3** verteilt; die vollständigen zugehörigen Quellen einschließlich des verwendeten JUCE-Commits liegen im Quellpaket bei. Für eine proprietäre Weiterverteilung ist die JUCE-Lizenz gesondert zu klären. Drittanbieterhinweise stehen im JUCE-Quellverzeichnis und in `THIRD_PARTY.md`.
