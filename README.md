# White224 Vintage Reverb

Eigenständiger Vintage-Digitalhall in C++20/JUCE 8.0.6, VST3 und Standalone. Version 0.1.0, technische Vorabversion. Acht unterschiedliche FDN-Konfigurationen und ein vierstimmiger Chorus. Keine Firmware- oder Koeffizientenkopie eines Originalgeräts.

**Prüfstatus und verbleibende Abnahmen:** [docs/PRUEFBERICHT.md](docs/PRUEFBERICHT.md). Windows-/Cubase-Verifikation ist nur dann erfolgt, wenn ein entsprechender tatsächlicher Prüfbeleg mitgeliefert wird. Eine kompilierbare Windows-Anleitung ist kein Cubase-Test. Referenzbilder fehlen; die Vektoroberfläche ist eine Rekonstruktion aus der Textbeschreibung. „3D Look“ verwendet Verläufe, Licht und Schatten, keine fotorealistische Fotovorlage.

## Windows: fertiges Paket installieren

Nur bei einem **Windows-Binärpaket** (enthält EXE und VST3):

1. Das komplette ZIP herunterladen und vollständig entpacken.
2. Den gesamten Ordner `White224 Vintage Reverb.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren. Die innere Datei nicht aus dem Bundle herausnehmen. Windows kann Administratorrechte für diesen Ordner verlangen.
3. Cubase schließen, neu starten und im Plug-in-Manager prüfen. Auf einer Stereo-Effektspur als Send MIX=100 % oder 100% WET aktivieren.
4. Standalone: `White224 Vintage Reverb.exe` starten. Im Audio-/MIDI-Setup ein Windows-Audiogerät (WASAPI/DirectSound) wählen. ASIO wird in der VST3-Version durch Cubase bereitgestellt; der Standard-Standalone-Build enthält kein ASIO-SDK. Für Mikrofone/Live-Rückkopplung die Geräteführung sorgfältig einstellen.

Windows 11 x64, SSE2-fähige CPU, 44,1/48/88,2/96 kHz. Nicht signierte private Builds können durch SmartScreen geprüft werden. Nicht global Schutzfunktionen deaktivieren. Ein **Quellprojekt-ZIP** enthält keine fertige Windows-Anwendung.

## Windows selbst bauen

Benötigt: Visual Studio 2022 mit „Desktopentwicklung mit C++“, Windows SDK, CMake >=3.22 und Git. JUCE ist AGPLv3 oder kommerziell zu lizenzieren (THIRD_PARTY.md). CMake lädt JUCE 8.0.6 als per SHA256 geprüftes Quellarchiv. Im vollständigen Quellpaket liegt zusätzlich `JUCE-8.0.6.tar.gz` bei; für einen Build ohne erneuten JUCE-Download dieses Archiv entpacken und `-DJUCE_SOURCE_DIR=<Pfad zu JUCE-8.0.6>` an CMake oder `-JuceSource <Pfad>` an das PowerShell-Skript übergeben. Alternativ kann der entpackte JUCE-Ordner unter `third_party/JUCE` liegen.

PowerShell im entpackten Projektordner:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

`Bypass` gilt nur für diesen PowerShell-Prozess. Alternativ das Skript nach Prüfung mit der lokal erlaubten Ausführungsrichtlinie starten. Das Skript prüft Werkzeuge, baut Release, führt DSP-/State-Tests aus, prüft den VST3-Bundlepfad, startet Standalone für fünf Sekunden und erstellt danach `Build/Release.zip` und SHA256. Ein existierender Zielordner wird nicht überschrieben; bei erneutem Build `-Destination Build/Release-2` verwenden. Ein Prozessstart ist keine Hörprüfung.

Manuell:

```powershell
cmake -S . -B Build/msvc -G "Visual Studio 17 2022" -A x64
cmake --build Build/msvc --config Release --parallel 2
ctest --test-dir Build/msvc -C Release --output-on-failure
```

Buildprodukte liegen unter `Build/msvc/White224_artefacts/Release/`. Keine Systeminstallation während des Builds.

## DSP ohne JUCE testen

```sh
cmake -S . -B work/core -DWHITE224_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build work/core --parallel 2
ctest --test-dir work/core --output-on-failure
```

`white224_tests <Messordner>` schreibt Float32-Stereoimpulsantworten, Bandbreiten- und CPU-Berichte. `python tests/analyse_ir.py <Messordner>` benötigt NumPy/SciPy und berechnet Schroeder-T20/RT60 in Oktavbändern 125 Hz–8 kHz. Rohe IR-Dateien können zusammen etwa 80 MB belegen. `WHITE224_SANITIZE=ON` aktiviert auf GCC/Clang Address-/Undefined-Sanitizer für die DSP-Tests. GUI-Tests unter Linux benötigen eine X11-Sitzung, beispielsweise Xvfb; echte Audiohardware ist dafür nicht nötig.

## Bedienung

- **1–8:** Hallprogramme. **SHIFT + 1:** Chorus A (neunter Algorithmus).
- **CALL:** Programmwahl lädt die sechs empfohlenen Primärwerte. Sekundärwerte bleiben erhalten. **IMMED:** Algorithmus wechseln und alle Faderwerte erhalten.
- **SHIFT:** rastend zweite Faderseite aufrufen. **SHIFT + 7/8:** Enhancement/Optimization an-/ausschalten. Die Programm-LEDs 7/8 zeigen auf der SHIFT-Seite diese Zustände.
- **SET → A–D:** vollständigen Parametersatz speichern. **A–D ohne SET:** Register abrufen. Belegte Register haben eine LED. Noch leere Register ändern nichts.
- **VALUE halten:** den zugehörigen Wert im Segmentdisplay anzeigen; loslassen zeigt den Mittelwert von BASS und MID. In Chorus A sind diese Hallzeiten ohne klangliche Wirkung.
- **Fader:** ziehen; Shift/Strg/Command beim Beginn des Ziehens für Feineinstellung; Doppelklick auf Empfehlung des aktiven Programms zurücksetzen. Maus-Rad wird unterstützt.
- **100% WET:** nur Effekt ausgeben, ohne MIX zu verändern. **BYPASS:** auf unverändertes Dry überblenden; der Tankeingang wird geschlossen und die bestehende Energie intern weiter berechnet. Host-Bypass nutzt denselben Weg.
- **Presetbrowser:** sechs Factory-Bänke und vorhandene User-Presets. **SAVE/LOAD:** `.w224`-Dateien schreiben/lesen. Nach SAVE in das Standardverzeichnis erscheint das Preset im Browser. SAVE an einem anderen Ort ist ebenfalls möglich; LOAD öffnet es von dort.
- **3D LOOK / CLASSIC LOOK:** Darstellung umschalten. Fenster an der Ecke proportional skalieren (630×546 bis 1440×1248). Design, Parameterseite, Wechselmodus und Fenstergröße werden im Projekt gespeichert.

Alle zwölf Faderparameter, Algorithmus, die zwei Aktivierungszustände, Wet und Bypass sind Hostparameter. Cubase kann sie automatisieren. Änderungsgesten werden an den Host gemeldet; dessen Undo-Verhalten ist hostabhängig. SHIFT, CALL/IMMED und SET sind Bedienzustände, keine Audioparameter. Audiospeicher/Hallfahnen werden bei Projektladen nicht als Audiodaten serialisiert.

## Parameter

| Primärseite | Bereich | Wirkung |
|---|---|---|
| BASS | 0,6–70 s | Tieffrequente Soll-RT60 |
| MID | 0,6–70 s | Mittlere Soll-RT60 |
| CROSS OVER | 100–10.900 Hz | Bass-/Mittenübergang im Feedback |
| TREBLE DECAY | 100–10.900 Hz | Beginn stärkerer Höhenbedämpfung |
| DEPTH | 0–71 | Hallkörperverzögerung bis 45 ms; in Chorus Modulationstiefe |
| PRE DELAY | 0–255 ms | Vorverzögerung, geglättet |

| SHIFT-Seite | Bereich | Standard |
|---|---|---|
| MIX | 0–100 % | 35 % |
| INPUT | −12…+12 dB | +2 dB |
| OUTPUT | −60…+12 dB | +4 dB |
| BANDWIDTH | 3–20 kHz | 8,1 kHz |
| MODE ENHANCEMENT | 1–16 | 4, aktiv |
| DECAY OPTIMIZATION | 1–16 | 4, aktiv |

BASS/MID und Frequenzen verwenden logarithmische Kennlinien. BANDWIDTH ist ein Butterworth-Tiefpass achter Ordnung. Seine effektive Grenzfrequenz ist auf 45 % der Samplerate begrenzt (bei 44,1 kHz also 19,845 kHz). Decay Optimization verkürzt Hallzeiten bei aktivem Eingang weich. Enhancement moduliert Delayzeiten langsam; hohe Werte werden hörbarer. Nachhall ist frequenzabhängig; insbesondere TREBLE DECAY senkt die gemessene RT60 oberer Oktaven. Keine samplegenaue Übereinstimmung mit historischer Hardware.

Chorus A nutzt vier modulierte Delaystimmen ohne FDN. BASS, MID, CROSS OVER, TREBLE DECAY und PRE DELAY beeinflussen dort den Klang nicht. Die zweite Seite bleibt für Gain, Bandbreite und Mix wirksam; die Reverb-spezifischen Enhancement-/Optimization-Funktionen sind im Chorus nicht wirksam.

## Presetformat und Wiederherstellung

Standard unter Windows: `%APPDATA%\White224\Presets\`. Versioniertes XML, Endung `.w224`, Wurzel `White224 version="1"`. Enthält `PARAMETERS` mit allen 17 eindeutigen `PARAM id/value`-Kindern, `Registers` mit vier `Slot index="0..3"` und UI-Metadaten. Register enthalten jeweils einen vollständigen PARAMETERS-Baum. Nicht belegte Slots bleiben leer. Factory-Index wird zusätzlich gespeichert; nach manueller Bearbeitung bleibt er der zuletzt geladenen Vorlage zugeordnet, ohne einen ungespeicherten Zustand als neues Preset auszugeben.

Dateien über 1 MB, unvollständige/duplizierte/unbekannte Parameter, nicht endliche oder außerhalb der Bereiche liegende Werte und unbekannte Versionen werden abgelehnt. Erst nach vollständiger Validierung wird der Zustand übernommen. Speichern verwendet eine temporäre Datei und atomaren Ersatz. Derselbe Inhalt wird für DAW-Zustände mit JUCE-Binär-XML gespeichert. Dateiformatversion 1 ist absichtlich strikt.

Factory: 9 Programs, 7 Blade Runner, 5 Halls & Spaces, 11 Long Decays, 5 Plates & Drums, 5 Character. Nur die dokumentierten acht Primärdatensätze stammen aus der gelieferten Tabelle. Zusätzliche DEPTH-/Choruswerte und alle 33 weiteren Presets sind eigenständige Annahmen. „Seventy Seconds“ ist endlich und kein unendliches Freeze. „Moving Plate“ ist ein moduliertes Reverb; „Wide Chorus“ der reine Chorus.

## Fehlerbehebung und offene praktische Prüfungen

- VST3 nicht gefunden: komplettes Bundle kopieren, x64-Host nutzen, Cubase-Blockliste/Scan prüfen. Erst einen aktuellen Build testen.
- Kein Ton: Host-Eingang, Buslayout und Geräteeinstellungen prüfen. 100% Wet mit langem Predelay kann verzögert einsetzen. Mono-Eingang muss auf Stereo-Ausgang geroutet sein.
- Pegel zu hoch: INPUT/OUTPUT reduzieren; die voreingestellten +2/+4 dB sind absichtlich aus der Vorgabe übernommen. Intern gibt es keinen Brickwall-Limiter; Ausgangspegel kann 0 dBFS überschreiten.
- Sehr lange Hallzeiten: hohe CPU ist nicht durch entsprechend längere Puffer bedingt; Feedbackverluste werden kleiner. Starke Parametereingriffe können charakteristische Tonhöhenbewegungen durch Delay-Glättung erzeugen.
- Fehlendes Preset: Standardordner und Endung kontrollieren; beliebige externe Presets per LOAD öffnen.
- Download: SHA256 des heruntergeladenen ZIP mit beiliegender Prüfsumme vergleichen, z. B. `Get-FileHash .\White224-....zip -Algorithm SHA256`. ZIP vollständig entpacken. Ein Workflow-Artefakt ist zeitlich begrenzt und kann GitHub-Anmeldung erfordern; ein dauerhaftes Release muss separat veröffentlicht werden.

Noch praktisch erforderlich: ASIO-/Audiogerätetest, Hörprüfung, Cubase 15 Pro Scan/Automation/Offline-Render/Projektwiederherstellung, Windows-DPI bei 100/150/200 %, reale CPU-Spitzen unter DAW-Last, Bildschirmabgleich nach Lieferung der Referenzbilder. Diese Punkte werden nicht durch einen Linux-Test ersetzt.
