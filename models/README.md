# Mitgeliefertes Audiomodell

EfficientAT mn04_as, AudioSet-Klassifikation und 384-dimensionale Embeddings.

- Quelle: https://github.com/fschmid56/EfficientAT
- Gewichte: https://github.com/fschmid56/EfficientAT/releases/download/v0.0.1/mn04_as_mAP_432.pt
- Lizenz: MIT, siehe LICENSE-EfficientAT.txt
- ONNX-Export: opset 17, Eingabe `mel` (1, 1, 128, Zeit), Ausgaben `logits` (1, 527) und `embedding` (1, 384).
- Modellarchitektur: MobileNetV3, width_mult=0.4; veröffentlichter AudioSet-mAP laut Quelle 43,2 %. Das ist keine Trefferquote für Sample-Unterkategorien.
- Vorverarbeitung: Mono 32 kHz; Preemphasis 0,97; STFT n_fft=1024, win_length=800, hop=320, symmetrisches Hann-Fenster; Kaldi-Melbank 128 Bänder, 0–15 kHz; log(mel + 1e-5); Normalisierung (x + 4,5)/5.
- Vorverarbeitungsgewichte in preprocess.npz wurden aus der Originalimplementierung übernommen. `labels.json` enthält die 527 AudioSet-Klassennamen aus metadata/class_labels_indices.csv.
- Bei langen Signalen werden bis zu drei 10-s-Fenster aus dem DSP-Analyseausschnitt aggregiert. Bei kurzen Signalen wird auf mindestens 1 s aufgefüllt.
- Numerischer Vergleich: maximale Logitabweichung ONNX/PyTorch beim Export 9,54e-6; NumPy-/Original-Mel-Vorverarbeitung 3,82e-6 auf dem Testsignal. Das bestätigt die Konvertierung, keine musikalische Klassifikationsqualität.
- Die App aktiviert keinen Modelldownload und deaktiviert ONNX-Telemetrie. Alle Modellartefakte sind lokal enthalten.

SHA256SUMS.json erlaubt die Prüfung der mitgelieferten Dateien. Das Originalprojekt ist vom eigenständigen Sample-Manager unabhängig.
