# IRON 2500 · Handbuch (0.1.0)

Eigenständiger, analog inspirierter VCA-Kompressor für Mono und Stereo. Kein Produkt von Waves oder API. Der Klang wurde nicht anhand eines Originalgeräts oder des Waves-Plugins kalibriert. Diese Version ist ein Entwicklungsstand; vor produktivem Einsatz bitte in der eigenen DAW prüfen.

## Installation unter Windows

Das komplette Verzeichnis **IRON 2500.vst3** aus dem Windows-Paket nach
`C:\Program Files\Common Files\VST3\` kopieren. Der Explorer fragt dort gegebenenfalls nach Administratorrechten. Cubase neu starten bzw. den VST-Plug-in-Manager aktualisieren. Hersteller: Iron Instruments; Kategorie: Dynamics. Das Bundle nicht in seine Einzeldateien zerlegen. `IRON 2500.exe` ist die zusätzliche Standalone-Anwendung; über deren Audio-Einstellungen Gerät, Eingänge und Ausgänge wählen. Bei Standalone-Nutzung Rückkopplungen zwischen Lautsprechern und Mikrofon vermeiden.

Windows 10/11 x64 sind das Ziel. Cubase und ein Windows-Audiotreiber stehen in der Entwicklungsumgebung nicht zur Verfügung; praktische Windows-/Cubase-Abnahme ist offen. Die beigefügte Testdokumentation unterscheidet lokale Prüfungen und offene Prüfungen.

## Bedienung

Regler ziehen oder Mausrad verwenden. Shift beim Ziehen schaltet auf Feineinstellung. Doppelklick stellt den Parameterdefault wieder her. Die Wertefelder sind editierbar; bei diskreten Reglern werden die bezeichneten Stufen verwendet. Auswahlfelder schalten die Betriebsarten. Tooltips erläutern die Bedienung. Das Fenster kann proportional skaliert werden; OPTIONS bietet 100/125/150/200 %.

Die Presetliste und Pfeile laden zwölf Werks-Presets. Beim Laden werden vorhandene Einstellungen ersetzt. Änderungen sind sofort hörbar und werden im DAW-Projekt gespeichert. OPTIONS speichert/lädt eigene `.ironpreset`-Dateien. A/B tauscht die aktuelle Einstellung mit einem zweiten Snapshot; beide beginnen beim Erzeugen des Prozessors mit Clean. Der alternative Snapshot ist flüchtig und wird nicht im DAW-Projekt gespeichert. Undo/Redo ist über OPTIONS erreichbar und verwendet JUCEs Parameterzustandsverwaltung. Die Presetliste zeigt das zuletzt ausgewählte Werks-Preset, auch wenn Parameter danach verändert wurden; ein Dirty-Indikator ist noch nicht vorhanden.

## Parameter

| Parameter | Bereich / Funktion |
|---|---|
| Threshold | −20 bis +10 dBFS. Bezug auf den Detektorpegel; Input Trim wirkt davor. |
| Attack | 0,03 / 0,1 / 0,3 / 1 / 3 / 10 / 30 ms. Zeitkonstante der GR-Hüllkurve in New. |
| Ratio | 1,5 / 2 / 3 / 4 / 6 / 10 / Inf:1. Inf wird intern durch 1000:1 angenähert; kein True-Peak-Limiter. |
| Release | 0,05 / 0,1 / 0,2 / 0,5 / 1 / 2 s. Zeitkonstante in New bei Variable=0. |
| Variable | 0–100 %. Verlängert Release abhängig von der über 400 ms gemittelten GR bis auf das Vierfache. |
| Knee | Hard=0, Medium=6, Soft=12 dB Übergangsbreite; quadratischer Übergang. |
| Thrust | Normal: unverändert. Medium: moderate Bassabsenkung/Höhenbetonung. Loud: stärkere Gewichtung. Ausschließlich Sidechain. |
| Type | New: Peak-Feedforward. Old: Peak-Feedback mit einer Sample Rückführung vor Sättigung. F Back: Feedback mit 4-ms-Leistungsdetektor und 1,4-fachen Zeitkonstanten. |
| Shape | BP: Differenz zweier Tiefpässe (180/2500 Hz). HP: Hochpass 180 Hz. LP: Tiefpass 180 Hz. IND: ungefilterter individueller Detektor. |
| L/R Link | 0–100 %. Interpoliert die individuellen dB-Detektorpegel zum größeren Pegel beider Kanäle. 100 % koppelt die Regelung. IND deaktiviert nur den Shape-Filter, nicht den Link-Regler. |
| Analog | Symmetrische tanh-Sättigung, 2× Oversampling mit zwei 17-Tap-FIR-Filtern. Kein künstliches Rauschen; keine erzeugte Gleichspannung bei symmetrischem Signal. |
| Output | −24 bis +24 dB nach Dry/Wet. Keine automatische Aufholverstärkung. |
| Mix | 0–100 %, lineare Parallelmischung. Dry wird vor Input Trim abgenommen und um acht Samples verzögert. 0 % ist bei Output=0 der zeitversetzte Originaleingang. |
| Trim | −18 bis +18 dB ausschließlich vor Kompressor und Sidechain, im Wet-Pfad. |
| VU Source | GR / Output / Input. Zwei unabhängige Kanalanzeigen. |
| Bypass | Verzögerter Originaleingang, unabhängig von Trim/Output/Analog. Die Engine läuft weiter; Ein-/Ausschalten wird geglättet. |

Alle 16 Parameter sind über JUCE/APVTS für Host-Automation verfügbar. Der Sample-Rate-Bereich beträgt 44,1–192 kHz; Mono/Stereo, Float/Double und beliebige Blockgrößen werden unterstützt. Kein MIDI und keine externe Sidechain.

## Signalfluss und Pegel

Input → Input Trim → Gain Reduction → optional Analog → Dry/Wet → Output.
Die Sidechain zweigt nach Input Trim ab. Feedback greift den Wet-Ausgang der Gain-Reduction vor Analog ab. Filterzustände sind pro Kanal unabhängig. In Mono werden beide Detektoren mit demselben Signal gespeist.

Die Analog-FIR-Stufe und die parallelen sauberen Pfade sind auf acht Samples ausgerichtet; diese Latenz wird dem Host gemeldet, auch bei Analog Off oder Bypass. Das sind ca. 0,181 ms bei 44,1 kHz bzw. 0,042 ms bei 192 kHz. Die kurze FIR-Auslegung reduziert Aliasing, bietet aber keine unbegrenzte Unterdrückung. Im Analog-Modus beeinflusst der FIR-Frequenzgang die obere Audiobandgrenze. Es gibt keinen Ausgangsbegrenzer: starke positive Trim-/Output-Werte können den Host über 0 dBFS aussteuern.

Kontinuierliche Steuerwerte werden mit 10-ms-Einpolglättung verarbeitet. Modusschalter ändern die Detektortopologie unmittelbar; bei starkem Signal können schnelle Moduswechsel hörbar sein. Feedback wird mit impliziter Relaxation gegen Instabilität bei sehr kurzer Attack und hoher Ratio stabilisiert. Seine transienten Zeitverläufe unterscheiden sich bewusst von New.

## Meter

Input/Output: RMS-Leistung mit 300-ms-Einpolballistik; 0 VU entspricht −18 dBFS RMS. Eine Sinuswelle mit −15 dBFS Peak liegt deshalb ungefähr bei 0 VU. Das ist eine VU-artige Anzeige, kein normgerecht kalibriertes elektromechanisches VU-Instrument. GR: 300-ms-geglättete positive dB-Reduktion, Skala 20…0 dB. Zusätzlich werden die Zeiger bei 30 Hz geglättet. Anzeige begrenzt, Audio nicht begrenzt.

## Startpunkte

Mixbus: Mix Bus Glue, dann Threshold für ca. 1–3 dB GR anpassen. Drums: Rock Drum Punch für betonte Anschläge; Parallel Drum Smash für zusätzliche Dichte. Masterbus: Gentle Master Bus vorsichtig einsetzen und den Ausgangspegel mit Bypass vergleichen. Presets sind Startwerte und müssen zum Quellpegel passen.
