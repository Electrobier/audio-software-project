# Abweichungen und offene Abnahme

## Referenz und Klang

Im Upload war ausschließlich die Textspezifikation, kein Screenshot und kein Referenzaudio enthalten. Deshalb konnte kein Bildvergleich erfolgen. Die GUI folgt der beschriebenen Anordnung mit dunklem Rack, zwei VU-Metern, Hauptreglern, Tone/Link und blauem Bedienbereich. Sie verwendet vollständig eigene Vektorgrafik und Schriftzüge. Eine optisch genaue Reproduktion ist nicht verifiziert.

Das DSP ist eine nachvollziehbare Eigenentwicklung. Es existiert keine gemessene Übereinstimmung mit Waves API 2500 oder API-Hardware. Thrust und Feedback sind eigene Näherungen des beschriebenen Funktionsprinzips. Nicht enthalten sind Bauteiltoleranzmodelle, Original-Übertrager, Schaltungsmodellierung und ein authentisch gemessener Frequenzgang. Analog arbeitet symmetrisch; asymmetrische Färbung und Kanalabweichungen sind nicht enthalten.

## Technische Einschränkungen

- JUCE 8.0.6 ist reproduzierbar festgelegt, nicht die aktuellste verfügbare Version.
- Windows-Build mit MSVC und Cubase-Praxistest sind offen. Der versuchte Linux→Windows-Cross-Build scheiterte an JUCEs ausdrücklicher MinGW-Ablehnung. Es wird kein ungetestetes Windows-Binary als fertiges Release ausgegeben.
- Die Oberfläche ist skalierbar; Windows-DPI-Wechsel bei mehreren Monitoren wurde nicht praktisch getestet.
- Norm-VU-Ballistik wird durch RMS-/Zeigerglättung angenähert. Kein True-Peak-Meter.
- Ratio Inf ist 1000:1; Attack lässt Transienten passieren. Kein Brickwall-Limiter.
- Sidechain- und Topologieschalter werden direkt geschaltet. Die kontinuierlichen Parameter und Analog/Bypass werden geglättet; Moduswechsel können hörbare Übergänge erzeugen.
- Shape filtert den gesamten Detektorpfad; IND deaktiviert diesen Filter, während der separate Link-Regler aktiv bleibt. Das ist eine dokumentierte Auslegung der nicht eindeutig beschriebenen Referenz.
- Kurzes 2× FIR-Oversampling, keine hochgradige Aliasing-Unterdrückung oder Messgarantie für Mastering.
- Acht Samples konstante Latenz. Keine externe Sidechain, kein automatisches Make-up, kein Tempo-Sync.
- A/B-Alternativzustand und Fenstergröße werden nicht im Projekt gespeichert. Der aktive Parameterzustand und das zuletzt gewählte Werksprogramm werden gespeichert.
- Undo/Redo vorhanden; keine separate Preset-Datenbank, kein Dirty-Indikator. Keine manuell kalibrierten Original-Reglerskalen; numerische Felder zeigen die exakten Werte.
- Unabhängiger Steinberg-Validator, Dauerlauf, Windows-CPU-/Treiberprüfung, Hörtests mit Musik sowie Cubase-Automation/Offline-Export stehen noch aus. Die eigenen Hosttests ersetzen keine offizielle Zertifizierung.

## Empfohlene Windows-Abnahme

1. Release und Debug mit dem beigefügten PowerShell-Skript bauen; CTest bestehen lassen.
2. VST3 mit einem aktuellen Steinberg-Validator und optional pluginval prüfen.
3. In Cubase Mono und Stereo mit allen sechs Abtastraten öffnen; 32-/64-Bit-Audio und Offline-Export prüfen.
4. Projekt mit Automation aller 16 Parameter speichern, schließen, neu öffnen und exportierte Audiodateien vergleichen.
5. Bypass, acht Samples Latenzausgleich und Parallelmischung mit Impulsen prüfen.
6. GUI bei 100/125/150/200 % und bei Monitorwechsel bedienen; Benutzer-Presets, A/B und Undo/Redo testen.
7. Hörvergleich und Messungen gegen gewünschte Klangreferenz durchführen, bevor ein Produktionsrelease deklariert wird.
